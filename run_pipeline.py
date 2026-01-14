#!/usr/bin/env python3
"""
Convenience wrapper for the Spec Validator Pipeline.

Usage:
    python run_pipeline.py frozen_lake "(F (eq playerX goalX && eq playerY goalY));"
    python run_pipeline.py frozen_lake "(F goal);" --runs 50 --debug
    python run_pipeline.py --help

This is a simpler interface that assumes configs are in configs/{name}.yaml
"""

import argparse
from pathlib import Path
import sys

# Add the project root to the path
project_root = Path(__file__).parent
sys.path.insert(0, str(project_root))

from pipeline import Pipeline, load_config


def main():
    parser = argparse.ArgumentParser(
        description="Spec Validator Pipeline - Quick Runner",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python run_pipeline.py frozen_lake "(F (eq playerX goalX && eq playerY goalY));"
  python run_pipeline.py frozen_lake "(F goal);" --runs 50 --debug
  python run_pipeline.py frozen_lake --spec-file my_spec.txt
""",
    )

    parser.add_argument(
        "config_name",
        type=str,
        help="Name of the configuration (looks for configs/{name}.yaml)",
    )

    spec_group = parser.add_mutually_exclusive_group(required=True)
    spec_group.add_argument(
        "spec",
        nargs="?",
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
        help="Enable debug output",
    )

    args = parser.parse_args()

    # Find config file
    config_path = project_root / "configs" / f"{args.config_name}.yaml"
    if not config_path.exists():
        print(f"Error: Config file not found: {config_path}", file=sys.stderr)
        print(f"Available configs in configs/:", file=sys.stderr)
        configs_dir = project_root / "configs"
        if configs_dir.exists():
            for f in configs_dir.glob("*.yaml"):
                print(f"  - {f.stem}", file=sys.stderr)
        return 1

    # Load config
    try:
        config = load_config(config_path, root_dir=project_root)
    except Exception as e:
        print(f"Error loading config: {e}", file=sys.stderr)
        return 1

    # Get spec
    if args.spec:
        spec = args.spec
    elif args.spec_file:
        try:
            spec = Path(args.spec_file).read_text().strip()
        except Exception as e:
            print(f"Error reading spec file: {e}", file=sys.stderr)
            return 1
    else:
        print("Error: No spec provided", file=sys.stderr)
        return 1

    # Print header
    print("=" * 60)
    print("  Spec Validator Pipeline")
    print("=" * 60)
    print(f"\nConfig: {config.name}")
    print(f"Boilerplate: {config.boilerplate_path}")
    print(f"Game: {config.game_path}")
    print(f"Constraints: {config.constraints_path}")

    # Run pipeline
    pipeline = Pipeline(config)
    result = pipeline.run(
        spec,
        runs_override=args.runs,
        debug_override=args.debug if args.debug else None,
    )

    # Print results
    print("\n" + "=" * 60)
    print("  Results")
    print("=" * 60)

    if result.success:
        print(f"\n{result.summary}")

        if result.execution:
            steps = [r.steps for r in result.execution.runs if r.steps is not None]
            if steps:
                avg_steps = sum(steps) / len(steps)
                print(f"\nStep Statistics:")
                print(f"  Average: {avg_steps:.1f}")
                print(f"  Min: {min(steps)}")
                print(f"  Max: {max(steps)}")

        return 0
    else:
        print(f"\nPipeline failed: {result.error_message}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
