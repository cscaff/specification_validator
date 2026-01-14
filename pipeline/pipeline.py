"""
Pipeline orchestrator for the Spec Validator Pipeline.

Coordinates all pipeline stages: spec building, synthesis, embedding, and execution.
"""

from dataclasses import dataclass
from pathlib import Path
import tempfile
from typing import Optional

from .config import PipelineConfig, validate_config
from .spec_builder import build_spec
from .synthesizer import synthesize, SynthesisResult
from .embedder import embed_from_synthesis
from .runner import run_game, ExecutionResult


@dataclass
class PipelineResult:
    """Complete result of a pipeline run."""
    config_name: str
    spec: str
    synthesis: Optional[SynthesisResult]
    execution: Optional[ExecutionResult]
    success: bool
    error_message: Optional[str] = None

    @property
    def summary(self) -> str:
        """Generate a summary string."""
        if not self.success:
            return f"FAILED: {self.error_message}"

        if self.execution:
            return f"Passed: {self.execution.passed}/{self.execution.total_runs} ({self.execution.pass_percentage})"
        return "No execution results"


class Pipeline:
    """
    Main pipeline orchestrator.

    Usage:
        config = load_config("configs/frozen_lake.yaml")
        pipeline = Pipeline(config)
        result = pipeline.run("(F (eq playerX goalX && eq playerY goalY));")
    """

    def __init__(self, config: PipelineConfig):
        self.config = config
        self._temp_files: list[Path] = []

    def run(
        self,
        spec: str,
        runs_override: Optional[int] = None,
        debug_override: Optional[bool] = None,
    ) -> PipelineResult:
        """
        Run the complete pipeline with the given specification.

        Args:
            spec: The liveliness/safety specification
            runs_override: Override the number of runs from config
            debug_override: Override the debug setting from config

        Returns:
            PipelineResult with all stage results
        """
        debug = debug_override if debug_override is not None else self.config.debug
        runs = runs_override if runs_override is not None else self.config.execution.runs

        try:
            # Validate config
            errors = validate_config(self.config)
            if errors:
                return PipelineResult(
                    config_name=self.config.name,
                    spec=spec,
                    synthesis=None,
                    execution=None,
                    success=False,
                    error_message=f"Configuration errors: {'; '.join(errors)}",
                )

            # Stage 1: Build specification
            self._print_stage("Building Specification", debug)
            if debug:
                print(f"  Boilerplate: {self.config.boilerplate_path}")
                print(f"  Input spec: {spec[:80]}{'...' if len(spec) > 80 else ''}")

            spec_path = build_spec(
                self.config.boilerplate_path,
                spec,
            )
            self._temp_files.append(spec_path)

            if debug:
                print(f"  Generated spec: {spec_path}")

            # Stage 2: Synthesize controller
            self._print_stage("Synthesizing Controller", debug)

            synthesis_result = synthesize(
                spec_path,
                command=self.config.synthesis.command,
                args=self.config.synthesis.args,
                debug=debug,
            )

            if not synthesis_result.success:
                return PipelineResult(
                    config_name=self.config.name,
                    spec=spec,
                    synthesis=synthesis_result,
                    execution=None,
                    success=False,
                    error_message=f"Synthesis failed: {synthesis_result.error_message}",
                )

            print(f"  Controller synthesized successfully ({synthesis_result.duration:.1f}s)")

            # Stage 3: Embed controller
            self._print_stage("Embedding Controller", debug)

            embed_from_synthesis(
                self.config.game_path,
                synthesis_result.controller_code,
            )

            print(f"  Controller written to {self.config.game_path}")

            # Stage 4: Run validation
            self._print_stage(f"Running Validation ({runs} runs)", debug)

            execution_result = run_game(
                self.config.game_path,
                self.config.cjson_path,
                constraints_path=self.config.constraints_path,
                runs=runs,
                debug=debug,
                timeout=self.config.execution.timeout,
            )

            # Print run summaries
            if not debug:
                for run in execution_result.runs:
                    steps_str = f" ({run.steps} steps)" if run.steps else ""
                    print(f"  Run {run.run_number:3d}/{runs}: {run.status_str}{steps_str}")

            return PipelineResult(
                config_name=self.config.name,
                spec=spec,
                synthesis=synthesis_result,
                execution=execution_result,
                success=True,
            )

        except Exception as e:
            return PipelineResult(
                config_name=self.config.name,
                spec=spec,
                synthesis=None,
                execution=None,
                success=False,
                error_message=str(e),
            )

        finally:
            self._cleanup()

    def _print_stage(self, name: str, debug: bool) -> None:
        """Print a stage header."""
        print(f"\n=== {name} ===")

    def _cleanup(self) -> None:
        """Clean up temporary files."""
        for path in self._temp_files:
            try:
                path.unlink()
            except Exception:
                pass
        self._temp_files.clear()
