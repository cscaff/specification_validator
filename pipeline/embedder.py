"""
Embedder module for the Spec Validator Pipeline.

Handles embedding synthesized controllers into game files.
"""

from pathlib import Path
from typing import Optional


CONTROLLER_MARKER_START = "\n/* ======================================== CONTROLLER ======================================== */\n"
CONTROLLER_MARKER_END = "\n/* ======================================== CONTROLLER END ======================================== */\n"


def extract_controller_function(synthesis_output: str) -> str:
    """
    Extract the main() function from synthesis output and rename it to step_controller().

    Args:
        synthesis_output: Raw output from the synthesis tool

    Returns:
        The controller function code ready for embedding

    Raises:
        ValueError: If no main() function is found in the output
    """
    separator = "void main() "

    if separator not in synthesis_output:
        raise ValueError("No 'void main()' function found in synthesis output")

    _, _, after = synthesis_output.partition(separator)

    return f"void step_controller() {after}"


def embed_controller(
    game_path: Path,
    controller_code: str,
    output_path: Optional[Path] = None,
) -> Path:
    """
    Embed a controller into a game file.

    Args:
        game_path: Path to the game file
        controller_code: The controller function code
        output_path: Optional output path (modifies game_path in place if None)

    Returns:
        Path to the modified game file

    Raises:
        FileNotFoundError: If game file doesn't exist
    """
    if not game_path.exists():
        raise FileNotFoundError(f"Game file not found: {game_path}")

    # Read existing content
    content = game_path.read_text()

    # Remove existing controller if present
    marker_idx = content.find(CONTROLLER_MARKER_START)
    if marker_idx != -1:
        content = content[:marker_idx]

    # Build new controller block
    controller_block = (
        CONTROLLER_MARKER_START +
        controller_code.rstrip() +
        CONTROLLER_MARKER_END
    )

    # Append controller
    content = content.rstrip() + controller_block

    # Write to output
    target_path = output_path if output_path else game_path
    target_path.write_text(content)

    return target_path


def embed_from_synthesis(
    game_path: Path,
    synthesis_output: str,
    output_path: Optional[Path] = None,
) -> Path:
    """
    Extract controller from synthesis output and embed into game file.

    Convenience function combining extract_controller_function and embed_controller.

    Args:
        game_path: Path to the game file
        synthesis_output: Raw output from synthesis tool
        output_path: Optional output path

    Returns:
        Path to the modified game file
    """
    controller_code = extract_controller_function(synthesis_output)
    return embed_controller(game_path, controller_code, output_path)
