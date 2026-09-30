"""AI-corrected EKF estimator — MLP RSSI correction + EKF positioning."""

from __future__ import annotations

from collections import deque

import numpy as np

from ..config import AnchorConfig
from ..mlp import MLP
from .base import Estimator
from .ekf import EKFEstimator


class AICorrectedEKF(Estimator):
    """EKF with a learned MLP front-end that corrects raw RSSI before distance conversion.

    Pipeline per update:
        raw RSSI -> feature extraction (current + stats) -> MLP correction -> EKF update

    Supports:
        - Offline calibration from known-position samples
        - Online adaptation using EKF estimates as pseudo-labels
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
        measurement_noise: float = 3.0,
        initial_position: tuple[float, float] = (5.0, 5.0),
        seed: int = 0,
    ) -> None:
        super().__init__(anchors)
        self.n_anchors = len(anchors)
        self.history_len = history_len
        self.online_cov_threshold = online_cov_threshold

        # Feature: [rssi×N, mean×N, std×N, pairwise_diff×C(N,2)] dimensions
        #   pairwise diffs: N*(N-1)/2 pairs
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

        # Internal EKF
        self.ekf = EKFEstimator(
            anchors,
            dt=dt,
            process_noise_pos=process_noise_pos,
            process_noise_vel=process_noise_vel,
            measurement_noise=measurement_noise,
            initial_position=initial_position,
        )

        # RSSI history ring buffer per anchor
        self._history: dict[str, deque] = {
            a.anchor_id: deque(maxlen=history_len) for a in anchors
        }
        self._calibrated = False
        self._step_count = 0

    def calibrate(
        self,
        rssi_sequences: list[list[dict[str, float]]],
        known_positions: list[tuple[float, float]],
        epochs: int = 50,
        seed: int = 0,
    ) -> list[float]:
        """Offline calibration from known-position measurements.

        Args:
            rssi_sequences: List of sequences, one per calibration point.
                Each sequence is a list of observation dicts over time.
            known_positions: Corresponding true (x, y) for each sequence.
            epochs: Training epochs over all calibration data.
            seed: Random seed for training shuffle reproducibility.

        Returns:
            Loss history (one value per epoch).
        """
        # Compute ideal RSSI targets for each known position
        training_pairs: list[tuple[np.ndarray, np.ndarray]] = []

        for seq, pos in zip(rssi_sequences, known_positions):
            pos_arr = np.array(pos)
            # Compute ideal RSSI for this position
            ideal_rssi = np.array([
                a.rssi_d0 - 10.0 * a.n * np.log10(
                    max(np.linalg.norm(pos_arr - np.array([a.x, a.y])), 0.01) / a.d0
                )
                for a in self.anchors
            ])

            # Build feature vectors from the sequence (need history_len warmup)
            history: dict[str, deque] = {
                a.anchor_id: deque(maxlen=self.history_len) for a in self.anchors
            }
            for obs in seq:
                for a in self.anchors:
                    if a.anchor_id in obs:
                        history[a.anchor_id].append(obs[a.anchor_id])

                # Only create training pairs after enough history
                if all(len(history[a.anchor_id]) >= 3 for a in self.anchors):
                    features = self._build_features_from(obs, history)
                    # Target is the residual: ideal - raw
                    raw_rssi = np.array([obs.get(a.anchor_id, -100.0) for a in self.anchors])
                    target_residual = ideal_rssi - raw_rssi
                    training_pairs.append((features, target_residual))

        if not training_pairs:
            return []

        # Train with learning rate decay
        loss_history = []
        initial_lr = self.mlp.lr
        train_rng = np.random.default_rng(seed)
        for epoch in range(epochs):
            # Cosine annealing LR schedule
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

        # Build features
        features = self._build_features_from(rssi_obs, self._history)

        # Apply MLP correction (residual: MLP outputs a small offset to add)
        correction = self.mlp(features)
        # Clamp correction to prevent extreme values
        correction = np.clip(correction, -15.0, 15.0)

        # Corrected RSSI = raw RSSI + learned offset
        raw_rssi_arr = np.array([
            rssi_obs.get(a.anchor_id, -100.0) for a in self.anchors
        ])
        corrected_rssi_arr = raw_rssi_arr + correction

        # Build corrected observation dict
        corrected_obs = {
            a.anchor_id: float(corrected_rssi_arr[i])
            for i, a in enumerate(self.anchors)
            if a.anchor_id in rssi_obs
        }

        # EKF update with corrected RSSI
        position = self.ekf.update(corrected_obs)

        # Online adaptation (if calibrated and EKF is confident)
        if self._calibrated and self._step_count > self.history_len:
            self._online_adapt(features, position, raw_rssi_arr)

        return position

    def _build_features_from(
        self, current_obs: dict[str, float], history: dict[str, deque]
    ) -> np.ndarray:
        """Extract normalized [rssi_current, mean, std, pairwise_diff] feature vector."""
        current = []
        means = []
        stds = []

        for a in self.anchors:
            aid = a.anchor_id
            rssi = current_obs.get(aid, -100.0)  # fallback for missing
            current.append(rssi)

            buf = list(history.get(aid, []))
            if len(buf) >= 2:
                means.append(float(np.mean(buf)))
                stds.append(float(np.std(buf)))
            else:
                means.append(rssi)
                stds.append(0.0)

        # Pairwise RSSI differences (position signature)
        pairwise = []
        for i in range(self.n_anchors):
            for j in range(i + 1, self.n_anchors):
                pairwise.append(current[i] - current[j])

        # Normalize: center RSSI around -60 dBm, scale by 20; std scaled by 5; diffs by 20
        raw = np.array(current + means + stds + pairwise, dtype=np.float64)
        n = self.n_anchors
        n_pairs = len(pairwise)
        raw[:n] = (raw[:n] + 60.0) / 20.0        # current rssi
        raw[n:2*n] = (raw[n:2*n] + 60.0) / 20.0  # mean rssi
        raw[2*n:3*n] = raw[2*n:3*n] / 5.0         # std
        raw[3*n:3*n+n_pairs] = raw[3*n:3*n+n_pairs] / 20.0  # pairwise diffs

        return raw

    def _online_adapt(self, features: np.ndarray, estimated_pos: np.ndarray, raw_rssi: np.ndarray) -> None:
        """Online SGD step using EKF estimate as pseudo-label."""
        # Check EKF confidence via covariance trace
        cov_trace = np.trace(self.ekf.P[:2, :2])
        if cov_trace > self.online_cov_threshold:
            return  # Too uncertain, skip

        # Compute ideal RSSI from estimated position
        ideal_rssi = np.array([
            a.rssi_d0 - 10.0 * a.n * np.log10(
                max(np.linalg.norm(estimated_pos - np.array([a.x, a.y])), 0.01) / a.d0
            )
            for a in self.anchors
        ])

        # Target residual: ideal - raw
        target_residual = ideal_rssi - raw_rssi

        # SGD step with reduced learning rate
        original_lr = self.mlp.lr
        self.mlp.lr = self.online_lr
        self.mlp.train_step(features, target_residual)
        self.mlp.lr = original_lr

    def reset(self) -> None:
        self.ekf.reset()
        self._history = {
            a.anchor_id: deque(maxlen=self.history_len) for a in self.anchors
        }
        self._step_count = 0
