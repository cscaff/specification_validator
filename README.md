# Spec Validator Pipeline

A modular pipeline for synthesizing and validating temporal logic controllers using the Issy synthesis tool. Supports multi-objective testing across multiple configurations to evaluate how well synthesized controllers generalize.

## Overview

This pipeline evaluates TSL (Temporal Stream Logic) objectives across different game configurations. For each objective, it:

1. **Generates** a complete TSLMT specification from configuration parameters
2. **Synthesizes** a controller using Issy
3. **Generates** a game harness from templates with embedded controller
4. **Validates** by compiling and running the game to check goal achievement
5. **Scores** each objective by the number of configurations that pass

This allows testing how well a given TSL objective generalizes across different scenarios (grid sizes, goal positions, obstacle layouts, etc.).

## Project Structure

```
Spec_Validator/
├── pipeline/                 # Core Python package
│   ├── __init__.py           # Package exports
│   ├── config.py             # YAML config loading
│   ├── spec_generator.py     # Dynamic TSLMT spec generation
│   ├── game_templates.py     # Dynamic C game harness generation
│   ├── synthesizer.py        # Issy synthesis wrapper
│   ├── embedder.py           # Controller extraction and embedding
│   ├── runner.py             # Game compilation/execution
│   └── pipeline.py           # Main orchestrator
├── configs/                  # YAML configuration files
│   ├── ice_lake.yaml         # Ice lake navigation
│   ├── taxi.yaml             # Taxi pickup/dropoff
│   ├── cliff_walking.yaml    # Cliff avoidance navigation
│   └── blackjack.yaml        # Blackjack strategy
├── games/                    # Debug output for generated game files
├── spec_output/              # Debug output for generated spec files
├── boilerplates/             # Legacy TSLMT templates (not used)
├── legacy/                   # Deprecated code and old approach
├── run_pipeline.py           # Main entry point
└── README.md
```

## Requirements

- Python 3.10+
- PyYAML (`pip install pyyaml`)
- Issy synthesis tool (path configured in YAML)
- GCC compiler

## Quick Start

### Basic Usage

```bash
# Run all objectives and configurations for a game
python run_pipeline.py ice_lake

# Enable debug output
python run_pipeline.py taxi --debug

# List available configurations
python run_pipeline.py --list-configs
```

### Example Output

```
============================================================
  Spec Validator Pipeline - ice_lake
============================================================

Using template-based generation for specs and games
Objectives: 2

============================================================
  Objective 1: F atGoal && G !atHolePos
  Configurations: 3
  Timeout: 1000 steps
============================================================

  --- Config 1/3: 4x4 single hole ---
  [1/4] Generating spec...
  Spec saved to: spec_output/ice_lake_4x4_single_hole_debug.tslmt
  [2/4] Synthesizing controller...
  Synthesis complete (12.3s)
  [3/4] Generating game and embedding controller...
  Debug copy saved to: games/ice_lake_4x4_single_hole_debug.c
  [4/4] Building and running...
  PASS: Goal reached in 7 steps

  --- Config 2/3: 5x5 two holes ---
  ...
  PASS: Goal reached in 12 steps

  --- Config 3/3: 4x4 corner holes ---
  ...
  PASS: Goal reached in 8 steps

  Objective Score: 3/3 (100.0%)

============================================================
  SUMMARY
============================================================

  F atGoal && G !atHolePos
    4x4 single hole: PASS (7 steps)
    5x5 two holes: PASS (12 steps)
    4x4 corner holes: PASS (8 steps)
  Score: 3/3 (100.0%)

============================================================
  TOTAL: 3/3 (100.0%)
============================================================
```

## Configuration

Configuration files define objectives and their test configurations:

