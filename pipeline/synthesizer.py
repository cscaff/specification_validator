"""
Synthesizer module for the Spec Validator Pipeline.

Handles interaction with the Issy synthesis tool.
"""

from dataclasses import dataclass
from pathlib import Path
import subprocess
import time
from typing import Optional


@dataclass
class SynthesisResult:
    """Result of a synthesis operation."""
    success: bool
    controller_code: str
    output: str
    duration: float
    error_message: Optional[str] = None


def synthesize(
    spec_path: Path,
    command: str = "issy",
    args: Optional[list[str]] = None,
    debug: bool = False,
) -> SynthesisResult:
    """
    Synthesize a controller from a TSLMT specification.

    Args:
        spec_path: Path to the .tslmt specification file
        command: The synthesis command (default: "issy")
        args: Additional arguments for the synthesis tool
        debug: If True, print synthesis output live

    Returns:
        SynthesisResult with the controller code or error information
    """
    if args is None:
        args = [
            "--tslmt", "--solve", "--synt", "--info",
            "--pruning", "1",
            "--accel-attr", "geom-ext",
            "--accel-difficulty", "easy"
        ]

    full_command = [command] + args + [str(spec_path)]

    start_time = time.time()

    try:
        process = subprocess.Popen(
            full_command,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
        )

        output_lines = []

        for line in iter(process.stdout.readline, ''):
            if debug:
                print(line.rstrip())
            output_lines.append(line)

        process.stdout.close()
        process.wait()

        duration = time.time() - start_time
        output = ''.join(output_lines)

        if process.returncode != 0:
            return SynthesisResult(
                success=False,
                controller_code="",
                output=output,
                duration=duration,
                error_message=f"Synthesis failed with return code {process.returncode}",
            )

        return SynthesisResult(
            success=True,
            controller_code=output,
            output=output,
            duration=duration,
        )

    except FileNotFoundError:
        return SynthesisResult(
            success=False,
            controller_code="",
            output="",
            duration=time.time() - start_time,
            error_message=f"Synthesis command not found: {command}",
        )
    except Exception as e:
        return SynthesisResult(
            success=False,
            controller_code="",
            output="",
            duration=time.time() - start_time,
            error_message=str(e),
        )
