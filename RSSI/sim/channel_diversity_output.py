"""Output artifacts for the Channel-Diversity AI-EKF algorithm.

Generates plots + a summary demonstrating that multi-channel acquisition with
median combining keeps the AI-corrected EKF robust under per-channel outliers
(2.4 GHz interference), where a single-channel pipeline collapses.

Artifacts -> sim/output/channel_diversity/
    robustness_curve.png   : MAE vs outlier probability (N=1 vs N=10 median)
    trajectory_compare.png : trajectories at p=0.15 (N=1 vs N=10 median)
    summary.txt            : numeric summary table

Run:  python -m sim.channel_diversity_output
"""

from __future__ import annotations

from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np

from .config import NoiseConfig, SimulationConfig, TrajectoryConfig, default_config
from .environment import Environment
from .estimators import AICorrectedEKF
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
OUT_DIR = Path(__file__).parent / "output" / "channel_diversity"


def _cfg(n_ch: int, combine: str, p_out: float, seed: int) -> SimulationConfig:
    cfg = default_config()
    cfg.duration_s = 30.0
    cfg.dt = 0.1
    cfg.seed = seed
    cfg.noise = NoiseConfig(
        sigma=SIGMA, spatial_bias_scale=3.0, n_channels=n_ch,
        fast_fading_fraction=FAST_FRACTION, channel_combine=combine,
        outlier_probability=p_out, outlier_magnitude=20.0,
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


def _run_ai(cfg: SimulationConfig) -> Evaluator:
    env = Environment(cfg)
    ts, tp = env.generate_trajectory()
    obs = SyntheticSource(cfg, tp).generate_all()
    ai = AICorrectedEKF(cfg.anchors, dt=cfg.dt, seed=cfg.seed)
    ai.calibrate(_gen_calib(cfg), CALIB_POSITIONS, epochs=EPOCHS, seed=cfg.seed)
    est = np.zeros_like(tp)
    for t in range(len(ts)):
        est[t] = ai.update(obs[t])
    return Evaluator(tp, est, ts)


def _robustness_curve(seeds: list[int]) -> dict[str, list[float]]:
    probs = [0.0, 0.02, 0.05, 0.10, 0.15, 0.20]
    variants = {
        "AI-EKF (N=1, no diversity)": (1, "mean"),
        "AI-EKF + Channel Diversity (N=10, median)": (10, "median"),
    }
    curves: dict[str, list[float]] = {k: [] for k in variants}
    for p in probs:
        for name, (n_ch, comb) in variants.items():
            vals = [_run_ai(_cfg(n_ch, comb, p, s)).mae() for s in seeds]
            curves[name].append(float(np.mean(vals)))

    fig, ax = plt.subplots(figsize=(9, 6))
    for name, ys in curves.items():
        ax.plot(probs, ys, "o-", linewidth=2, markersize=6, label=name)
    ax.set_xlabel("Per-channel outlier probability")
    ax.set_ylabel("MAE [m]")
    ax.set_title("Channel Diversity keeps AI-EKF robust under 2.4 GHz outliers")
    ax.grid(True, alpha=0.3)
    ax.legend()
    fig.tight_layout()
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    fig.savefig(OUT_DIR / "robustness_curve.png", dpi=120)
    plt.close(fig)
    curves["_probs"] = probs
    return curves


def _trajectory_compare(seed: int) -> None:
    p = 0.15
    ev1 = _run_ai(_cfg(1, "mean", p, seed))
    ev10 = _run_ai(_cfg(10, "median", p, seed))
    anchors = np.array([[a.x, a.y] for a in default_config().anchors])

    fig, axes = plt.subplots(1, 2, figsize=(14, 7))
    for ax, ev, title in (
        (axes[0], ev1, f"N=1 (no diversity)  MAE={ev1.mae():.2f}m"),
        (axes[1], ev10, f"N=10 median  MAE={ev10.mae():.2f}m"),
    ):
        ax.plot(ev.true_pos[:, 0], ev.true_pos[:, 1], "g-", linewidth=2, label="True")
        ax.plot(ev.est_pos[:, 0], ev.est_pos[:, 1], "r--", linewidth=1, alpha=0.7, label="Estimated")
        ax.scatter(anchors[:, 0], anchors[:, 1], marker="^", s=180, c="blue", zorder=5, label="Anchors")
        ax.set_xlim(-1, 11)
        ax.set_ylim(-1, 11)
        ax.set_aspect("equal")
        ax.set_title(title)
        ax.grid(True, alpha=0.3)
        ax.legend(fontsize=8)
    fig.suptitle(f"Trajectory under per-channel outliers (p={p})")
    fig.tight_layout()
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    fig.savefig(OUT_DIR / "trajectory_compare.png", dpi=120)
    plt.close(fig)


def main() -> None:
    seeds = [42, 43, 44, 45, 46, 47]
    print("Generating Channel-Diversity AI-EKF output artifacts...")
    curves = _robustness_curve(seeds)
    _trajectory_compare(seed=42)

    probs = curves.pop("_probs")
    lines = ["Channel-Diversity AI-EKF — robustness summary",
             "sigma=4dB, fast_fading_fraction=0.6, spatial_bias=3.0, 1.5 m/s, 10 Hz",
             f"averaged over {len(seeds)} seeds\n",
             f"{'p_outlier':>10} | " + " | ".join(f"{k:>42}" for k in curves)]
    lines.append("-" * len(lines[-1]))
    for i, p in enumerate(probs):
        row = f"{p:>10.2f} | " + " | ".join(f"{curves[k][i]:>42.3f}" for k in curves)
        lines.append(row)
    text = "\n".join(lines)

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    (OUT_DIR / "summary.txt").write_text(text + "\n", encoding="utf-8")
    print(text)
    print(f"\nArtifacts saved to: {OUT_DIR}")


if __name__ == "__main__":
    main()