```yaml
# configs/ice_lake.yaml
name: ice_lake

paths:
  boilerplate: boilerplates/ice_lake.tslmt  # Legacy, not used
  game: games/ice_lake_game.c               # Legacy, not used

synthesis:
  command: /path/to/issy
  args:
    - "--tslmt"
    - "--synt"

debug: false

run_configuration:
  # Objective 1: Reach goal while avoiding holes
  - objective: "F atGoal && G !atHolePos"
    timeout: 1000
    configurations:
      - name: "4x4 single hole"
        grid_size: 4
        goal: {x: 3, y: 3}
        holes:
          - {x: 1, y: 1}

      - name: "5x5 two holes"
        grid_size: 5
        goal: {x: 4, y: 4}
        holes:
          - {x: 1, y: 1}
          - {x: 2, y: 3}

  # Objective 2: Just reach goal (relaxed)
  - objective: "F atGoal"
    timeout: 1000
    configurations:
      - name: "5x5 relaxed"
        grid_size: 5
        goal: {x: 4, y: 4}
        holes:
          - {x: 1, y: 1}
```

## Supported Games

### Ice Lake
Robot navigates a grid avoiding holes to reach a goal.
- **Parameters**: `grid_size`, `goal`, `holes`
- **Objectives**: `F atGoal`, `F atGoal && G !atHolePos`

### Taxi
Taxi navigates to pick up a passenger and deliver to destination.
- **Parameters**: `grid_size`, `pickup`, `dropoff`, `barriers`
- **Objectives**: `F delivered`

### Cliff Walking
Robot navigates a grid avoiding a cliff region to reach a goal.
- **Parameters**: `grid_size`, `grid_rows`, `cliff_min`, `cliff_max`, `start_pos`, `goal_pos`
- **Objectives**: `F atGoal && G !onCliff`, `F atGoal`

### Blackjack
Controller makes hit/stand decisions based on basic strategy.
- **Parameters**: None (decision logic is fixed)
- **Objectives**: `shouldHitCondition -> hit && !shouldHitCondition -> stand`

## Architecture

### Template-Based Generation

Both specs and games are generated dynamically from Python templates:

1. **Spec Templates** (`pipeline/spec_generator.py`): Generate complete TSLMT specifications with configuration parameters (grid sizes, goal positions, hole/barrier locations) baked in.

2. **Game Templates** (`pipeline/game_templates.py`): Generate C game harnesses with:
   - Configuration-specific constants (grid size, goals, obstacles)
   - Validation logic (`read_inputs()` function)
   - Proper exit codes (0=success, 1+=failure)

### Controller Embedding

The embedder (`pipeline/embedder.py`) processes synthesis output:
1. Extracts C code from Issy output
2. Removes the `read_inputs()` stub (game template provides this)
3. Converts `void main()` to `int main()` for C standard compliance
4. Combines game template + controller into a single compilable file

### Debug Output

For each configuration, debug files are saved:
- `spec_output/{game}_{config}_debug.tslmt` - Generated specification
- `games/{game}_{config}_debug.c` - Complete game with embedded controller

These files persist after the pipeline run for debugging.

## Adding a New Game

1. **Add a spec generator** in `pipeline/spec_generator.py`:
   ```python
   def generate_mygame_spec(params: dict[str, Any], objective: str) -> str:
       grid_size = params.get("grid_size", 4)
       # ... build spec string with variables, predicates, guarantees ...
       return f'''var Int x
   var Int y
   var Bool atGoal

   SPECIFICATION

   /* ... predicates and constants ... */

   assume {{
       /* environment assumptions */
   }}

   guarantee {{
       /* controller guarantees */
       {objective};
   }}
   '''

   # Register in SPEC_GENERATORS dict
   SPEC_GENERATORS["mygame"] = generate_mygame_spec
   ```

2. **Add a game template** in `pipeline/game_templates.py`:
   ```python
   def generate_mygame_game(params: dict[str, Any]) -> str:
       grid_size = params.get("grid_size", 4)
       # ... extract config params ...

       return f'''
   #include <stdio.h>
   #include <stdlib.h>
   #include <stdbool.h>

   #define GRID_SIZE {grid_size}
   /* ... other config constants ... */

   static int step_count = 0;

   /* Forward declarations for controller variables */
   extern bool atGoal;
   extern int x;
   extern int y;

   void read_inputs(void) {{
       step_count++;

       /* Check win/lose conditions */
       if (at_goal(x, y)) {{
           printf("SUCCESS: Goal reached in %d steps\\n", step_count);
           exit(0);
       }}
       if (hit_obstacle(x, y)) {{
           printf("FAIL: Hit obstacle at (%d,%d)\\n", x, y);
           exit(1);
       }}
       if (step_count >= MAX_STEPS) {{
           printf("FAIL: Step timeout\\n");
           exit(3);
       }}
   }}
   '''

   # Register in GAME_GENERATORS dict
   GAME_GENERATORS["mygame"] = generate_mygame_game
   ```

