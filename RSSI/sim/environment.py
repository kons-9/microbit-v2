"""Module 1: Environment model — ground truth trajectories and anchor layout."""

from __future__ import annotations

import numpy as np

from .config import SimulationConfig, TrajectoryConfig


class Environment:
    """Generates ground-truth receiver positions over time."""

    def __init__(self, config: SimulationConfig) -> None:
        self.config = config
        self.anchors = {a.anchor_id: np.array([a.x, a.y]) for a in config.anchors}
        self.rng = np.random.default_rng(config.seed)

    def generate_trajectory(self) -> tuple[np.ndarray, np.ndarray]:
        """Generate ground-truth positions.

        Returns:
            timestamps: shape (T,) array of time values [s]
            positions: shape (T, 2) array of (x, y) positions [m]
        """
        tc = self.config.trajectory
        n_steps = int(self.config.duration_s / self.config.dt)
        timestamps = np.arange(n_steps) * self.config.dt

        if tc.type == "static":
            positions = np.tile(tc.static_position, (n_steps, 1))
        elif tc.type == "linear":
            positions = self._linear_trajectory(tc, n_steps)
        elif tc.type == "random_walk":
            positions = self._random_walk(tc, n_steps)
        elif tc.type == "grid":
            timestamps, positions = self._grid_positions(tc)
        else:
            raise ValueError(f"Unknown trajectory type: {tc.type}")

        return timestamps, positions

    def _linear_trajectory(self, tc: TrajectoryConfig, n_steps: int) -> np.ndarray:
        start = np.array(tc.start)
        end = np.array(tc.end)
        direction = end - start
        total_dist = np.linalg.norm(direction)
        unit = direction / total_dist

        distances = np.linspace(0, min(tc.speed * self.config.duration_s, total_dist), n_steps)
        positions = start + np.outer(distances, unit)
        return positions

    def _random_walk(self, tc: TrajectoryConfig, n_steps: int) -> np.ndarray:
        positions = np.zeros((n_steps, 2))
        positions[0] = tc.static_position  # start position

        for i in range(1, n_steps):
            angle = self.rng.uniform(0, 2 * np.pi)
            step = tc.step_size * np.array([np.cos(angle), np.sin(angle)])
            candidate = positions[i - 1] + step
            # Clamp to field boundaries
            candidate[0] = np.clip(candidate[0], 0, tc.field_size[0])
            candidate[1] = np.clip(candidate[1], 0, tc.field_size[1])
            positions[i] = candidate

        return positions

    def _grid_positions(self, tc: TrajectoryConfig) -> tuple[np.ndarray, np.ndarray]:
        xs = np.arange(0, tc.field_size[0] + tc.grid_spacing, tc.grid_spacing)
        ys = np.arange(0, tc.field_size[1] + tc.grid_spacing, tc.grid_spacing)
        xx, yy = np.meshgrid(xs, ys)
        positions = np.column_stack([xx.ravel(), yy.ravel()])
        timestamps = np.arange(len(positions)) * self.config.dt
        return timestamps, positions
