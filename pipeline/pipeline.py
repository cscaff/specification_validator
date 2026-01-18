"""
Pipeline orchestrator for the Spec Validator Pipeline.

Coordinates all pipeline stages: spec generation, synthesis, embedding, and execution.

New flow:
1. For each objective:
   - For each configuration under that objective:
     a. Generate TSLMT spec with configuration parameters
     b. Synthesize a controller
     c. Embed controller into game file
     d. Run game and validate (goal reached within timeout, no hazard violations)
     e. Record pass/fail
   - Calculate score for this objective
2. Output summary with scores per objective
"""

from dataclasses import dataclass, field
from pathlib import Path
import shutil
import tempfile
from typing import Optional

from .config import PipelineConfig, ObjectiveConfig, GameConfiguration, validate_config
from .spec_generator import generate_spec
from .synthesizer import synthesize, SynthesisResult
from .embedder import embed_from_template
from .runner import run_game_once, build_game, RunResult


@dataclass
class ConfigurationResult:
    """Result of running a single configuration."""
    config_name: str
    success: bool
    steps: Optional[int] = None
    error_message: Optional[str] = None
    synthesis_time: Optional[float] = None


@dataclass
class ObjectiveResult:
    """Result of testing an objective across all its configurations."""
    objective: str
    configurations: list[ConfigurationResult] = field(default_factory=list)

    @property
    def passed(self) -> int:
        return sum(1 for c in self.configurations if c.success)

    @property
    def total(self) -> int:
        return len(self.configurations)

    @property
    def score(self) -> str:
        if self.total == 0:
            return "0/0 (0%)"
        pct = (self.passed / self.total) * 100
        return f"{self.passed}/{self.total} ({pct:.1f}%)"


@dataclass
class PipelineResult:
    """Complete result of a pipeline run."""
    config_name: str
    objectives: list[ObjectiveResult] = field(default_factory=list)
    success: bool = True
    error_message: Optional[str] = None


