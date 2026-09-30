"""Weighted trilateration estimator."""

from __future__ import annotations

import numpy as np

from ..config import AnchorConfig
from .base import Estimator


class WeightedTrilateration(Estimator):
    """Position estimation via weighted least-squares trilateration.

    Converts RSSI to distances, then solves the linearized trilateration
    system using weighted least squares where weights are inversely
    proportional to estimated distance (closer anchors are more reliable).
    """

    def __init__(self, anchors: list[AnchorConfig]) -> None:
        super().__init__(anchors)

    def update(self, rssi_obs: dict[str, float]) -> np.ndarray:
        # Collect distances and corresponding anchor positions
        distances = []
        positions = []
        weights = []

        for anchor in self.anchors:
            if anchor.anchor_id not in rssi_obs:
                continue
            rssi = rssi_obs[anchor.anchor_id]
            d = self.rssi_to_distance(rssi, anchor)
            distances.append(d)
            positions.append([anchor.x, anchor.y])
            # Weight: inverse of distance squared (closer = more reliable)
            weights.append(1.0 / max(d**2, 0.01))

        distances = np.array(distances)
        positions = np.array(positions)
        weights = np.array(weights)

        n = len(distances)
        if n < 3:
            # Not enough anchors — return centroid weighted by inverse distance
            if n == 0:
                return np.array([5.0, 5.0])  # fallback
            w_norm = weights / weights.sum()
            return (positions.T @ w_norm).flatten()

        # Linearize trilateration:
        # ||p - a_i||^2 = d_i^2
        # x^2 - 2*a_ix*x + a_ix^2 + y^2 - 2*a_iy*y + a_iy^2 = d_i^2
        # Subtract last equation from all others to linearize:
        # 2*(a_n_x - a_i_x)*x + 2*(a_n_y - a_i_y)*y = d_i^2 - d_n^2 - a_ix^2 - a_iy^2 + a_nx^2 + a_ny^2

        ref_idx = n - 1
        A = np.zeros((n - 1, 2))
        b = np.zeros(n - 1)
        W = np.zeros((n - 1, n - 1))

        for i in range(n - 1):
            A[i, 0] = 2.0 * (positions[ref_idx, 0] - positions[i, 0])
            A[i, 1] = 2.0 * (positions[ref_idx, 1] - positions[i, 1])
            b[i] = (
                distances[i] ** 2
                - distances[ref_idx] ** 2
                - positions[i, 0] ** 2
                - positions[i, 1] ** 2
                + positions[ref_idx, 0] ** 2
                + positions[ref_idx, 1] ** 2
            )
            W[i, i] = weights[i]

        # Weighted least squares: (A^T W A)^-1 A^T W b
        AtW = A.T @ W
        AtWA = AtW @ A
        AtWb = AtW @ b

        try:
            pos_est = np.linalg.solve(AtWA, AtWb)
        except np.linalg.LinAlgError:
            # Singular matrix fallback
            pos_est = np.linalg.lstsq(A, b, rcond=None)[0]

        return pos_est
