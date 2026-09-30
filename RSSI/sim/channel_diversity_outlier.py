"""Channel-diversity outlier-robustness test.

Real 2.4 GHz operation suffers per-channel outliers (WiFi interference, corrupted
packets on specific frequencies). A temporal filter handles these poorly (a sudden
drop induces lag), but combining several channels with a MEDIAN rejects them.

Compares no-diversity (N=1) vs channel diversity (N=10, mean vs median) under
increasing per-channel outlier probability, for EKF and AI-corrected EKF.

Run:  python -m sim.channel_diversity_outlier
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
FAST_FRACTION = 0.6
EPOCHS = 120


def _cfg(n_ch: int, combine: str, p_out: float, seed: int) -> SimulationConfig:
    cfg = default_config()
    cfg.duration_s = 30.0
    cfg.dt = 0.1
    cfg.seed = seed
    cfg.noise = NoiseConfig(
        sigma=SIGMA,
        spatial_bias_scale=3.0,
        n_channels=n_ch,
        fast_fading_fraction=FAST_FRACTION,
        channel_combine=combine,
        outlier_probability=p_out,
        outlier_magnitude=20.0,
    )
    cfg.trajectory = TrajectoryConfig(
        type="random_walk", step_size=0.15, static_position=(5.0, 5.0)
    )
    return cfg


def _gen_calib(cfg: SimulationConfig) -> list[list[dict[str, float]]]:
    calib_cfg = SimulationConfig(
        anchors=cfg.anchors, noise=cfg.noise,
        trajectory=TrajectoryConfig(type="static"),
        duration_s=5.0, dt=cfg.dt, seed=cfg.seed + 100,
    )
    seqs = []
    for pos in CALIB_POSITIONS:
        calib_cfg.trajectory.static_position = pos
        env = Environment(calib_cfg)
        _, tp = env.generate_trajectory()
        seqs.append(SyntheticSource(calib_cfg, tp).generate_all())
    return seqs


def _run(cfg: SimulationConfig) -> dict[str, float]:
    env = Environment(cfg)
    ts, tp = env.generate_trajectory()
    obs = SyntheticSource(cfg, tp).generate_all()
    out = {}

    ekf = EKFEstimator(cfg.anchors, dt=cfg.dt)
    est = np.zeros_like(tp)
    for t in range(len(ts)):
        est[t] = ekf.update(obs[t])
    out["EKF"] = Evaluator(tp, est, ts).mae()

    ai = AICorrectedEKF(cfg.anchors, dt=cfg.dt, seed=cfg.seed)
    ai.calibrate(_gen_calib(cfg), CALIB_POSITIONS, epochs=EPOCHS, seed=cfg.seed)
    est = np.zeros_like(tp)
    for t in range(len(ts)):
        est[t] = ai.update(obs[t])
    out["AI-EKF"] = Evaluator(tp, est, ts).mae()
    return out


def main() -> None:
    configs = [
        ("N=1 (no diversity)", 1, "mean"),
        ("N=10 mean", 10, "mean"),
        ("N=10 median", 10, "median"),
    ]
    outlier_probs = [0.0, 0.05, 0.15]
    seeds = [42, 43, 44, 45, 46, 47]

    print("Channel-diversity OUTLIER robustness  (sigma=4dB, fast_frac=0.6, "
          "spatial_bias=3.0, 1.5 m/s, 10 Hz)")
    print(f"outlier_magnitude=20 dB, averaged over {len(seeds)} seeds. "
          "Cell: EKF / AI-EKF MAE [m]\n")

    hdr = f"{'p_outlier':>10} | " + "  ".join(f"{name:<20}" for name, _, _ in configs)
    print(hdr)
    print("-" * len(hdr))

    for p in outlier_probs:
        cells = []
        for _, n_ch, combine in configs:
            ekf_v, ai_v = [], []
            for seed in seeds:
                res = _run(_cfg(n_ch, combine, p, seed))
                ekf_v.append(res["EKF"])
                ai_v.append(res["AI-EKF"])
            cells.append(f"{np.mean(ekf_v):.2f}/{np.mean(ai_v):.2f}")
        print(f"{p:>10.2f} | " + "  ".join(f"{c:<20}" for c in cells))

    print("\nCell = EKF MAE / AI-EKF MAE. Lower is better.")


if __name__ == "__main__":
    main()