class Pipeline:
    """
    Main pipeline orchestrator.

    Usage:
        config = load_config("configs/ice_lake.yaml")
        pipeline = Pipeline(config)
        result = pipeline.run()
    """

    def __init__(self, config: PipelineConfig):
        self.config = config

    def run(self, debug_override: Optional[bool] = None) -> PipelineResult:
        """
        Run the complete pipeline with all objectives and configurations.

        Args:
            debug_override: Override the debug setting from config

        Returns:
            PipelineResult with all objective results and scores
        """
        debug = debug_override if debug_override is not None else self.config.debug

        # Validate config
        errors = validate_config(self.config)
        if errors:
            return PipelineResult(
                config_name=self.config.name,
                success=False,
                error_message=f"Configuration errors: {'; '.join(errors)}",
            )

        result = PipelineResult(config_name=self.config.name)

        print("=" * 60)
        print(f"  Spec Validator Pipeline - {self.config.name}")
        print("=" * 60)
        print(f"\nUsing template-based generation for specs and games")
        print(f"Objectives: {len(self.config.objectives)}")

        # Process each objective
        for obj_idx, objective in enumerate(self.config.objectives):
            obj_result = self._run_objective(objective, obj_idx + 1, debug)
            result.objectives.append(obj_result)

        # Print summary
        self._print_summary(result)

        return result

    def _run_objective(
        self,
        objective: ObjectiveConfig,
        obj_num: int,
        debug: bool,
    ) -> ObjectiveResult:
        """Run all configurations for a single objective."""
        print(f"\n{'=' * 60}")
        print(f"  Objective {obj_num}: {objective.objective}")
        print(f"  Configurations: {len(objective.configurations)}")
        print(f"  Timeout: {objective.timeout} steps")
        print("=" * 60)

        obj_result = ObjectiveResult(objective=objective.objective)

        for cfg_idx, config in enumerate(objective.configurations):
            cfg_result = self._run_configuration(
                config,
                objective.objective,
                objective.timeout,
                cfg_idx + 1,
                len(objective.configurations),
                debug,
            )
            obj_result.configurations.append(cfg_result)

        print(f"\n  Objective Score: {obj_result.score}")
        return obj_result

    def _run_configuration(
        self,
        config: GameConfiguration,
        objective: str,
        timeout_steps: int,
        cfg_num: int,
        total_cfgs: int,
        debug: bool,
    ) -> ConfigurationResult:
        """Run a single configuration: generate spec, synthesize, embed, run."""
        print(f"\n  --- Config {cfg_num}/{total_cfgs}: {config.name} ---")

        if debug:
            print(f"  Parameters: {config.params}")

        try:
            # Step 1: Generate spec
            print("  [1/4] Generating spec...")
            spec = generate_spec(self.config.name, config.params, objective)

            if debug:
                print("  Generated spec preview:")
                for line in spec.split('\n')[:10]:
                    print(f"    {line}")
                print("    ...")

            # Save debug copy of spec to spec_output folder
            spec_output_dir = self.config.root_dir / "spec_output"
            spec_output_dir.mkdir(exist_ok=True)
            debug_name = config.name.replace(" ", "_").replace("/", "_")
            spec_debug_file = spec_output_dir / f"{self.config.name}_{debug_name}_debug.tslmt"
            spec_debug_file.write_text(spec)
            print(f"  Spec saved to: {spec_debug_file}")

            # Write spec to temp file
            with tempfile.NamedTemporaryFile(
                mode='w', suffix='.tslmt', delete=False
            ) as f:
                f.write(spec)
                spec_path = Path(f.name)

            # Step 2: Synthesize
            print("  [2/4] Synthesizing controller...")
            synthesis_result = synthesize(
                spec_path,
                command=self.config.synthesis.command,
                args=self.config.synthesis.args,
                debug=debug,
            )

            # Clean up spec file
            spec_path.unlink()

            if not synthesis_result.success:
                print(f"  FAIL: Synthesis failed - {synthesis_result.error_message}")
                return ConfigurationResult(
                    config_name=config.name,
                    success=False,
                    error_message=f"Synthesis failed: {synthesis_result.error_message}",
                    synthesis_time=synthesis_result.duration,
                )

            print(f"  Synthesis complete ({synthesis_result.duration:.1f}s)")

            # Step 3: Generate game from template and embed controller
            print("  [3/4] Generating game and embedding controller...")

            # Create temp directory for game build
            with tempfile.TemporaryDirectory() as tmp:
                tmp_dir = Path(tmp)
                game_file = tmp_dir / "game.c"

                # Generate complete game file from template + controller
                embed_from_template(
                    game_name=self.config.name,
                    params=config.params,
                    synthesis_output=synthesis_result.controller_code,
                    output_path=game_file,
                )

                # Save a debug copy to games folder
                debug_name = config.name.replace(" ", "_").replace("/", "_")
                debug_file = self.config.root_dir / "games" / f"{self.config.name}_{debug_name}_debug.c"
                shutil.copy(game_file, debug_file)
                print(f"  Debug copy saved to: {debug_file}")

                # Step 4: Build and run
                print("  [4/4] Building and running...")

                # Build game
                if not build_game(tmp_dir, debug=debug):
                    print("  FAIL: Build failed")
                    return ConfigurationResult(
                        config_name=config.name,
                        success=False,
                        error_message="Build failed",
                        synthesis_time=synthesis_result.duration,
                    )

                # Run game
                run_result = run_game_once(
                    tmp_dir,
                    timeout_steps=timeout_steps,
                    config_params=config.params,
                    debug=debug,
                )

                if run_result.success:
                    print(f"  PASS: Goal reached in {run_result.steps} steps")
                else:
                    print(f"  FAIL: {run_result.error_message}")

                return ConfigurationResult(
                    config_name=config.name,
                    success=run_result.success,
                    steps=run_result.steps,
                    error_message=run_result.error_message if not run_result.success else None,
                    synthesis_time=synthesis_result.duration,
                )

        except Exception as e:
            print(f"  FAIL: {str(e)}")
            return ConfigurationResult(
                config_name=config.name,
                success=False,
                error_message=str(e),
            )

    def _print_summary(self, result: PipelineResult) -> None:
        """Print final summary of all objectives."""
        print("\n" + "=" * 60)
        print("  SUMMARY")
        print("=" * 60)

        for obj_result in result.objectives:
            print(f"\n  {obj_result.objective}")
            for cfg in obj_result.configurations:
                status = "PASS" if cfg.success else "FAIL"
                steps_str = f" ({cfg.steps} steps)" if cfg.steps else ""
                err_str = f" - {cfg.error_message}" if cfg.error_message else ""
                print(f"    {cfg.config_name}: {status}{steps_str}{err_str}")
            print(f"  Score: {obj_result.score}")

        print("\n" + "=" * 60)
        total_passed = sum(obj.passed for obj in result.objectives)
        total_configs = sum(obj.total for obj in result.objectives)
        if total_configs > 0:
            total_pct = (total_passed / total_configs) * 100
            print(f"  TOTAL: {total_passed}/{total_configs} ({total_pct:.1f}%)")
        print("=" * 60)
