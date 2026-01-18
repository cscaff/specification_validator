#!/usr/bin/env python3
"""
Convenience wrapper for the Spec Validator Pipeline.

Usage:
    python run_pipeline.py ice_lake
    python run_pipeline.py taxi --debug
    python run_pipeline.py --help

This runs all objectives and configurations defined in configs/{name}.yaml
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
        description="Spec Validator Pipeline - Multi-Objective Runner",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python run_pipeline.py ice_lake
  python run_pipeline.py taxi --debug
  python run_pipeline.py blackjack
  python run_pipeline.py cliff_walking

The pipeline will:
  1. Read objectives and configurations from configs/{name}.yaml
  2. For each objective, test all its configurations:
     - Generate TSLMT spec with configuration parameters
     - Synthesize a controller
     - Embed controller into game file
     - Run validation
  3. Calculate scores (passed/total) per objective
""",
    )

    parser.add_argument(
        "config_name",
        type=str,
        help="Name of the configuration (looks for configs/{name}.yaml)",
    )

    parser.add_argument(
        "--debug",
        action="store_true",
        help="Enable debug output",
    )

    parser.add_argument(
        "--list-configs",
        action="store_true",
        help="List available configurations and exit",
    )

    args = parser.parse_args()

    # List configs if requested
    if args.list_configs:
        configs_dir = project_root / "configs"
        if configs_dir.exists():
            print("Available configurations:")
            for f in sorted(configs_dir.glob("*.yaml")):
                print(f"  - {f.stem}")
        else:
            print("No configs directory found")
        return 0

    # Find config file
    config_path = project_root / "configs" / f"{args.config_name}.yaml"
    if not config_path.exists():
        print(f"Error: Config file not found: {config_path}", file=sys.stderr)
        print(f"\nAvailable configs:", file=sys.stderr)
        configs_dir = project_root / "configs"
        if configs_dir.exists():
            for f in sorted(configs_dir.glob("*.yaml")):
                print(f"  - {f.stem}", file=sys.stderr)
        return 1

    # Load config
    try:
        config = load_config(config_path, root_dir=project_root)
    except Exception as e:
        print(f"Error loading config: {e}", file=sys.stderr)
        return 1

    # Run pipeline
    pipeline = Pipeline(config)
    result = pipeline.run(debug_override=args.debug if args.debug else None)

    # Return exit code based on overall success
    if result.error_message:
        print(f"\nPipeline error: {result.error_message}", file=sys.stderr)
        return 1

    # Calculate overall success rate
    total_passed = sum(obj.passed for obj in result.objectives)
    total_configs = sum(obj.total for obj in result.objectives)

    if total_configs == 0:
        print("\nNo configurations were run", file=sys.stderr)
        return 1

    # Return 0 if at least one configuration passed
    if total_passed > 0:
        return 0
    else:
        return 1


if __name__ == "__main__":
    sys.exit(main())
