"""A/B experiment: does a temporal-window AI denoiser beat mean/std features?

Compares three RSSI correctors under increasing temporal noise correlation:

    baseline   : AICorrectedEKF          (mean/std summary features)
    window_mlp : TemporalAICorrectedEKF  (raw window -> MLP)          [A]
    tcn        : TemporalAICorrectedEKF  (raw window -> causal TCN)   [B]

Sweeps ``ar_coefficient`` (0.0 = white, up to 0.8 = strongly correlated).
Hypothesis:
    * white noise      -> all three ~equal (temporal model has no signal to use)
    * correlated noise -> window_mlp/tcn pull ahead (temporal structure usable)

Run:  python -m sim.temporal_ab
"""

from __future__ import annotations

import numpy as np

from .config import (
    NoiseConfig,
    SimulationConfig,
    TrajectoryConfig,
    default_config,
)
from .environment import Environment
from .estimators import AICorrectedEKF, TemporalAICorrectedEKF
from .evaluator import Evaluator
from .observation import SyntheticSource

CALIB_POSITIONS = [
    (2.0, 2.0), (5.0, 2.0), (8.0, 2.0),
    (2.0, 5.0), (5.0, 5.0), (8.0, 5.0),
    (2.0, 8.0), (5.0, 8.0), (8.0, 8.0),
]

WINDOW = 8
EPOCHS = 120


def _make_config(ar: float, seed: int) -> SimulationConfig:
    cfg = default_config()
    cfg.duration_s = 30.0
    cfg.dt = 0.1  # 10 Hz
    cfg.seed = seed
    cfg.noise = NoiseConfig(sigma=4.0, ar_coefficient=ar, spatial_bias_scale=3.0)
    cfg.trajectory = TrajectoryConfig(
        type="random_walk",
        step_size=0.15,  # 1.5 m/s at dt=0.1 (realistic walking)
        static_position=(5.0, 5.0),
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
    sequences: list[list[dict[str, float]]] = []
    for pos in CALIB_POSITIONS:
        calib_cfg.trajectory.static_position = pos
        env = Environment(calib_cfg)
        _, true_pos = env.generate_trajectory()
        src = SyntheticSource(calib_cfg, true_pos)
        sequences.append(src.generate_all())
    return sequences


def _run_one(cfg: SimulationConfig) -> dict[str, float]:
    env = Environment(cfg)
    timestamps, true_positions = env.generate_trajectory()
    source = SyntheticSource(cfg, true_positions)
    all_obs = source.generate_all()

    calib_seqs = _gen_calib(cfg)

    estimators = {
        "baseline": AICorrectedEKF(cfg.anchors, dt=cfg.dt, seed=cfg.seed),
        "window_mlp": TemporalAICorrectedEKF(
            cfg.anchors, dt=cfg.dt, window=WINDOW, backend="window_mlp", seed=cfg.seed
        ),
        "tcn": TemporalAICorrectedEKF(
            cfg.anchors, dt=cfg.dt, window=WINDOW, backend="tcn", seed=cfg.seed
        ),
    }

    maes: dict[str, float] = {}
    for name, est in estimators.items():
        est.calibrate(calib_seqs, CALIB_POSITIONS, epochs=EPOCHS, seed=cfg.seed)
        est_positions = np.zeros_like(true_positions)
        for t in range(len(timestamps)):
            est_positions[t] = est.update(all_obs[t])
        maes[name] = Evaluator(true_positions, est_positions, timestamps).mae()
    return maes


def main() -> None:
    ar_values = [0.0, 0.5, 0.8]
    seeds = [42, 43, 44]

    print("Temporal-window AI denoiser A/B  (sigma=4dB, spatial_bias=3.0, 1.5 m/s, 10 Hz)")
    print(f"window={WINDOW}, epochs={EPOCHS}, averaged over {len(seeds)} seeds\n")
    header = f"{'ar_coef':>8} | {'baseline':>10} | {'window_mlp':>11} | {'tcn':>8}"
    print(header)
    print("-" * len(header))

    for ar in ar_values:
        agg: dict[str, list[float]] = {"baseline": [], "window_mlp": [], "tcn": []}
        for seed in seeds:
            cfg = _make_config(ar, seed)
            maes = _run_one(cfg)
            for k, v in maes.items():
                agg[k].append(v)
        b = np.mean(agg["baseline"])
        w = np.mean(agg["window_mlp"])
        t = np.mean(agg["tcn"])
        print(f"{ar:>8.2f} | {b:>10.3f} | {w:>11.3f} | {t:>8.3f}")

    print("\n(MAE in meters; lower is better)")


if __name__ == "__main__":
    main()
