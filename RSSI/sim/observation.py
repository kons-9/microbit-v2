"""Module 2: RSSI observation generation — synthetic and recorded sources."""

from __future__ import annotations

from pathlib import Path
from typing import Protocol

import numpy as np
import csv

from .config import AnchorConfig, NoiseConfig, SimulationConfig


class RssiSource(Protocol):
    """Protocol for RSSI data sources."""

    def get_observations(self, t_index: int) -> dict[str, float]:
        """Return RSSI [dBm] from each anchor at time index t_index."""
        ...


class SyntheticSource:
    """Generate RSSI observations from log-distance path-loss model with configurable noise."""

    def __init__(self, config: SimulationConfig, positions: np.ndarray) -> None:
        """
        Args:
            config: Simulation configuration.
            positions: Ground-truth positions, shape (T, 2).
        """
        self.config = config
        self.positions = positions
        self.anchors = config.anchors
        self.noise_cfg = config.noise
        self.rng = np.random.default_rng(config.seed + 1)

        # Pre-generate AR(1) noise state per anchor
        self._ar_state: dict[str, float] = {a.anchor_id: 0.0 for a in self.anchors}

        # Pre-generate spatial bias fields (smooth, position-dependent systematic error)
        self._spatial_bias = self._generate_spatial_bias_fields()

    def _generate_spatial_bias_fields(self) -> dict[str, np.ndarray | None]:
        """Generate a smooth spatial bias field per anchor using random Fourier features."""
        nc = self.noise_cfg
        if nc.spatial_bias_scale <= 0:
            return {a.anchor_id: None for a in self.anchors}

        bias_rng = np.random.default_rng(nc.spatial_bias_seed)
        fields: dict[str, np.ndarray | None] = {}
        n_components = 6  # Number of sinusoidal components for smooth field

        for anchor in self.anchors:
            # Random frequencies (low-frequency for smooth spatial variation)
            freqs = bias_rng.uniform(0.2, 0.8, size=(n_components, 2))
            phases = bias_rng.uniform(0, 2 * np.pi, size=n_components)
            amplitudes = bias_rng.normal(0, 1, size=n_components)
            fields[anchor.anchor_id] = np.column_stack([freqs, phases[:, None], amplitudes[:, None]])

        return fields

    def _evaluate_spatial_bias(self, anchor_id: str, position: np.ndarray) -> float:
        """Evaluate spatial bias at a given position for a given anchor."""
        field_params = self._spatial_bias.get(anchor_id)
        if field_params is None:
            return 0.0

        bias = 0.0
        for row in field_params:
            fx, fy, phase, amplitude = row
            bias += amplitude * np.sin(2 * np.pi * (fx * position[0] + fy * position[1]) + phase)

        return float(bias * self.noise_cfg.spatial_bias_scale)

    def get_observations(self, t_index: int) -> dict[str, float]:
        pos = self.positions[t_index]
        observations: dict[str, float] = {}

        for anchor in self.anchors:
            anchor_pos = np.array([anchor.x, anchor.y])
            dist = np.linalg.norm(pos - anchor_pos)
            dist = max(dist, 0.01)  # Avoid log(0)

            # Base RSSI from path-loss model
            rssi = anchor.rssi_d0 - 10.0 * anchor.n * np.log10(dist / anchor.d0)

            # Add spatial bias (deterministic, position-dependent)
            rssi += self._evaluate_spatial_bias(anchor.anchor_id, pos)

            # Add noise
            rssi += self._generate_noise(anchor)

            observations[anchor.anchor_id] = float(rssi)

        return observations

    def _generate_noise(self, anchor: AnchorConfig) -> float:
        nc = self.noise_cfg
        aid = anchor.anchor_id

        # Split total noise variance into a frequency-COMMON component (shadowing,
        # body, systematic — shared by every channel) and a per-channel independent
        # FAST-FADING component (multipath — decorrelated across BLE channels).
        f = min(max(nc.fast_fading_fraction, 0.0), 1.0)
        sigma_common = nc.sigma * np.sqrt(1.0 - f)
        sigma_fast = nc.sigma * np.sqrt(f)

        # Common shadowing (frequency-independent), optionally AR(1) correlated in time.
        common_white = self.rng.normal(0, sigma_common) if sigma_common > 0 else 0.0
        if nc.ar_coefficient > 0:
            self._ar_state[aid] = (
                nc.ar_coefficient * self._ar_state[aid]
                + np.sqrt(1 - nc.ar_coefficient**2) * common_white
            )
            common = self._ar_state[aid]
        else:
            common = common_white

        n_ch = max(1, nc.n_channels)

        # Per-channel fast fading (independent across channels).
        if sigma_fast > 0:
            per_channel = self.rng.normal(0, sigma_fast, size=n_ch)
        else:
            per_channel = np.zeros(n_ch)

        channel_noise = common + per_channel  # noise on each sampled channel

        # Outliers occur per-channel (a corrupted packet on one frequency).
        if nc.outlier_probability > 0:
            hits = self.rng.random(n_ch) < nc.outlier_probability
            channel_noise[hits] -= nc.outlier_magnitude

        # Combine channels (diversity gain).
        if nc.channel_combine == "median":
            noise = float(np.median(channel_noise))
        else:
            noise = float(np.mean(channel_noise))

        # NLOS extra loss (frequency-common structural blockage).
        if aid in nc.nlos_anchors:
            noise -= nc.nlos_extra_loss

        return noise

    def generate_all(self) -> list[dict[str, float]]:
        """Generate observations for all time steps at once."""
        return [self.get_observations(t) for t in range(len(self.positions))]


class RecordedSource:
    """Load RSSI observations from a CSV file (recorded real data)."""

    def __init__(self, csv_path: str | Path) -> None:
        """Load CSV with columns: timestamp_ms, anchor_id, rssi_dbm."""
        self._data: list[dict[str, float]] = []
        self._load(Path(csv_path))

    def _load(self, path: Path) -> None:
        raw: dict[int, dict[str, float]] = {}

        with open(path, newline="", encoding="utf-8") as f:
            reader = csv.DictReader(f)
            for row in reader:
                ts = int(row["timestamp_ms"])
                anchor_id = row["anchor_id"]
                rssi = float(row["rssi_dbm"])
                if ts not in raw:
                    raw[ts] = {}
                raw[ts][anchor_id] = rssi

        # Sort by timestamp and store as ordered list
        for ts in sorted(raw.keys()):
            self._data.append(raw[ts])

    def get_observations(self, t_index: int) -> dict[str, float]:
        if t_index >= len(self._data):
            raise IndexError(f"t_index {t_index} out of range (have {len(self._data)} records)")
        return self._data[t_index]

    def __len__(self) -> int:
        return len(self._data)
