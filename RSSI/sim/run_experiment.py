"""Experiment runner — orchestrates environment, observation, estimators, and evaluation."""

from __future__ import annotations

import argparse
import sys
from typing import Type

import numpy as np

from .config import SimulationConfig, default_config, NoiseConfig, TrajectoryConfig
from .environment import Environment
from .observation import SyntheticSource
from .estimators import Estimator, WeightedTrilateration, EKFEstimator, ParticleFilterEstimator, AICorrectedEKF, EnhancedAICorrectedEstimator
from .evaluator import Evaluator


def run_single(config: SimulationConfig, estimator_cls: Type[Estimator], **estimator_kwargs) -> Evaluator:
    """Run a single experiment: generate data, estimate, evaluate.

    Args:
        config: Simulation configuration.
        estimator_cls: Estimator class to instantiate.
        **estimator_kwargs: Extra kwargs passed to estimator constructor.

    Returns:
        Evaluator instance with results.
    """
    # 1. Generate ground truth
    env = Environment(config)
    timestamps, true_positions = env.generate_trajectory()

    # 2. Generate observations
    source = SyntheticSource(config, true_positions)

    # 3. Run estimator
    estimator = estimator_cls(config.anchors, **estimator_kwargs)
    estimated_positions = np.zeros_like(true_positions)

    for t in range(len(timestamps)):
        obs = source.get_observations(t)
        estimated_positions[t] = estimator.update(obs)

    # 4. Evaluate
    evaluator = Evaluator(true_positions, estimated_positions, timestamps)
    return evaluator


def run_comparison(config: SimulationConfig) -> dict[str, Evaluator]:
    """Run all estimators on the same scenario for comparison."""
    env = Environment(config)
    timestamps, true_positions = env.generate_trajectory()
    source = SyntheticSource(config, true_positions)

    # Pre-generate all observations (same noise realization for fair comparison)
    all_obs = source.generate_all()

    estimators: dict[str, Estimator] = {
        "Trilateration": WeightedTrilateration(config.anchors),
        "EKF": EKFEstimator(config.anchors, dt=config.dt),
        "Particle Filter": ParticleFilterEstimator(
            config.anchors,
            field_size=config.trajectory.field_size,
            seed=config.seed,
        ),
    }

    # --- AI-Corrected EKF: calibrate then run ---
    ai_ekf = AICorrectedEKF(
        config.anchors,
        dt=config.dt,
        seed=config.seed,
    )

    # Simulate calibration: collect RSSI at 9 known positions (3x3 grid)
    calib_positions = [
        (2.0, 2.0), (5.0, 2.0), (8.0, 2.0),
        (2.0, 5.0), (5.0, 5.0), (8.0, 5.0),
        (2.0, 8.0), (5.0, 8.0), (8.0, 8.0),
    ]
    calib_config = SimulationConfig(
        anchors=config.anchors,
        noise=config.noise,
        trajectory=TrajectoryConfig(type="static"),
        duration_s=5.0,  # 5 seconds per point
        dt=config.dt,
        seed=config.seed + 100,
    )
    calib_sequences: list[list[dict[str, float]]] = []
    for pos in calib_positions:
        calib_config.trajectory.static_position = pos
        calib_env = Environment(calib_config)
        _, calib_true_pos = calib_env.generate_trajectory()
        calib_source = SyntheticSource(calib_config, calib_true_pos)
        calib_sequences.append(calib_source.generate_all())

    loss_history = ai_ekf.calibrate(calib_sequences, calib_positions, epochs=120, seed=config.seed)
    if loss_history:
        print(f"  AI-EKF calibration: final loss = {loss_history[-1]:.4f}")

    estimators["AI-Corrected EKF"] = ai_ekf

    # --- Enhanced AI + PF: same calibration data ---
    enhanced = EnhancedAICorrectedEstimator(
        config.anchors,
        dt=config.dt,
        field_size=config.trajectory.field_size,
        seed=config.seed,
    )
    loss_history_e = enhanced.calibrate(calib_sequences, calib_positions, epochs=120, seed=config.seed)
    if loss_history_e:
        print(f"  Enhanced calibration: final loss = {loss_history_e[-1]:.4f}")

    estimators["Enhanced AI+PF"] = enhanced

    results: dict[str, Evaluator] = {}

    for name, estimator in estimators.items():
        est_positions = np.zeros_like(true_positions)
        for t in range(len(timestamps)):
            est_positions[t] = estimator.update(all_obs[t])
        results[name] = Evaluator(true_positions, est_positions, timestamps)

    return results


