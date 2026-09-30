"""Particle filter estimator for RSSI-based positioning."""

from __future__ import annotations

import numpy as np

from ..config import AnchorConfig
from .base import Estimator


class ParticleFilterEstimator(Estimator):
    """Sequential Monte Carlo (particle filter) position estimator.

    Supports:
    - Random walk motion model (no IMU/PDR)
    - RSSI distance likelihood weighting
    - Systematic resampling
    - Optional field boundary constraints
    """

    def __init__(
        self,
        anchors: list[AnchorConfig],
        n_particles: int = 500,
        motion_noise: float = 0.5,
        measurement_sigma: float = 3.0,
        field_size: tuple[float, float] = (10.0, 10.0),
        seed: int = 42,
    ) -> None:
        super().__init__(anchors)
        self.n_particles = n_particles
        self.motion_noise = motion_noise
        self.measurement_sigma = measurement_sigma
        self.field_size = field_size
        self.rng = np.random.default_rng(seed + 2)

        # Initialize particles uniformly across field
        self.particles = np.column_stack([
            self.rng.uniform(0, field_size[0], n_particles),
            self.rng.uniform(0, field_size[1], n_particles),
        ])
        self.weights = np.ones(n_particles) / n_particles

    def update(self, rssi_obs: dict[str, float]) -> np.ndarray:
        # --- Prediction: random walk ---
        noise = self.rng.normal(0, self.motion_noise, size=(self.n_particles, 2))
        self.particles += noise

        # Clamp to field boundaries
        self.particles[:, 0] = np.clip(self.particles[:, 0], 0, self.field_size[0])
        self.particles[:, 1] = np.clip(self.particles[:, 1], 0, self.field_size[1])

        # --- Update: compute weights from RSSI likelihood ---
        log_weights = np.zeros(self.n_particles)

        for anchor in self.anchors:
            if anchor.anchor_id not in rssi_obs:
                continue

            rssi = rssi_obs[anchor.anchor_id]
            observed_dist = self.rssi_to_distance(rssi, anchor)

            # Distance from each particle to this anchor
            anchor_pos = np.array([anchor.x, anchor.y])
            particle_dists = np.linalg.norm(self.particles - anchor_pos, axis=1)

            # Gaussian likelihood in distance domain
            diff = particle_dists - observed_dist
            log_weights += -0.5 * (diff / self.measurement_sigma) ** 2

        # Normalize weights (log-sum-exp for numerical stability)
        log_weights -= np.max(log_weights)
        self.weights = np.exp(log_weights)
        weight_sum = self.weights.sum()
        if weight_sum > 0:
            self.weights /= weight_sum
        else:
            self.weights = np.ones(self.n_particles) / self.n_particles

        # --- Estimate: weighted mean ---
        estimate = self.particles.T @ self.weights

        # --- Resample if effective sample size is low ---
        n_eff = 1.0 / np.sum(self.weights**2)
        if n_eff < self.n_particles / 2:
            self._systematic_resample()

        return estimate

    def _systematic_resample(self) -> None:
        """Systematic resampling to avoid particle degeneracy."""
        n = self.n_particles
        positions = (np.arange(n) + self.rng.random()) / n
        cumsum = np.cumsum(self.weights)
        indices = np.searchsorted(cumsum, positions)
        indices = np.clip(indices, 0, n - 1)
        self.particles = self.particles[indices].copy()
        self.weights = np.ones(n) / n

    def reset(self) -> None:
        self.particles = np.column_stack([
            self.rng.uniform(0, self.field_size[0], self.n_particles),
            self.rng.uniform(0, self.field_size[1], self.n_particles),
        ])
        self.weights = np.ones(self.n_particles) / self.n_particles
