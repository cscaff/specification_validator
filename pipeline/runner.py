"""
Runner module for the Spec Validator Pipeline.

Handles game compilation and execution.
"""

from dataclasses import dataclass, field
from pathlib import Path
import shutil
import signal
import subprocess
import tempfile
from typing import Optional


MAKEFILE_CONTENT = """
CC := gcc
CFLAGS := -std=c11 -O0 -g -Wall -Wextra
TARGET := game_binary
SRC := game.c cJSON.c

all: $(TARGET)

$(TARGET): $(SRC)
\t$(CC) $(CFLAGS) -o $@ $^

clean:
\trm -f $(TARGET)
"""


@dataclass
class RunResult:
    """Result of a single game run."""
    run_number: int
    success: bool
    return_code: int
    output: str
    steps: Optional[int] = None

    @property
    def status_str(self) -> str:
        return "PASS" if self.success else "FAIL"


@dataclass
class ExecutionResult:
    """Result of executing multiple game runs."""
    total_runs: int
    passed: int
    failed: int
    runs: list[RunResult] = field(default_factory=list)

    @property
    def pass_rate(self) -> float:
        if self.total_runs == 0:
            return 0.0
        return self.passed / self.total_runs

    @property
    def pass_percentage(self) -> str:
        return f"{self.pass_rate * 100:.1f}%"


def build_game(tmp_dir: Path, debug: bool = False) -> bool:
    """
    Build the game in a temporary directory.

    Args:
        tmp_dir: Directory containing the game source
        debug: If True, print build output

    Returns:
        True if build succeeded
    """
    # Write Makefile
    (tmp_dir / "Makefile").write_text(MAKEFILE_CONTENT)

    process = subprocess.Popen(
        ["make"],
        cwd=tmp_dir,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
    )

    output_lines = []
    for line in process.stdout:
        output_lines.append(line)
        if debug:
            print(line, end="")

    process.stdout.close()
    process.wait()

    return process.returncode == 0


def run_once(
    tmp_dir: Path,
    config_path: Optional[Path] = None,
    debug: bool = False,
    timeout: int = 60,
) -> tuple[int, str]:
    """
    Run the game binary once.

    Args:
        tmp_dir: Directory containing the compiled game
        config_path: Optional path to constraints config file
        debug: If True, print output live
        timeout: Timeout in seconds

    Returns:
        Tuple of (return_code, output)
    """
    cmd = ["./game_binary"]
    if config_path:
        cmd.append(str(config_path))

    try:
        process = subprocess.Popen(
            cmd,
            cwd=tmp_dir,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
        )

        output_lines = []
        for line in process.stdout:
            output_lines.append(line)
            if debug:
                print(line, end="")

        process.stdout.close()
        process.wait(timeout=timeout)

        return process.returncode, "".join(output_lines)

    except subprocess.TimeoutExpired:
        process.kill()
        return -1, "Timeout exceeded"


def parse_steps_from_output(output: str) -> Optional[int]:
    """Extract the number of steps from game output."""
    import re
    match = re.search(r"in (\d+) steps", output)
    if match:
        return int(match.group(1))
    return None


def classify_result(return_code: int) -> bool:
    """Classify a return code as success or failure."""
    return return_code == 0


def run_game(
    game_path: Path,
    cjson_dir: Path,
    constraints_path: Optional[Path] = None,
    runs: int = 1,
    debug: bool = False,
    timeout: int = 60,
) -> ExecutionResult:
    """
    Run the game multiple times and collect results.

    Args:
        game_path: Path to the game source file
        cjson_dir: Path to the cJSON directory
        constraints_path: Optional path to constraints JSON
        runs: Number of times to run the game
        debug: If True, print detailed output
        timeout: Timeout per run in seconds

    Returns:
        ExecutionResult with all run statistics
    """
    results = []
    passed = 0

    with tempfile.TemporaryDirectory() as tmp:
        tmp_dir = Path(tmp)

        # Copy game source
        shutil.copy(game_path, tmp_dir / "game.c")

        # Copy cJSON files
        shutil.copy(cjson_dir / "cJSON.c", tmp_dir / "cJSON.c")
        shutil.copy(cjson_dir / "cJSON.h", tmp_dir / "cJSON.h")

        # Build
        if debug:
            print(f"Building game in {tmp_dir}...")

        if not build_game(tmp_dir, debug=debug):
            return ExecutionResult(
                total_runs=runs,
                passed=0,
                failed=runs,
                runs=[RunResult(i+1, False, -1, "Build failed") for i in range(runs)],
            )

        # Run N times
        for i in range(1, runs + 1):
            if debug:
                print(f"\n--- Run {i}/{runs} ---")

            rc, output = run_once(
                tmp_dir,
                config_path=constraints_path,
                debug=debug,
                timeout=timeout,
            )

            success = classify_result(rc)
            steps = parse_steps_from_output(output)

            if success:
                passed += 1

            result = RunResult(
                run_number=i,
                success=success,
                return_code=rc,
                output=output,
                steps=steps,
            )
            results.append(result)

            # Print error info if not debug (debug already printed)
            if not debug and not success and rc != 0:
                if rc < 0:
                    sig = -rc
                    sig_name = signal.Signals(sig).name if sig in signal.Signals.__members__.values() else f"SIG{sig}"
                    print(f"  Run {i}: Terminated by signal {sig_name}")
                else:
                    print(f"  Run {i}: Exited with code {rc}")

    return ExecutionResult(
        total_runs=runs,
        passed=passed,
        failed=runs - passed,
        runs=results,
    )
