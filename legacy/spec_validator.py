from pathlib import Path
import subprocess
import tempfile
import argparse
import re
import os


def main():
    parser = argparse.ArgumentParser(
        description="Controller synthesis and test harness"
    )

    parser.add_argument(
        "mode",
        choices=["synthesize", "run", "both"],
        default="both",
        help="What to do: synthesize only, run only, or synthesize then run",
    )

    parser.add_argument(
        "--runs",
        type=int,
        default=1,
        help="Number of runs when mode is run or both (default: 1)",
    )

    parser.add_argument(
        "--game",
        type=str,
        default="../path/to/game.c",
        help="Path to game.c (default: ../path/to/game.c)",
    )

    parser.add_argument(
        "--live-log",
        action="store_true",
        help="Print controller output live during each run",
    )

    parser.add_argument(
        "spec_name",
        help="File Name of Spec Located in Src Folder.\nExample: \"src/cop.tslmt\" -> name: \"cop\".\n"
    )

    parser.add_argument(
        "--config",
        type=str,
        default=None,
        help="Path to constraints config file (passed as argv[1] to game_binary).",
    )

    args = parser.parse_args()

    if args.mode in ("synthesize", "both"):
        print("=== Synthesizing controller ===")
        # Generate Controller
        spec_path = f"src/{args.spec_name}.tslmt"
        controller_str = generate_controller(spec_path)

        # Write Controller to File
        write_controller(args.spec_name, controller_str)

    if args.mode in ("run", "both"):
        print(f"=== Running controller ({args.runs} runs) ===")
        test_controller(
            args.game,
            runs=args.runs,
            live_log_each_run=args.live_log,
            config_path=args.config,
        )

# ======================================== CONTROLLER GENERATOR ========================================

def generate_controller(file_path: str):
    """
    Generate Controller
    
    Generates Controller for Inserted TSL Specification Path.
    """

    # Issy Command Set Up
    spec_path = file_path
    command = ["issy", "--tslmt", "--solve", "--synt", "--info", "--pruning", "1", "--accel-attr", "geom-ext", "--accel-difficulty", "easy", spec_path]

    # Call Issy Command
    process = subprocess.Popen(command,
                               stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT,
                               text=True,
                               bufsize=1
                               )
    
    # Live Log Printing
    output_lines = []

    # Print + Save Log
    for line in iter(process.stdout.readline, ''):
        print(line.rstrip())
        output_lines.append(line)

    process.stdout.close()
    process.wait()

    stdout = ''.join(output_lines)

    if process.returncode != 0:
        raise RuntimeError("Issy Failed")
    
    return stdout


def write_controller(game_name: str, controller_str: str):
    """
    Write Controller

    Creates New Controller File to write the controller to. Appends proper header and
    input reading function.
    """

    # Polish Controller
    separator = "void main() "
    _, _, after = controller_str.partition(separator)

    # Controller Indicator
    controller_marker = "\n/* ======================================== CONTROLLER ======================================== */\n"

    controller_str = (f"{controller_marker}void step_controller() {after}"
                      f"\n/* ======================================== CONTROLLER END ======================================== */\n")

    # Find Game File
    file_name = f"{game_name}_game.cpp"
    file_path = f"games/{file_name}"

    if not os.path.exists(file_path):
        raise RuntimeError(f"{file_path} does not exist.")         
    
    with open(file_path, "r", encoding="utf-8") as f:
        content = f.read()

    # Remove Existing Controller
    idx = content.find(controller_marker)
    if idx != -1:
        content = content[:idx]

    # Append Controller
    content += controller_str

    # Write Controller to end of Game File
    with open(file_path, "w", encoding="utf-8") as f:
        f.write(content)

    print(f"Controller Written to {file_path} successfully")

# ======================================== GAME RUNNER ========================================

MAKEFILE_CONTENT = r"""
CC := gcc
CFLAGS := -std=c11 -O0 -g -Wall -Wextra
TARGET := game_binary
SRC := game.c cJSON.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)
"""


def build(tmp_dir: Path) -> None:
    (tmp_dir / "Makefile").write_text(MAKEFILE_CONTENT)

    process = subprocess.Popen(
        ["make"],
        cwd=tmp_dir,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,   # merge stderr
        text=True,
        bufsize=1,
    )

    assert process.stdout is not None

    output_lines: list[str] = []

    # Stream output live + capture
    for line in process.stdout:
        output_lines.append(line)
        print(line, end="")

    process.stdout.close()
    process.wait()

import signal
from pathlib import Path
import subprocess


def run_once(
    tmp_dir: Path,
    live_log: bool = False,
    config_path: str | None = None,
) -> tuple[int, str]:
    """
    Returns:
      (exit_code, combined_output)
        - exit_code is the raw process returncode
        - combined_output is stdout + stderr text
    """

    cmd = ["./game_binary"]
    if config_path:
        cmd.append(config_path)

    process = subprocess.Popen(
        cmd,
        cwd=tmp_dir,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,   # merge stderr
        text=True,
        bufsize=1,
    )

    assert process.stdout is not None

    output_lines: list[str] = []

    # Stream output live + capture
    for line in process.stdout:
        output_lines.append(line)
        if live_log:
            print(line, end="")

    process.stdout.close()
    process.wait()

    combined_output = "".join(output_lines)
    rc = process.returncode


    if combined_output.strip():
        print("----- captured output -----")
        print(combined_output.rstrip())


    # ================= ERROR REPORTING =================
    if rc != 0:
        print("\n========== GAME ERROR ==========")

        if rc < 0:
            # Killed by signal
            sig = -rc
            sig_name = signal.Signals(sig).name if sig in signal.Signals.__members__.values() else f"SIG{sig}"
            print(f"Terminated by signal: {sig_name} ({sig})")
        else:
            print(f"Exited with code: {rc}")

        print("========== END ERROR ==========\n")

    return rc, combined_output


def classify_returncode(rc: int) -> int:
    """
    Your C program likely calls exit(-1), which shows up as:
      - 255 on Unix (because exit codes are 0..255)
    We'll treat 0 as success, anything else as failure.
    """
    return 0 if rc == 0 else -1

def test_controller(source_path: str, runs: int, live_log_each_run: bool = False, config_path: str = None) -> None:
    positives = 0

    with tempfile.TemporaryDirectory() as tmp:
        tmp_dir = Path(tmp)

        # Copy game source
        (tmp_dir / "game.c").write_text(Path(source_path).read_text())

        # Copy cJSON sources
        cjson_dir = Path("third_party/cjson")
        (tmp_dir / "cJSON.c").write_text((cjson_dir / "cJSON.c").read_text())
        (tmp_dir / "cJSON.h").write_text((cjson_dir / "cJSON.h").read_text())

        # Build once
        print(f"Building game in temporary directory {tmp_dir}...")
        build(tmp_dir)

        # Run N times
        for i in range(1, runs + 1):
            print(f"RUN {i}")
            rc, out = run_once(tmp_dir, live_log=live_log_each_run, config_path=config_path)
            status = classify_returncode(rc)

            if status == 0:
                positives += 1

            # Log per-run result
            print(f"[run {i}/{runs}] returncode={rc} -> {'POSITIVE (0)' if status == 0 else 'NEGATIVE (-1)'}")

        print(f"\nSummary: {positives}/{runs} positive runs")


if __name__ == "__main__":
    main()
