"""
Configuration module for the Spec Validator Pipeline.

Handles loading and validation of YAML configuration files.

New structure supports multiple objectives, each with multiple configurations.
"""

from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional, Any
import yaml


@dataclass
class PathsConfig:
    """Paths to required files."""
    boilerplate: Path
    game: Path


DEFAULT_SYNTHESIS_ARGS = [
    "--tslmt",
    "--synt",
]


@dataclass
class SynthesisConfig:
    """Configuration for the Issy synthesis tool."""
    command: str = "issy"
    args: list[str] = field(default_factory=lambda: DEFAULT_SYNTHESIS_ARGS.copy())


@dataclass
class GameConfiguration:
    """
    A single game configuration with specific parameters.

    The params dict contains game-specific configuration like:
    - Ice Lake: grid_size, goal, holes
    - Taxi: grid_size, pickup, dropoff, barriers
    - Cliff Walking: grid_size, cliff_min, cliff_max, start_pos, goal_pos
    - Blackjack: (no additional params)
    """
    name: str
    params: dict[str, Any]


@dataclass
class ObjectiveConfig:
    """
    An objective with its guarantee and list of configurations to test.

    The objective is a TSL guarantee string (e.g., "F atGoal && !atHolePos").
    Configurations are tested against this objective to see how well
    a synthesized controller generalizes.
    """
    objective: str
    timeout: int
    configurations: list[GameConfiguration]


@dataclass
class PipelineConfig:
    """Main configuration for the pipeline."""
    name: str
    paths: PathsConfig
    synthesis: SynthesisConfig
    objectives: list[ObjectiveConfig]
    debug: bool = False

    # Root directory for resolving relative paths
    root_dir: Path = field(default_factory=Path.cwd)

    def resolve_path(self, path: Path) -> Path:
        """Resolve a path relative to the root directory."""
        if path.is_absolute():
            return path
        return self.root_dir / path

    @property
    def boilerplate_path(self) -> Path:
        return self.resolve_path(self.paths.boilerplate)

    @property
    def game_path(self) -> Path:
        return self.resolve_path(self.paths.game)


def load_config(config_path: str | Path, root_dir: Optional[Path] = None) -> PipelineConfig:
    """
    Load a pipeline configuration from a YAML file.

    Args:
        config_path: Path to the YAML configuration file
        root_dir: Root directory for resolving relative paths (defaults to config file's parent)

    Returns:
        PipelineConfig instance

    Raises:
        FileNotFoundError: If config file doesn't exist
        ValueError: If config is invalid
    """
    config_path = Path(config_path)

    if not config_path.exists():
        raise FileNotFoundError(f"Configuration file not found: {config_path}")

    with open(config_path, "r") as f:
        raw = yaml.safe_load(f)

    if root_dir is None:
        root_dir = config_path.parent.parent  # Go up from configs/ to project root

    # Parse paths
    paths_raw = raw.get("paths", {})
    paths = PathsConfig(
        boilerplate=Path(paths_raw.get("boilerplate", "")),
        game=Path(paths_raw.get("game", "")),
    )

    # Parse synthesis config
    synth_raw = raw.get("synthesis", {})
    synthesis = SynthesisConfig(
        command=synth_raw.get("command", "issy"),
        args=synth_raw.get("args", DEFAULT_SYNTHESIS_ARGS.copy()),
    )

    # Parse run_configuration (objectives and their configurations)
    objectives = []
    run_config = raw.get("run_configuration", [])

    for obj_raw in run_config:
        # Parse configurations for this objective
        configs = []
        for i, cfg_raw in enumerate(obj_raw.get("configurations", [])):
            config_name = cfg_raw.get("name", f"config_{i+1}")
            # Remove 'name' from params, keep everything else
            params = {k: v for k, v in cfg_raw.items() if k != "name"}
            configs.append(GameConfiguration(name=config_name, params=params))

        objectives.append(ObjectiveConfig(
            objective=obj_raw.get("objective", "").strip(),
            timeout=obj_raw.get("timeout", 1000),
            configurations=configs,
        ))

    return PipelineConfig(
        name=raw.get("name", "unnamed"),
        paths=paths,
        synthesis=synthesis,
        objectives=objectives,
        debug=raw.get("debug", False),
        root_dir=root_dir,
    )


def validate_config(config: PipelineConfig) -> list[str]:
    """
    Validate a configuration, returning a list of errors.

    Returns:
        List of error messages (empty if valid)
    """
    errors = []

    # Note: boilerplate and game paths are no longer required since we use
    # template-based generation (spec_generator.py and game_templates.py)

    if not config.objectives:
        errors.append("No objectives defined in run_configuration")

    for i, obj in enumerate(config.objectives):
        if not obj.objective:
            errors.append(f"Objective {i+1} has no objective string")
        if not obj.configurations:
            errors.append(f"Objective '{obj.objective}' has no configurations")

    return errors