3. **Create a config** in `configs/mygame.yaml`:
   ```yaml
   name: mygame

   paths:
     boilerplate: boilerplates/mygame.tslmt  # Not used, can be placeholder
     game: games/mygame_game.c               # Not used, can be placeholder

   synthesis:
     command: /path/to/issy
     args:
       - "--tslmt"
       - "--synt"

   run_configuration:
     - objective: "F atGoal"
       timeout: 1000
       configurations:
         - name: "basic"
           grid_size: 4
           goal: {x: 3, y: 3}
   ```

4. **Run**:
   ```bash
   python run_pipeline.py mygame
   ```

## Programmatic Usage

```python
from pipeline import Pipeline, load_config

# Load configuration
config = load_config("configs/ice_lake.yaml")

# Create and run pipeline
pipeline = Pipeline(config)
result = pipeline.run(debug_override=True)

# Check results
for obj_result in result.objectives:
    print(f"{obj_result.objective}: {obj_result.score}")
    for cfg in obj_result.configurations:
        status = "PASS" if cfg.success else "FAIL"
        print(f"  {cfg.config_name}: {status}")
```

## How It Works

```
┌─────────────────────────────────────────────────────────────────┐
│                        Pipeline Flow                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  YAML Config                                                    │
│      │                                                          │
│      ▼                                                          │
│  ┌─────────────────┐                                            │
│  │ Spec Generator  │──► spec_output/{game}_{config}_debug.tslmt │
│  └────────┬────────┘                                            │
│           │                                                     │
│           ▼                                                     │
│  ┌─────────────────┐                                            │
│  │ Issy Synthesis  │                                            │
│  └────────┬────────┘                                            │
│           │                                                     │
│           ▼                                                     │
│  ┌─────────────────┐                                            │
│  │    Embedder     │                                            │
│  │  - Extract C    │                                            │
│  │  - Remove stub  │                                            │
│  │  - Fix main()   │                                            │
│  └────────┬────────┘                                            │
│           │                                                     │
│           ▼                                                     │
│  ┌─────────────────┐                                            │
│  │ Game Generator  │──► games/{game}_{config}_debug.c           │
│  └────────┬────────┘                                            │
│           │                                                     │
│           ▼                                                     │
│  ┌─────────────────┐                                            │
│  │  GCC Compile    │                                            │
│  └────────┬────────┘                                            │
│           │                                                     │
│           ▼                                                     │
│  ┌─────────────────┐                                            │
│  │   Run Binary    │──► Exit 0 = PASS, Exit 1+ = FAIL           │
│  └─────────────────┘                                            │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

1. **Config Loading**: Parses YAML to extract objectives and their configurations
2. **Spec Generation**: Dynamically generates TSLMT spec with parameters baked in
3. **Synthesis**: Calls Issy to synthesize a controller from the spec
4. **Embedding**: Extracts C code, removes stubs, fixes `main()` signature
5. **Game Generation**: Creates complete C file from template + controller
6. **Compilation**: Uses GCC to compile the game
7. **Validation**: Runs the binary; exit code 0 = pass, non-zero = fail
8. **Scoring**: Counts passed configurations per objective

## Legacy Code

The `legacy/` folder contains the previous pipeline approach:

- `legacy/spec_validator.py` - Original monolithic pipeline
- `legacy/boilerplates/` - Template TSLMT files with placeholders
- `legacy/constraints/` - JSON constraint files
- `legacy/spec_builder.py` - Boilerplate + spec merging
- `legacy/cli.py` - Old CLI interface

The new approach generates both specs and games dynamically from Python templates, with configuration embedded directly in YAML files.
