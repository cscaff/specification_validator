# Spec Validator Pipeline

A modular pipeline for synthesizing and validating temporal logic controllers using the Issy synthesis tool.

## Overview

This pipeline takes a **liveliness/safety specification** (the "winning strategy") and combines it with a **boilerplate template** containing environmental assumptions and legal system updates. It then:

1. **Builds** a complete TSLMT specification
2. **Synthesizes** a controller using Issy
3. **Embeds** the controller into a game file
4. **Validates** the controller over N iterations
5. **Reports** win/loss statistics

## Project Structure

```
Spec_Validator/
├── pipeline/                 # Core Python package
│   ├── __init__.py           # Package exports
│   ├── __main__.py           # python -m pipeline support
│   ├── cli.py                # Full CLI with argparse
│   ├── config.py             # YAML config loading
│   ├── spec_builder.py       # Boilerplate + spec merging
│   ├── synthesizer.py        # Issy synthesis wrapper
│   ├── embedder.py           # Controller embedding
│   ├── runner.py             # Game compilation/execution
│   └── pipeline.py           # Main orchestrator
├── boilerplates/             # Template .tslmt files
│   └── frozen_lake.tslmt     # Frozen lake game template
├── configs/                  # YAML configuration files
│   └── frozen_lake.yaml      # Frozen lake configuration
├── games/                    # Game implementations (C++)
│   └── frozen_lake_three_game.cpp
├── constraints/              # Validation schemas (JSON)
│   └── constraints_lake.json
├── third_party/              # Dependencies
│   └── cjson/                # JSON parsing library
├── legacy/                   # Deprecated code
│   ├── spec_validator.py     # Old monolithic pipeline
│   └── src/                  # Old complete .tslmt specs
├── run_pipeline.py           # Convenience CLI wrapper
└── README.md
```

## Requirements

- Python 3.10+
- PyYAML (`pip install pyyaml`)
- Issy synthesis tool (must be in PATH)
- GCC compiler

## Quick Start

### Basic Usage

```bash
# Run with a spec string
python run_pipeline.py frozen_lake "(F (eq playerX goalX && eq playerY goalY));"

# Run with more iterations
python run_pipeline.py frozen_lake "(F (eq playerX goalX && eq playerY goalY));" --runs 50

# Enable debug output (shows synthesis logs and detailed run info)
python run_pipeline.py frozen_lake "(F goal);" --debug
```

### Using a Spec File

```bash
# Create a spec file
echo "(F (eq playerX goalX && eq playerY goalY)) && (G ((avoid_hole1) <-> ((avoid_hole0) || (avoid_hole2))));" > my_spec.txt

# Run with spec file
python run_pipeline.py frozen_lake --spec-file my_spec.txt --runs 20
```

### Full CLI

```bash
python -m pipeline --config configs/frozen_lake.yaml --spec "(F goal);" --runs 20 --debug
```

## Configuration

Configuration files are YAML and define paths and settings for each game:

```yaml
# configs/frozen_lake.yaml
name: frozen_lake

paths:
  boilerplate: boilerplates/frozen_lake.tslmt
  game: games/frozen_lake_three_game.cpp
  constraints: constraints/constraints_lake.json
  cjson_dir: third_party/cjson

synthesis:
  command: issy
  args:
    - --tslmt
    - --solve
    - --synt
    - --info
    - --pruning
    - "1"
    - --accel-attr
    - geom-ext
    - --accel-difficulty
    - easy

execution:
  runs: 20
  timeout: 60

debug: false
```

## Boilerplate Templates

Boilerplate templates contain everything except the winning strategy:

- Variable declarations
- Grid constants and predicates
- Action macros (moveL, moveR, etc.)
- Environmental assumptions (`assume` block)
- Safety guarantees (`always guarantee` block)

The placeholder `{{GUARANTEE_BLOCK}}` is replaced with your input spec:

```tslmt
var Int playerX
var Int playerY

SPECIFICATION

// Constants, predicates, macros...

assume {
  inbounds && ! avoid_hole0 && ! avoid_hole1 && ! avoid_hole2;
}

always guarantee {
  inbounds;
  (moveL || moveR || moveU || moveD || stay);
}

{{GUARANTEE_BLOCK}}
```

## Adding a New Game

1. **Create a boilerplate** in `boilerplates/{name}.tslmt`:
   - Define variables, constants, predicates
   - Add `assume` and `always guarantee` blocks
   - Include `{{GUARANTEE_BLOCK}}` marker at the end

2. **Create a game file** in `games/{name}_game.cpp`:
   - Include constraint loading via cJSON
   - Implement `read_inputs()` function
   - Declare `step_controller()` prototype

3. **Create a constraints file** in `constraints/constraints_{name}.json`:
   ```json
   {
     "bounds": { "xmin": 0, "xmax": 3, "ymin": 0, "ymax": 3 },
     "forbidden": [{ "x": 1, "y": 1 }],
     "max_steps": 1000
   }
   ```

4. **Create a config** in `configs/{name}.yaml`:
   - Point to your boilerplate, game, and constraints
   - Configure synthesis arguments if needed

5. **Run**:
   ```bash
   python run_pipeline.py {name} "your spec here;"
   ```

## Example Output

```
============================================================
  Spec Validator Pipeline
============================================================

Config: frozen_lake
Boilerplate: boilerplates/frozen_lake.tslmt
Game: games/frozen_lake_three_game.cpp
Constraints: constraints/constraints_lake.json

=== Building Specification ===

=== Synthesizing Controller ===
  Controller synthesized successfully (337.1s)

=== Embedding Controller ===
  Controller written to games/frozen_lake_three_game.cpp

=== Running Validation (20 runs) ===
  Run   1/20: PASS (7 steps)
  Run   2/20: PASS (7 steps)
  ...
  Run  20/20: PASS (7 steps)

============================================================
  Results
============================================================

Passed: 20/20 (100.0%)

Step Statistics:
  Average: 7.0
  Min: 7
  Max: 7
```

## Programmatic Usage

```python
from pipeline import Pipeline, load_config

# Load configuration
config = load_config("configs/frozen_lake.yaml")

# Create pipeline
pipeline = Pipeline(config)

# Run with a spec
result = pipeline.run(
    spec="(F (eq playerX goalX && eq playerY goalY));",
    runs_override=10,
    debug_override=True,
)

# Check results
if result.success:
    print(f"Passed: {result.execution.passed}/{result.execution.total_runs}")
else:
    print(f"Failed: {result.error_message}")
```

## Legacy Code

The `legacy/` folder contains the original monolithic pipeline:

- `legacy/spec_validator.py` - Original single-file pipeline
- `legacy/src/` - Complete .tslmt specifications (not templates)

These are preserved for reference but are no longer maintained.
