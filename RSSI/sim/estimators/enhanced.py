"""Enhanced AI-corrected EKF — adds boundary clamp, adaptive R, and outlier gating."""

from __future__ import annotations

from collections import deque

import numpy as np

from ..config import AnchorConfig
from ..mlp import MLP
from .base import Estimator
from .ekf import EKFEstimator


class EnhancedAICorrectedEstimator(Estimator):
    """AI-corrected EKF enhanced with:
    - Field boundary constraints (state clamping)
    - Adaptive measurement noise R (based on RSSI std and correction magnitude)
    - Mahalanobis outlier gating (reject implausible observations)

    Target: MAE <= 0.8m
    """

    def __init__(
        self,
        anchors: list[AnchorConfig],
        dt: float = 0.2,
        history_len: int = 5,
        mlp_hidden: list[int] | None = None,
        mlp_lr: float = 0.005,
        online_lr: float = 0.001,
        online_cov_threshold: float = 5.0,
        process_noise_pos: float = 0.1,
        process_noise_vel: float = 1.0,
        measurement_noise_base: float = 2.0,
        field_size: tuple[float, float] = (10.0, 10.0),
        gate_threshold: float = 3.0,
        seed: int = 0,
    ) -> None:
        super().__init__(anchors)
        self.n_anchors = len(anchors)
        self.history_len = history_len
        self.online_cov_threshold = online_cov_threshold
        self.field_size = field_size
        self.gate_threshold = gate_threshold
        self.measurement_noise_base = measurement_noise_base

        # Feature dimensions
        n_pairs = self.n_anchors * (self.n_anchors - 1) // 2
        input_dim = 3 * self.n_anchors + n_pairs
        output_dim = self.n_anchors
        hidden = mlp_hidden or [48, 24]

        self.mlp = MLP(
            layer_sizes=[input_dim] + hidden + [output_dim],
            learning_rate=mlp_lr,
            weight_decay=0.001,
            seed=seed,
        )
        self.online_lr = online_lr

        # Internal EKF (custom update to support adaptive R and gating)
        self.dt = dt
        self.x = np.array([field_size[0] / 2, field_size[1] / 2, 0.0, 0.0])
        self.P = np.diag([10.0, 10.0, 1.0, 1.0])
        self.F = np.array([
            [1, 0, dt, 0],
            [0, 1, 0, dt],
            [0, 0, 1, 0],
            [0, 0, 0, 1],
        ])
        self.Q = np.diag([process_noise_pos, process_noise_pos,
                          process_noise_vel, process_noise_vel])

        # RSSI history
        self._history: dict[str, deque] = {
            a.anchor_id: deque(maxlen=history_len) for a in anchors
        }
        self._calibrated = False
        self._step_count = 0

    def calibrate(
        self,
        rssi_sequences: list[list[dict[str, float]]],
        known_positions: list[tuple[float, float]],
        epochs: int = 120,
        seed: int = 0,
    ) -> list[float]:
        """Offline calibration (identical to AICorrectedEKF)."""
        training_pairs: list[tuple[np.ndarray, np.ndarray]] = []

        for seq, pos in zip(rssi_sequences, known_positions):
            pos_arr = np.array(pos)
            ideal_rssi = np.array([
                a.rssi_d0 - 10.0 * a.n * np.log10(
                    max(np.linalg.norm(pos_arr - np.array([a.x, a.y])), 0.01) / a.d0
                )
                for a in self.anchors
            ])

            history: dict[str, deque] = {
                a.anchor_id: deque(maxlen=self.history_len) for a in self.anchors
            }
            for obs in seq:
                for a in self.anchors:
                    if a.anchor_id in obs:
                        history[a.anchor_id].append(obs[a.anchor_id])
                if all(len(history[a.anchor_id]) >= 3 for a in self.anchors):
                    features = self._build_features(obs, history)
                    raw_rssi = np.array([obs.get(a.anchor_id, -100.0) for a in self.anchors])
                    target_residual = ideal_rssi - raw_rssi
                    training_pairs.append((features, target_residual))

        if not training_pairs:
            return []

        loss_history = []
        initial_lr = self.mlp.lr
        train_rng = np.random.default_rng(seed)
        for epoch in range(epochs):
            self.mlp.lr = initial_lr * 0.5 * (1 + np.cos(np.pi * epoch / epochs))
            epoch_loss = 0.0
            indices = train_rng.permutation(len(training_pairs))
            for idx in indices:
                x, target = training_pairs[idx]
                loss = self.mlp.train_step(x, target)
                epoch_loss += loss
            loss_history.append(epoch_loss / len(training_pairs))

        self.mlp.lr = initial_lr
        self._calibrated = True
        return loss_history

    def update(self, rssi_obs: dict[str, float]) -> np.ndarray:
        self._step_count += 1

        # Update history
        for a in self.anchors:
            if a.anchor_id in rssi_obs:
                self._history[a.anchor_id].append(rssi_obs[a.anchor_id])

        # Build features and apply MLP correction
        features = self._build_features(rssi_obs, self._history)
        correction = self.mlp(features)
        correction = np.clip(correction, -15.0, 15.0)

        raw_rssi_arr = np.array([rssi_obs.get(a.anchor_id, -100.0) for a in self.anchors])
        corrected_rssi_arr = raw_rssi_arr + correction

        # --- EKF Prediction ---
        self.x = self.F @ self.x
        self.P = self.F @ self.P @ self.F.T + self.Q

        # --- Boundary clamp on predicted state ---
        self.x[0] = np.clip(self.x[0], 0, self.field_size[0])
        self.x[1] = np.clip(self.x[1], 0, self.field_size[1])

        # --- Prepare observations ---
        obs_distances = []
        obs_anchor_pos = []
        obs_sigmas = []
        obs_indices = []

        for i, anchor in enumerate(self.anchors):
            if anchor.anchor_id not in rssi_obs:
                continue
            d = self.rssi_to_distance(float(corrected_rssi_arr[i]), anchor)
            obs_distances.append(d)
            obs_anchor_pos.append(np.array([anchor.x, anchor.y]))
            obs_indices.append(i)

            # Adaptive sigma: higher std or larger correction → less reliable
            buf = list(self._history[anchor.anchor_id])
            std_val = float(np.std(buf)) if len(buf) >= 2 else 5.0
            corr_mag = abs(float(correction[i]))
            sigma = self.measurement_noise_base * (1.0 + 0.2 * std_val + 0.05 * corr_mag)
            obs_sigmas.append(sigma)

        if not obs_distances:
            return self.x[:2].copy()

        obs_distances = np.array(obs_distances)
        obs_sigmas = np.array(obs_sigmas)

        # --- Outlier gating ---
        pos_pred = self.x[:2]
        predicted_distances = np.array([
            np.linalg.norm(pos_pred - ap) for ap in obs_anchor_pos
        ])
        residuals = np.abs(obs_distances - predicted_distances)
        gate_mask = residuals < (self.gate_threshold * obs_sigmas)

        # If too few pass the gate, use all with inflated noise
        if gate_mask.sum() < 2:
            gate_mask[:] = True
            obs_sigmas *= 2.0

        # Filter to gated observations only
        m = int(gate_mask.sum())
        z = obs_distances[gate_mask]
        sigmas = obs_sigmas[gate_mask]
        anchors_gated = [obs_anchor_pos[j] for j in range(len(gate_mask)) if gate_mask[j]]

        # --- EKF Update with adaptive R ---
        h = np.zeros(m)
        H = np.zeros((m, 4))

        for i, anchor_pos in enumerate(anchors_gated):
            diff = pos_pred - anchor_pos
            dist_pred = np.linalg.norm(diff)
            dist_pred = max(dist_pred, 0.01)
            h[i] = dist_pred
            H[i, 0] = diff[0] / dist_pred
            H[i, 1] = diff[1] / dist_pred

        # Adaptive R matrix (diagonal, per-anchor)
        R = np.diag(sigmas ** 2)

        # Innovation
        y = z - h

        # Kalman gain
        S = H @ self.P @ H.T + R
        try:
            K = self.P @ H.T @ np.linalg.inv(S)
        except np.linalg.LinAlgError:
            return self.x[:2].copy()

        self.x = self.x + K @ y
        self.P = (np.eye(4) - K @ H) @ self.P

        # --- Post-update boundary clamp ---
        self.x[0] = np.clip(self.x[0], 0, self.field_size[0])
        self.x[1] = np.clip(self.x[1], 0, self.field_size[1])

        position = self.x[:2].copy()

        # Online adaptation
        if self._calibrated and self._step_count > self.history_len:
            self._online_adapt(features, position, raw_rssi_arr)

        return position

    def _build_features(
        self, current_obs: dict[str, float], history: dict[str, deque]
    ) -> np.ndarray:
        """Extract normalized feature vector with pairwise diffs."""
        current = []
        means = []
        stds = []

        for a in self.anchors:
            aid = a.anchor_id
            rssi = current_obs.get(aid, -100.0)
            current.append(rssi)
            buf = list(history.get(aid, []))
            if len(buf) >= 2:
                means.append(float(np.mean(buf)))
                stds.append(float(np.std(buf)))
            else:
                means.append(rssi)
                stds.append(0.0)

        pairwise = []
        for i in range(self.n_anchors):
            for j in range(i + 1, self.n_anchors):
                pairwise.append(current[i] - current[j])

        raw = np.array(current + means + stds + pairwise, dtype=np.float64)
        n = self.n_anchors
        n_pairs = len(pairwise)
        raw[:n] = (raw[:n] + 60.0) / 20.0
        raw[n:2*n] = (raw[n:2*n] + 60.0) / 20.0
        raw[2*n:3*n] = raw[2*n:3*n] / 5.0
        raw[3*n:3*n+n_pairs] = raw[3*n:3*n+n_pairs] / 20.0

        return raw

    def _online_adapt(self, features: np.ndarray, estimated_pos: np.ndarray, raw_rssi: np.ndarray) -> None:
        """Online SGD step using EKF estimate as pseudo-label."""
        cov_trace = np.trace(self.P[:2, :2])
        if cov_trace > self.online_cov_threshold:
            return

        ideal_rssi = np.array([
            a.rssi_d0 - 10.0 * a.n * np.log10(
                max(np.linalg.norm(estimated_pos - np.array([a.x, a.y])), 0.01) / a.d0
            )
            for a in self.anchors
        ])
        target_residual = ideal_rssi - raw_rssi

        original_lr = self.mlp.lr
        self.mlp.lr = self.online_lr
        self.mlp.train_step(features, target_residual)
        self.mlp.lr = original_lr

    def reset(self) -> None:
        self.x = np.array([self.field_size[0] / 2, self.field_size[1] / 2, 0.0, 0.0])
        self.P = np.diag([10.0, 10.0, 1.0, 1.0])
        self._history = {
            a.anchor_id: deque(maxlen=self.history_len) for a in self.anchors
        }
        self._step_count = 0
