"""Experiment configuration dataclasses."""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Literal


@dataclass
class AnchorConfig:
    """Single anchor (transmitter) definition."""

    anchor_id: str
    x: float
    y: float
    rssi_d0: float = -50.0  # RSSI at reference distance d0 [dBm]
    n: float = 2.5  # Path-loss exponent
    d0: float = 1.0  # Reference distance [m]


@dataclass
class NoiseConfig:
    """RSSI noise model parameters."""

    sigma: float = 4.0  # Log-normal shadowing std [dB]
    ar_coefficient: float = 0.0  # AR(1) temporal correlation (0 = no correlation)
    outlier_probability: float = 0.0  # Probability of outlier per observation
    outlier_magnitude: float = 20.0  # Extra loss for outlier [dB]
    nlos_anchors: list[str] = field(default_factory=list)  # Anchors with NLOS
    nlos_extra_loss: float = 10.0  # Additional loss for NLOS anchors [dB]
    spatial_bias_scale: float = 0.0  # Scale of position-dependent systematic bias [dB]
    spatial_bias_seed: int = 99  # Seed for spatial bias field generation
    # --- Channel diversity model ---
    n_channels: int = 1  # Channels sampled & combined per observation (freq. diversity)
    fast_fading_fraction: float = 0.0  # Fraction of noise VARIANCE that is per-channel
    # independent (multipath/fast fading). The rest (1-frac) is frequency-common
    # (shadowing/body/systematic) and is NOT reduced by channel averaging.
    channel_combine: Literal["mean", "median"] = "mean"  # Multi-channel combiner


@dataclass
class TrajectoryConfig:
    """Receiver trajectory configuration."""

    type: Literal["static", "linear", "random_walk", "grid"] = "static"
    # Static point
    static_position: tuple[float, float] = (5.0, 5.0)
    # Linear movement
    start: tuple[float, float] = (1.0, 1.0)
    end: tuple[float, float] = (9.0, 9.0)
    speed: float = 1.0  # [m/s]
    # Random walk
    step_size: float = 0.3  # [m] per time step
    # Grid evaluation
    grid_spacing: float = 1.0  # [m]
    # Common
    field_size: tuple[float, float] = (10.0, 10.0)


@dataclass
class SimulationConfig:
    """Top-level simulation configuration."""

    anchors: list[AnchorConfig] = field(default_factory=list)
    noise: NoiseConfig = field(default_factory=NoiseConfig)
    trajectory: TrajectoryConfig = field(default_factory=TrajectoryConfig)
    duration_s: float = 10.0  # Total simulation time [s]
    dt: float = 0.1  # Time step [s] (100ms, typical BLE advertising interval)
    seed: int = 42  # Random seed for reproducibility


def default_config() -> SimulationConfig:
    """Create a default configuration with 4 anchors at corners of 10x10m field."""
    anchors = [
        AnchorConfig("anchor_1", 0.0, 0.0),
        AnchorConfig("anchor_2", 10.0, 0.0),
        AnchorConfig("anchor_3", 10.0, 10.0),
        AnchorConfig("anchor_4", 0.0, 10.0),
    ]
    return SimulationConfig(anchors=anchors)
