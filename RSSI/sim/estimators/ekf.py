"""Extended Kalman Filter estimator for RSSI-based positioning."""

from __future__ import annotations

import numpy as np

from ..config import AnchorConfig
from .base import Estimator


class EKFEstimator(Estimator):
    """EKF with constant-velocity motion model and RSSI distance observations.

    State vector: [x, y, vx, vy]
    Observation: distances to each anchor (derived from RSSI)
    """

    def __init__(
        self,
        anchors: list[AnchorConfig],
        dt: float = 0.1,
        process_noise_pos: float = 0.1,
        process_noise_vel: float = 1.0,
        measurement_noise: float = 3.0,
        initial_position: tuple[float, float] = (5.0, 5.0),
    ) -> None:
        super().__init__(anchors)
        self.dt = dt

        # State: [x, y, vx, vy]
        self.x = np.array([initial_position[0], initial_position[1], 0.0, 0.0])

        # State covariance
        self.P = np.diag([10.0, 10.0, 1.0, 1.0])

        # State transition matrix (constant velocity)
        self.F = np.array([
            [1, 0, dt, 0],
            [0, 1, 0, dt],
            [0, 0, 1, 0],
            [0, 0, 0, 1],
        ])

        # Process noise covariance
        self.Q = np.diag([
            process_noise_pos,
            process_noise_pos,
            process_noise_vel,
            process_noise_vel,
        ])

        # Measurement noise variance (per anchor)
        self.R_scalar = measurement_noise**2

    def update(self, rssi_obs: dict[str, float]) -> np.ndarray:
        # --- Prediction step ---
        self.x = self.F @ self.x
        self.P = self.F @ self.P @ self.F.T + self.Q

        # --- Update step ---
        # Collect valid observations
        obs_distances = []
        obs_anchors = []

        for anchor in self.anchors:
            if anchor.anchor_id not in rssi_obs:
                continue
            rssi = rssi_obs[anchor.anchor_id]
            d = self.rssi_to_distance(rssi, anchor)
            obs_distances.append(d)
            obs_anchors.append(np.array([anchor.x, anchor.y]))

        if not obs_distances:
            return self.x[:2].copy()

        m = len(obs_distances)
        z = np.array(obs_distances)

        # Predicted distances and Jacobian
        h = np.zeros(m)
        H = np.zeros((m, 4))

        pos_pred = self.x[:2]

        for i, anchor_pos in enumerate(obs_anchors):
            diff = pos_pred - anchor_pos
            dist_pred = np.linalg.norm(diff)
            dist_pred = max(dist_pred, 0.01)

            h[i] = dist_pred
            H[i, 0] = diff[0] / dist_pred
            H[i, 1] = diff[1] / dist_pred
            # H[i, 2] = 0, H[i, 3] = 0 (velocity doesn't affect distance directly)

        # Measurement noise matrix
        R = np.eye(m) * self.R_scalar

        # Innovation
        y = z - h

        # Kalman gain
        S = H @ self.P @ H.T + R
        try:
            K = self.P @ H.T @ np.linalg.inv(S)
        except np.linalg.LinAlgError:
            return self.x[:2].copy()

        # State update
        self.x = self.x + K @ y
        self.P = (np.eye(4) - K @ H) @ self.P

        return self.x[:2].copy()

    def reset(self) -> None:
        self.x = np.array([5.0, 5.0, 0.0, 0.0])
        self.P = np.diag([10.0, 10.0, 1.0, 1.0])