def main() -> None:
    parser = argparse.ArgumentParser(description="RSSI Position Estimation Simulator")
    parser.add_argument("--trajectory", choices=["static", "linear", "random_walk", "grid"], default="random_walk")
    parser.add_argument("--duration", type=float, default=30.0, help="Simulation duration [s]")
    parser.add_argument("--sigma", type=float, default=4.0, help="RSSI noise std [dB]")
    parser.add_argument("--seed", type=int, default=42, help="Random seed")
    parser.add_argument("--save-dir", type=str, default=None, help="Directory to save plots")
    args = parser.parse_args()

    # Build config
    config = default_config()
    config.duration_s = args.duration
    config.seed = args.seed
    config.noise = NoiseConfig(sigma=args.sigma, spatial_bias_scale=3.0)
    config.trajectory = TrajectoryConfig(type=args.trajectory)

    print(f"Configuration:")
    print(f"  Trajectory: {args.trajectory}")
    print(f"  Duration: {args.duration}s, dt: {config.dt}s")
    print(f"  Noise sigma: {args.sigma} dB")
    print(f"  Anchors: {len(config.anchors)}")
    print()

    # Run comparison
    results = run_comparison(config)

    # Print summaries
    for name, evaluator in results.items():
        evaluator.print_summary(label=name)

    anchor_pos = np.array([[a.x, a.y] for a in config.anchors])

    # Generate plots
    save_dir = None
    if args.save_dir:
        from pathlib import Path
        save_dir = Path(args.save_dir)
        save_dir.mkdir(parents=True, exist_ok=True)

    # Per-estimator plots in separate folders
    for name, ev in results.items():
        folder_name = name.lower().replace(" ", "_").replace("-", "_")
        if save_dir:
            est_dir = save_dir / folder_name
            est_dir.mkdir(parents=True, exist_ok=True)
            ev.plot_trajectory(anchor_pos, save_path=str(est_dir / "trajectory.png"))
            ev.plot_error_over_time(save_path=str(est_dir / "error_time.png"))
            ev.plot_cdf(save_path=str(est_dir / "cdf.png"))
        else:
            ev.plot_trajectory(anchor_pos)
            ev.plot_error_over_time()
            ev.plot_cdf()

    # Comparison plots
    _plot_comparison(results, anchor_pos, save_dir)


def _plot_comparison(
    results: dict[str, "Evaluator"],
    anchor_pos: np.ndarray,
    save_dir: "Path | None" = None,
) -> None:
    """Generate multi-estimator comparison plots."""
    import matplotlib.pyplot as plt

    colors = {
        "Trilateration": "#d62728",
        "EKF": "#ff7f0e",
        "Particle Filter": "#9467bd",
        "AI-Corrected EKF": "#2ca02c",
        "Enhanced AI+PF": "#1f77b4",
    }

    # --- 1. All trajectories on one plot ---
    fig, ax = plt.subplots(1, 1, figsize=(10, 10))
    # True trajectory (from any evaluator — they share the same ground truth)
    first_eval = next(iter(results.values()))
    ax.plot(first_eval.true_pos[:, 0], first_eval.true_pos[:, 1],
            "k-", linewidth=2.5, label="True", zorder=10)

    for name, ev in results.items():
        c = colors.get(name, "gray")
        ax.plot(ev.est_pos[:, 0], ev.est_pos[:, 1],
                "--", linewidth=1, alpha=0.7, color=c, label=f"{name} (MAE={ev.mae():.2f}m)")

    ax.scatter(anchor_pos[:, 0], anchor_pos[:, 1],
               marker="^", s=200, c="blue", zorder=15, label="Anchors")
    ax.set_xlabel("x [m]")
    ax.set_ylabel("y [m]")
    ax.set_aspect("equal")
    ax.legend(fontsize=9)
    ax.set_title("Position Estimation: All Estimators")
    ax.grid(True, alpha=0.3)

    if save_dir:
        fig.savefig(str(save_dir / "trajectory_comparison.png"), dpi=150, bbox_inches="tight")
        print(f"Saved: {save_dir / 'trajectory_comparison.png'}")
    else:
        plt.show()
    plt.close(fig)

    # --- 2. Error over time for all estimators ---
    fig, ax = plt.subplots(1, 1, figsize=(12, 5))
    for name, ev in results.items():
        c = colors.get(name, "gray")
        t = ev.timestamps if ev.timestamps is not None else np.arange(len(ev.errors))
        ax.plot(t, ev.errors, linewidth=0.8, color=c, alpha=0.8,
                label=f"{name} (MAE={ev.mae():.2f}m)")
    ax.set_xlabel("Time [s]")
    ax.set_ylabel("Error [m]")
    ax.set_title("Position Error Over Time: All Estimators")
    ax.legend(fontsize=9)
    ax.grid(True, alpha=0.3)

    if save_dir:
        fig.savefig(str(save_dir / "error_comparison.png"), dpi=150, bbox_inches="tight")
        print(f"Saved: {save_dir / 'error_comparison.png'}")
    else:
        plt.show()
    plt.close(fig)

    # --- 3. CDF comparison ---
    fig, ax = plt.subplots(1, 1, figsize=(8, 6))
    for name, ev in results.items():
        c = colors.get(name, "gray")
        sorted_errors = np.sort(ev.errors)
        cdf = np.arange(1, len(sorted_errors) + 1) / len(sorted_errors)
        ax.plot(sorted_errors, cdf, linewidth=2, color=c, label=f"{name}")
    ax.set_xlabel("Error [m]")
    ax.set_ylabel("CDF")
    ax.set_title("Cumulative Error Distribution: All Estimators")
    ax.legend(fontsize=9)
    ax.grid(True, alpha=0.3)

    if save_dir:
        fig.savefig(str(save_dir / "cdf_comparison.png"), dpi=150, bbox_inches="tight")
        print(f"Saved: {save_dir / 'cdf_comparison.png'}")
    else:
        plt.show()
    plt.close(fig)


if __name__ == "__main__":
    main()
