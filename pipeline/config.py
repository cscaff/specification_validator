"""
Configuration module for the Spec Validator Pipeline.

Handles loading and validation of YAML configuration files.
"""

from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional
import yaml


@dataclass
class PathsConfig:
    """Paths to required files."""
    boilerplate: Path
    game: Path
    constraints: Path
    cjson_dir: Path = field(default_factory=lambda: Path("third_party/cjson"))


DEFAULT_SYNTHESIS_ARGS = [
    "--tslmt", "--solve", "--synt", "--info",
    "--pruning", "1",
    "--accel-attr", "geom-ext",
    "--accel-difficulty", "easy"
]


@dataclass
class SynthesisConfig:
    """Configuration for the Issy synthesis tool."""
    command: str = "issy"
    args: list[str] = field(default_factory=lambda: DEFAULT_SYNTHESIS_ARGS.copy())


@dataclass
class ExecutionConfig:
    """Configuration for game execution."""
    runs: int = 20
    timeout: int = 60


@dataclass
class PipelineConfig:
    """Main configuration for the pipeline."""
    name: str
    paths: PathsConfig
    synthesis: SynthesisConfig = field(default_factory=SynthesisConfig)
    execution: ExecutionConfig = field(default_factory=ExecutionConfig)
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

    @property
    def constraints_path(self) -> Path:
        return self.resolve_path(self.paths.constraints)

    @property
    def cjson_path(self) -> Path:
        return self.resolve_path(self.paths.cjson_dir)


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
        constraints=Path(paths_raw.get("constraints", "")),
        cjson_dir=Path(paths_raw.get("cjson_dir", "third_party/cjson")),
    )

    # Parse synthesis config
    synth_raw = raw.get("synthesis", {})
    synthesis = SynthesisConfig(
        command=synth_raw.get("command", "issy"),
        args=synth_raw.get("args", DEFAULT_SYNTHESIS_ARGS.copy()),
    )

    # Parse execution config
    exec_raw = raw.get("execution", {})
    execution = ExecutionConfig(
        runs=exec_raw.get("runs", 20),
        timeout=exec_raw.get("timeout", 60),
    )

    return PipelineConfig(
        name=raw.get("name", "unnamed"),
        paths=paths,
        synthesis=synthesis,
        execution=execution,
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

    if not config.boilerplate_path.exists():
        errors.append(f"Boilerplate file not found: {config.boilerplate_path}")

    if not config.game_path.exists():
        errors.append(f"Game file not found: {config.game_path}")

    if not config.constraints_path.exists():
        errors.append(f"Constraints file not found: {config.constraints_path}")

    if not config.cjson_path.exists():
        errors.append(f"cJSON directory not found: {config.cjson_path}")

    if config.execution.runs < 1:
        errors.append(f"Number of runs must be >= 1, got {config.execution.runs}")

    return errors
