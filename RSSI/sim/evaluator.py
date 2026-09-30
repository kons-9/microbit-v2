"""Module 4: Evaluation — metrics computation and visualization."""

from __future__ import annotations

from pathlib import Path

import numpy as np


class Evaluator:
    """Compute position estimation error metrics and generate plots."""

    def __init__(
        self,
        true_positions: np.ndarray,
        estimated_positions: np.ndarray,
        timestamps: np.ndarray | None = None,
    ) -> None:
        """
        Args:
            true_positions: shape (T, 2)
            estimated_positions: shape (T, 2)
            timestamps: shape (T,), optional
        """
        self.true_pos = true_positions
        self.est_pos = estimated_positions
        self.timestamps = timestamps
        self.errors = np.linalg.norm(true_positions - estimated_positions, axis=1)

    def mae(self) -> float:
        """Mean Absolute Error [m]."""
        return float(np.mean(self.errors))

    def rmse(self) -> float:
        """Root Mean Square Error [m]."""
        return float(np.sqrt(np.mean(self.errors**2)))

    def percentile(self, p: float = 95.0) -> float:
        """p-th percentile of error [m]."""
        return float(np.percentile(self.errors, p))

    def max_error(self) -> float:
        """Maximum error [m]."""
        return float(np.max(self.errors))

    def summary(self) -> dict[str, float]:
        """Return all metrics as a dict."""
        return {
            "MAE [m]": self.mae(),
            "RMSE [m]": self.rmse(),
            "95th percentile [m]": self.percentile(95),
            "Max error [m]": self.max_error(),
        }

    def print_summary(self, label: str = "") -> None:
        """Print metrics to console."""
        header = f"=== {label} ===" if label else "=== Evaluation Results ==="
        print(header)
        for key, val in self.summary().items():
            print(f"  {key}: {val:.3f}")
        print()

    def plot_trajectory(self, anchor_positions: np.ndarray | None = None, save_path: str | None = None) -> None:
        """Plot true vs estimated trajectory in 2D."""
        import matplotlib.pyplot as plt

        fig, ax = plt.subplots(1, 1, figsize=(8, 8))

        ax.plot(self.true_pos[:, 0], self.true_pos[:, 1], "g-", linewidth=2, label="True")
        ax.plot(self.est_pos[:, 0], self.est_pos[:, 1], "r--", linewidth=1, alpha=0.7, label="Estimated")

        if anchor_positions is not None:
            ax.scatter(anchor_positions[:, 0], anchor_positions[:, 1], marker="^", s=200, c="blue", zorder=5, label="Anchors")

        ax.set_xlabel("x [m]")
        ax.set_ylabel("y [m]")
        ax.set_aspect("equal")
        ax.legend()
        ax.set_title("Position Estimation: True vs Estimated")
        ax.grid(True, alpha=0.3)

        if save_path:
            fig.savefig(save_path, dpi=150, bbox_inches="tight")
            print(f"Saved: {save_path}")
        else:
            plt.show()
        plt.close(fig)

    def plot_error_over_time(self, save_path: str | None = None) -> None:
        """Plot estimation error over time."""
        import matplotlib.pyplot as plt

        fig, ax = plt.subplots(1, 1, figsize=(10, 4))

        t = self.timestamps if self.timestamps is not None else np.arange(len(self.errors))
        ax.plot(t, self.errors, "b-", linewidth=0.8)
        ax.axhline(self.mae(), color="r", linestyle="--", label=f"MAE = {self.mae():.2f} m")
        ax.set_xlabel("Time [s]" if self.timestamps is not None else "Step")
        ax.set_ylabel("Error [m]")
        ax.set_title("Position Estimation Error Over Time")
        ax.legend()
        ax.grid(True, alpha=0.3)

        if save_path:
            fig.savefig(save_path, dpi=150, bbox_inches="tight")
            print(f"Saved: {save_path}")
        else:
            plt.show()
        plt.close(fig)

    def plot_cdf(self, save_path: str | None = None) -> None:
        """Plot cumulative distribution function of errors."""
        import matplotlib.pyplot as plt

        fig, ax = plt.subplots(1, 1, figsize=(8, 5))

        sorted_errors = np.sort(self.errors)
        cdf = np.arange(1, len(sorted_errors) + 1) / len(sorted_errors)

        ax.plot(sorted_errors, cdf, "b-", linewidth=2)
        ax.axvline(self.percentile(50), color="orange", linestyle="--", label=f"50th = {self.percentile(50):.2f} m")
        ax.axvline(self.percentile(95), color="red", linestyle="--", label=f"95th = {self.percentile(95):.2f} m")
        ax.set_xlabel("Error [m]")
        ax.set_ylabel("CDF")
        ax.set_title("Cumulative Distribution of Position Error")
        ax.legend()
        ax.grid(True, alpha=0.3)

        if save_path:
            fig.savefig(save_path, dpi=150, bbox_inches="tight")
            print(f"Saved: {save_path}")
        else:
            plt.show()
        plt.close(fig)

    def plot_error_heatmap(
        self,
        field_size: tuple[float, float] = (10.0, 10.0),
        grid_resolution: float = 0.5,
        save_path: str | None = None,
    ) -> None:
        """Plot spatial heatmap of errors (useful with grid trajectory)."""
        import matplotlib.pyplot as plt

        fig, ax = plt.subplots(1, 1, figsize=(8, 8))

        # Bin errors spatially
        nx = int(field_size[0] / grid_resolution) + 1
        ny = int(field_size[1] / grid_resolution) + 1
        error_grid = np.full((ny, nx), np.nan)
        count_grid = np.zeros((ny, nx))

        for i in range(len(self.true_pos)):
            xi = int(self.true_pos[i, 0] / grid_resolution)
            yi = int(self.true_pos[i, 1] / grid_resolution)
            xi = min(xi, nx - 1)
            yi = min(yi, ny - 1)
            if np.isnan(error_grid[yi, xi]):
                error_grid[yi, xi] = 0.0
            error_grid[yi, xi] += self.errors[i]
            count_grid[yi, xi] += 1

        mask = count_grid > 0
        error_grid[mask] /= count_grid[mask]

        im = ax.imshow(
            error_grid,
            origin="lower",
            extent=[0, field_size[0], 0, field_size[1]],
            cmap="hot",
            aspect="equal",
        )
        fig.colorbar(im, ax=ax, label="Error [m]")
        ax.set_xlabel("x [m]")
        ax.set_ylabel("y [m]")
        ax.set_title("Spatial Error Distribution")

        if save_path:
            fig.savefig(save_path, dpi=150, bbox_inches="tight")
            print(f"Saved: {save_path}")
        else:
            plt.show()
        plt.close(fig)
