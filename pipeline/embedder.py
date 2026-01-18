"""
Embedder module for the Spec Validator Pipeline.

Handles embedding synthesized controllers into game files.
Supports both legacy embedding (into existing files) and template-based
generation where game files are generated from templates with config params.
"""

import re
from pathlib import Path
from typing import Any, Optional

from .game_templates import generate_game


CONTROLLER_MARKER_START = "\n/* ======================================== CONTROLLER ======================================== */\n"
CONTROLLER_MARKER_END = "\n/* ======================================== CONTROLLER END ======================================== */\n"


def extract_controller_code(synthesis_output: str, rename_main: bool = False, inject_start_pos: bool = False) -> str:
    """
    Extract the complete controller code from synthesis output, including
    variable declarations and the main function.

    Args:
        synthesis_output: Raw output from the synthesis tool
        rename_main: If True, rename main() to step_controller() (legacy behavior)
        inject_start_pos: If True, inject x = START_X; y = START_Y; at start of main()

    Returns:
        The complete controller code ready for embedding

    Raises:
        ValueError: If no valid C code is found in the output
    """
    if "void main()" not in synthesis_output:
        raise ValueError("No 'void main()' function found in synthesis output")

    # Find the start of the C code
    # Look for common patterns that indicate start of C code:
    # - #include directives
    # - Variable declarations (bool, int, etc.)
    # - void read_inputs()

    lines = synthesis_output.split('\n')
    code_start_idx = None

    # Patterns that indicate start of C code
    c_code_patterns = [
        r'^#include\s*<',
        r'^(bool|int|char|float|double|void|unsigned|signed)\s+\w+',
        r'^_Bool\s+\w+',
    ]

    for i, line in enumerate(lines):
        stripped = line.strip()
        for pattern in c_code_patterns:
            if re.match(pattern, stripped):
                code_start_idx = i
                break
        if code_start_idx is not None:
            break

    if code_start_idx is None:
        # Fallback: find "void main()" and look backwards for declarations
        for i, line in enumerate(lines):
            if 'void main()' in line:
                # Look backwards for the start of C code
                code_start_idx = i
                for j in range(i - 1, -1, -1):
                    stripped = lines[j].strip()
                    if stripped and not stripped.startswith('//') and not stripped.startswith('/*'):
                        # Check if this looks like C code
                        if any(re.match(p, stripped) for p in c_code_patterns) or stripped == '}' or stripped.startswith('void '):
                            code_start_idx = j
                        elif stripped and not any(c in stripped for c in ['=', '{', '}', '(', ')', ';']):
                            # Looks like non-code text, stop here
                            break
                break

    if code_start_idx is None:
        raise ValueError("Could not find start of C code in synthesis output")

    # Extract from code_start_idx to end
    code_lines = lines[code_start_idx:]
    code = '\n'.join(code_lines)

    # Remove read_inputs() stub function definition - the game template provides this
    # Matches patterns like: void read_inputs() { /* INSERT HERE */ }
    # or: void read_inputs(void) { ... }
    code = re.sub(
        r'void\s+read_inputs\s*\(\s*(?:void)?\s*\)\s*\{[^}]*\}\s*',
        '',
        code
    )

    # Optionally rename main() to step_controller() (legacy behavior)
    if rename_main:
        code = re.sub(r'\bvoid\s+main\s*\(\s*\)', 'void step_controller()', code)
    else:
        # Convert void main() to int main() for C standard compliance
        code = re.sub(r'\bvoid\s+main\s*\(\s*\)', 'int main()', code)

    # Optionally inject start position initialization at the start of main()
    # This ensures x and y are set to START_X and START_Y (defined in game harness)
    # before any controller logic runs, overriding any default initialization
    if inject_start_pos:
        # Find "int main() {" or "int main()" followed by "{" and inject after the opening brace
        code = re.sub(
            r'(int\s+main\s*\(\s*\)\s*\{)',
            r'\1\n  x = START_X; y = START_Y;',
            code
        )

    return code


def generate_game_with_controller(
    game_name: str,
    params: dict[str, Any],
    controller_code: str,
) -> str:
    """
    Generate a complete game file from template and controller code.

    This is the new template-based approach where:
    1. Game harness is generated from template with config params
    2. Controller code is appended (keeping main() as the entry point)

    Args:
        game_name: Name of the game (ice_lake, taxi, cliff_walking, blackjack)
        params: Game-specific configuration parameters
        controller_code: The extracted controller code (with void main())

    Returns:
        Complete C source file ready for compilation
    """
    # Generate game harness from template
    game_harness = generate_game(game_name, params)

    # Build complete file: harness + controller
    complete_file = (
        game_harness +
        CONTROLLER_MARKER_START +
        controller_code.rstrip() +
        CONTROLLER_MARKER_END
    )

    return complete_file


def embed_from_template(
    game_name: str,
    params: dict[str, Any],
    synthesis_output: str,
    output_path: Path,
) -> Path:
    """
    Generate a complete game file from template and synthesis output.

    This is the main entry point for the new template-based workflow.

    Args:
        game_name: Name of the game (ice_lake, taxi, cliff_walking, blackjack)
        params: Game-specific configuration parameters
        synthesis_output: Raw output from synthesis tool
        output_path: Path to write the complete game file

    Returns:
        Path to the generated game file
    """
    # Determine if we need to inject start position initialization
    # This is needed for games that use x/y position variables (not blackjack)
    inject_start_pos = game_name.lower() in ("ice_lake", "taxi", "cliff_walking")

    # Extract controller code (keep main() as-is, optionally inject start position)
    controller_code = extract_controller_code(synthesis_output, rename_main=False, inject_start_pos=inject_start_pos)

    # Generate complete file
    complete_file = generate_game_with_controller(game_name, params, controller_code)

    # Write to output
    output_path.write_text(complete_file)

    return output_path


def extract_controller_function(synthesis_output: str) -> str:
    """
    Legacy function - now calls extract_controller_code for full extraction.

    Kept for backward compatibility.
    """
    return extract_controller_code(synthesis_output)


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
