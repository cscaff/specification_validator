"""
Spec Builder module for the Spec Validator Pipeline.

Handles combining boilerplate templates with input specifications.
"""

from pathlib import Path
import tempfile
from typing import Optional


GUARANTEE_MARKER = "{{GUARANTEE_BLOCK}}"


def build_spec(
    boilerplate_path: Path,
    input_spec: str,
    output_path: Optional[Path] = None,
    debug: bool = False,
) -> Path:
    """
    Build a complete specification by combining a boilerplate with an input spec.

    Args:
        boilerplate_path: Path to the boilerplate .tslmt file
        input_spec: The liveliness/safety specification to inject
        output_path: Optional path for the output file (creates temp file if None)

    Returns:
        Path to the generated specification file

    Raises:
        FileNotFoundError: If boilerplate doesn't exist
        ValueError: If boilerplate doesn't contain the marker
    """
    if not boilerplate_path.exists():
        raise FileNotFoundError(f"Boilerplate not found: {boilerplate_path}")

    # Read boilerplate
    boilerplate_content = boilerplate_path.read_text()

    # Build guarantee block
    # Clean up input spec (remove trailing semicolon if present, we'll add it)
    clean_spec = input_spec.strip()
    if clean_spec.endswith(";"):
        clean_spec = clean_spec[:-1]

    guarantee_block = f"guarantee {{\n  {clean_spec};\n}}"

    # Check for marker and replace, or append at end
    if GUARANTEE_MARKER in boilerplate_content:
        full_spec = boilerplate_content.replace(GUARANTEE_MARKER, guarantee_block)
    else:
        # Append guarantee block at the end
        full_spec = boilerplate_content.rstrip() + "\n\n" + guarantee_block + "\n"

    # Write to output file
    if output_path is None:
        # Create temp file that won't be deleted automatically
        fd, temp_path = tempfile.mkstemp(suffix=".tslmt", prefix="spec_")
        output_path = Path(temp_path)
        import os
        os.close(fd)

    # Print Spec to Terminal
    if debug:
        print("Generated Specification:\n")
        print(full_spec)
        print("\nEnd of Specification\n")

    output_path.write_text(full_spec)

    return output_path


def read_spec_from_file(spec_file: Path) -> str:
    """
    Read a specification from a file.

    Args:
        spec_file: Path to the file containing the spec

    Returns:
        The specification string
    """
    if not spec_file.exists():
        raise FileNotFoundError(f"Spec file not found: {spec_file}")

    return spec_file.read_text().strip()
