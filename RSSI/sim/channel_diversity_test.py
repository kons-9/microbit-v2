"""Channel-diversity experiment: reduce instantaneous noise by combining channels.

Physics: total RSSI noise = frequency-COMMON (shadowing/body) + per-channel
FAST FADING (multipath, decorrelated across BLE channels). Sampling N channels
and combining (mean) averages out the fast-fading part:

    sigma_eff = sigma * sqrt( (1-f) + f / N )

where f = fast_fading_fraction. The common part (1-f) is the irreducible floor.

Sweeps N_channels for a plain EKF and the AI-corrected EKF under realistic
walking (1.5 m/s, 10 Hz). Run:  python -m sim.channel_diversity_test
"""

from __future__ import annotations

import numpy as np

from .config import NoiseConfig, SimulationConfig, TrajectoryConfig, default_config
from .environment import Environment
from .estimators import AICorrectedEKF, EKFEstimator
from .evaluator import Evaluator
from .observation import SyntheticSource

CALIB_POSITIONS = [
    (2.0, 2.0), (5.0, 2.0), (8.0, 2.0),
    (2.0, 5.0), (5.0, 5.0), (8.0, 5.0),
    (2.0, 8.0), (5.0, 8.0), (8.0, 8.0),
]

SIGMA = 4.0
FAST_FADING_FRACTION = 0.6  # 60% of noise variance is per-channel multipath
EPOCHS = 120


def _make_noise(n_channels: int, fast_fraction: float) -> NoiseConfig:
    return NoiseConfig(
        sigma=SIGMA,
        spatial_bias_scale=3.0,
        n_channels=n_channels,
        fast_fading_fraction=fast_fraction,
        channel_combine="mean",
    )


def _make_config(n_channels: int, fast_fraction: float, seed: int) -> SimulationConfig:
    cfg = default_config()
    cfg.duration_s = 30.0
    cfg.dt = 0.1
    cfg.seed = seed
    cfg.noise = _make_noise(n_channels, fast_fraction)
    cfg.trajectory = TrajectoryConfig(
        type="random_walk", step_size=0.15, static_position=(5.0, 5.0)
    )
    return cfg


def _gen_calib(cfg: SimulationConfig) -> list[list[dict[str, float]]]:
    calib_cfg = SimulationConfig(
        anchors=cfg.anchors,
        noise=cfg.noise,
        trajectory=TrajectoryConfig(type="static"),
        duration_s=5.0,
        dt=cfg.dt,
        seed=cfg.seed + 100,
    )
    seqs: list[list[dict[str, float]]] = []
    for pos in CALIB_POSITIONS:
        calib_cfg.trajectory.static_position = pos
        env = Environment(calib_cfg)
        _, true_pos = env.generate_trajectory()
        seqs.append(SyntheticSource(calib_cfg, true_pos).generate_all())
    return seqs


def _run_one(cfg: SimulationConfig) -> dict[str, float]:
    env = Environment(cfg)
    timestamps, true_positions = env.generate_trajectory()
    all_obs = SyntheticSource(cfg, true_positions).generate_all()
    calib_seqs = _gen_calib(cfg)

    out: dict[str, float] = {}

    ekf = EKFEstimator(cfg.anchors, dt=cfg.dt)
    est = np.zeros_like(true_positions)
    for t in range(len(timestamps)):
        est[t] = ekf.update(all_obs[t])
    out["EKF"] = Evaluator(true_positions, est, timestamps).mae()

    ai = AICorrectedEKF(cfg.anchors, dt=cfg.dt, seed=cfg.seed)
    ai.calibrate(calib_seqs, CALIB_POSITIONS, epochs=EPOCHS, seed=cfg.seed)
    est = np.zeros_like(true_positions)
    for t in range(len(timestamps)):
        est[t] = ai.update(all_obs[t])
    out["AI-EKF"] = Evaluator(true_positions, est, timestamps).mae()

    return out


def main() -> None:
    fractions = [0.3, 0.6, 0.9]
    channels = [1, 3, 10]
    seeds = [42, 43, 44, 45, 46, 47]

    print("Channel-diversity sensitivity  (sigma=4dB, spatial_bias=3.0, 1.5 m/s, 10 Hz)")
    print(f"averaged over {len(seeds)} seeds. Each cell: EKF / AI-EKF MAE [m]\n")

    # Header
    col_hdr = "  ".join(f"N={n:<11}" for n in channels)
    print(f"{'fast_frac':>9} | {col_hdr}")
    print("-" * (12 + len(col_hdr) + 2))

    for f in fractions:
        cells = []
        for n_ch in channels:
            agg_ekf, agg_ai = [], []
            for seed in seeds:
                res = _run_one(_make_config(n_ch, f, seed))
                agg_ekf.append(res["EKF"])
                agg_ai.append(res["AI-EKF"])
            cells.append(f"{np.mean(agg_ekf):.2f}/{np.mean(agg_ai):.2f}")
        row = "  ".join(f"{c:<13}" for c in cells)
        print(f"{f:>9.1f} | {row}")

    print("\nEach cell = EKF MAE / AI-EKF MAE. Lower is better.")
    print("sigma_eff = 4 * sqrt((1-f) + f/N).  Channel diversity only reduces the")
    print("per-channel (fast-fading) fraction f; the common part (1-f) is the floor.")


if __name__ == "__main__":
    main()
