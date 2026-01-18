"""
CLI module for the Spec Validator Pipeline.

Provides command-line interface for running the pipeline.
"""

import argparse
from pathlib import Path
import sys

from .config import load_config
from .spec_builder import read_spec_from_file
from .pipeline import Pipeline


def create_parser() -> argparse.ArgumentParser:
    """Create the argument parser."""
    parser = argparse.ArgumentParser(
        description="Spec Validator Pipeline - Synthesize and validate temporal logic controllers",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Using spec string
  python -m pipeline --config configs/frozen_lake.yaml \\
    --spec "(F (eq playerX goalX && eq playerY goalY));"

  # Using spec file
  python -m pipeline --config configs/frozen_lake.yaml \\
    --spec-file specs/my_strategy.txt

  # Override runs and enable debug
  python -m pipeline --config configs/frozen_lake.yaml \\
    --spec "(F goal);" --runs 50 --debug
""",
    )

    parser.add_argument(
        "--config",
        type=str,
        required=True,
        help="Path to YAML configuration file",
    )

    spec_group = parser.add_mutually_exclusive_group(required=True)
    spec_group.add_argument(
        "--spec",
        type=str,
        help="The liveliness/safety specification (string)",
    )
    spec_group.add_argument(
        "--spec-file",
        type=str,
        help="Path to file containing the specification",
    )

    parser.add_argument(
        "--runs",
        type=int,
        default=None,
        help="Number of validation runs (overrides config)",
    )

    parser.add_argument(
        "--debug",
        action="store_true",
        help="Enable debug output (synthesis logs, run details)",
    )

    return parser


def main(args: list[str] = None) -> int:
    """
    Main entry point for the CLI.

    Args:
        args: Command line arguments (defaults to sys.argv[1:])

    Returns:
        Exit code (0 for success, 1 for failure)
    """
    parser = create_parser()
    parsed = parser.parse_args(args)

    # Print header
    print("=" * 50)
    print("  Spec Validator Pipeline")
    print("=" * 50)

    # Load config
    try:
        config = load_config(parsed.config)
        print(f"\nConfig: {config.name}")
        print(f"Boilerplate: {config.boilerplate_path}")
    except Exception as e:
        print(f"\nError loading config: {e}", file=sys.stderr)
        return 1

    # Get spec
    if parsed.spec:
        spec = parsed.spec
    else:
        try:
            spec = read_spec_from_file(Path(parsed.spec_file))
        except Exception as e:
            print(f"\nError reading spec file: {e}", file=sys.stderr)
            return 1

    # Determine debug mode
    debug = parsed.debug or config.debug

    # Run pipeline
    pipeline = Pipeline(config)
    result = pipeline.run(
        spec,
        runs_override=parsed.runs,
        debug_override=parsed.debug if parsed.debug else None,
    )

    # Print results
    print("\n" + "=" * 50)
    print("  Results")
    print("=" * 50)

    if result.success:
        print(f"\n{result.summary}")

        if result.execution:
            # Additional stats
            if result.execution.runs:
                steps = [r.steps for r in result.execution.runs if r.steps is not None]
                if steps:
                    avg_steps = sum(steps) / len(steps)
                    print(f"Average steps: {avg_steps:.1f}")
                    print(f"Min steps: {min(steps)}")
                    print(f"Max steps: {max(steps)}")

        return 0
    else:
        print(f"\nPipeline failed: {result.error_message}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
