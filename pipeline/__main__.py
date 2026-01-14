"""
Entry point for running the pipeline as a module.

Usage: python -m pipeline --config configs/frozen_lake.yaml --spec "..."
"""

import sys
from .cli import main

if __name__ == "__main__":
    sys.exit(main())
