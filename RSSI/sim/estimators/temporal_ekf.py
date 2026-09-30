"""Temporal-window AI-corrected EKF — raw RSSI window denoising front-end.

Unlike :class:`AICorrectedEKF` (which summarizes history as mean/std features),
this estimator feeds a *full raw RSSI window* (T timesteps x N anchors) into a
temporal corrector, letting the model learn its own temporal filter.

Backends (see ``sim.temporal``):
    "window_mlp" : flattened-window MLP  (A)
    "tcn"        : causal temporal conv  (B)

Everything downstream (residual learning, EKF, online adaptation) mirrors
:class:`AICorrectedEKF` so that A/B comparisons isolate the *input representation
and temporal model* rather than confounding filter changes.
"""

from __future__ import annotations

from collections import deque

import numpy as np

from ..config import AnchorConfig
from ..temporal import TCNCorrector, WindowMLPCorrector
from .base import Estimator
from .ekf import EKFEstimator


class TemporalAICorrectedEKF(Estimator):
    """EKF with a temporal-window RSSI denoiser front-end."""

    def __init__(
        self,
        anchors: list[AnchorConfig],
        dt: float = 0.2,
        window: int = 8,
        backend: str = "window_mlp",
        mlp_hidden: list[int] | None = None,
        tcn_hidden_ch: int = 12,
        tcn_kernel: int = 3,
        corrector_lr: float = 0.005,
        weight_decay: float = 0.001,
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
        self.window = window
        self.backend = backend
        self.online_lr = online_lr
        self.online_cov_threshold = online_cov_threshold

        n_pairs = self.n_anchors * (self.n_anchors - 1) // 2
        in_channels = self.n_anchors + n_pairs  # raw RSSI + pairwise diffs per timestep
        self._n_pairs = n_pairs

        if backend == "window_mlp":
            self.corrector = WindowMLPCorrector(
                in_channels=in_channels,
                out_channels=self.n_anchors,
                window=window,
                hidden=mlp_hidden or [48, 24],
                learning_rate=corrector_lr,
                weight_decay=weight_decay,
                seed=seed,
            )
        elif backend == "tcn":
            self.corrector = TCNCorrector(
                in_channels=in_channels,
                out_channels=self.n_anchors,
                window=window,
                hidden_ch=tcn_hidden_ch,
                kernel=tcn_kernel,
                learning_rate=corrector_lr,
                weight_decay=weight_decay,
                seed=seed,
            )
        else:
            raise ValueError(f"unknown backend: {backend!r}")

        self.ekf = EKFEstimator(
            anchors,
            dt=dt,
            process_noise_pos=process_noise_pos,
            process_noise_vel=process_noise_vel,
            measurement_noise=measurement_noise,
            initial_position=initial_position,
        )

        self._history: dict[str, deque] = {
            a.anchor_id: deque(maxlen=window) for a in anchors
        }
        self._calibrated = False
        self._step_count = 0

    # ------------------------------------------------------------------
    # Feature (window) construction
    # ------------------------------------------------------------------
    def _build_window(self, history: dict[str, deque]) -> np.ndarray:
        """Normalized RSSI window with per-timestep pairwise diffs.

        Shape (window, n_anchors + n_pairs). Left-padded (front) by repeating the
        oldest available sample so the window always has fixed length; the *last*
        row is the current sample. Channels: normalized RSSI (N) then normalized
        pairwise differences (n_pairs) reproducing the baseline's spatial signature.
        """
        cols = []
        for a in self.anchors:
            buf = list(history.get(a.anchor_id, []))
            if not buf:
                buf = [-100.0]
            if len(buf) < self.window:
                buf = [buf[0]] * (self.window - len(buf)) + buf
            cols.append(buf[-self.window:])
        rssi = np.array(cols, dtype=np.float64).T          # (window, N)

        # Per-timestep pairwise differences (spatial signature over time)
        pair_cols = []
        for i in range(self.n_anchors):
            for j in range(i + 1, self.n_anchors):
                pair_cols.append(rssi[:, i] - rssi[:, j])
        pairs = np.array(pair_cols, dtype=np.float64).T if pair_cols else np.zeros((self.window, 0))

        rssi_norm = (rssi + 60.0) / 20.0
        pairs_norm = pairs / 20.0
        return np.concatenate([rssi_norm, pairs_norm], axis=1)  # (window, N + n_pairs)

    def _ideal_rssi(self, pos: np.ndarray) -> np.ndarray:
        return np.array([
            a.rssi_d0 - 10.0 * a.n * np.log10(
                max(np.linalg.norm(pos - np.array([a.x, a.y])), 0.01) / a.d0
            )
            for a in self.anchors
        ])

    # ------------------------------------------------------------------
    # Calibration
    # ------------------------------------------------------------------
    def calibrate(
        self,
        rssi_sequences: list[list[dict[str, float]]],
        known_positions: list[tuple[float, float]],
        epochs: int = 120,
        seed: int = 0,
    ) -> list[float]:
        training_pairs: list[tuple[np.ndarray, np.ndarray]] = []

        for seq, pos in zip(rssi_sequences, known_positions):
            ideal_rssi = self._ideal_rssi(np.array(pos))
            history: dict[str, deque] = {
                a.anchor_id: deque(maxlen=self.window) for a in self.anchors
            }
            for obs in seq:
                for a in self.anchors:
                    if a.anchor_id in obs:
                        history[a.anchor_id].append(obs[a.anchor_id])
                if all(len(history[a.anchor_id]) >= 3 for a in self.anchors):
                    win = self._build_window(history)
                    raw_rssi = np.array([obs.get(a.anchor_id, -100.0) for a in self.anchors])
                    training_pairs.append((win, ideal_rssi - raw_rssi))

        if not training_pairs:
            return []

        loss_history: list[float] = []
        initial_lr = self.corrector.lr
        train_rng = np.random.default_rng(seed)
        for epoch in range(epochs):
            self.corrector.lr = initial_lr * 0.5 * (1 + np.cos(np.pi * epoch / epochs))
            epoch_loss = 0.0
            for idx in train_rng.permutation(len(training_pairs)):
                win, target = training_pairs[idx]
                epoch_loss += self.corrector.train_step(win, target)
            loss_history.append(epoch_loss / len(training_pairs))

        self.corrector.lr = initial_lr
        self._calibrated = True
        return loss_history

    # ------------------------------------------------------------------
    # Online update
    # ------------------------------------------------------------------
    def update(self, rssi_obs: dict[str, float]) -> np.ndarray:
        self._step_count += 1
        for a in self.anchors:
            if a.anchor_id in rssi_obs:
                self._history[a.anchor_id].append(rssi_obs[a.anchor_id])

        win = self._build_window(self._history)
        correction = np.clip(self.corrector(win), -15.0, 15.0)

        raw_rssi_arr = np.array([rssi_obs.get(a.anchor_id, -100.0) for a in self.anchors])
        corrected_rssi_arr = raw_rssi_arr + correction

        corrected_obs = {
            a.anchor_id: float(corrected_rssi_arr[i])
            for i, a in enumerate(self.anchors)
            if a.anchor_id in rssi_obs
        }

        position = self.ekf.update(corrected_obs)

        if self._calibrated and self._step_count > self.window:
            self._online_adapt(win, position, raw_rssi_arr)

        return position

    def _online_adapt(self, win: np.ndarray, estimated_pos: np.ndarray, raw_rssi: np.ndarray) -> None:
        if np.trace(self.ekf.P[:2, :2]) > self.online_cov_threshold:
            return
        target_residual = self._ideal_rssi(estimated_pos) - raw_rssi
        original_lr = self.corrector.lr
        self.corrector.lr = self.online_lr
        self.corrector.train_step(win, target_residual)
        self.corrector.lr = original_lr

    def reset(self) -> None:
        self.ekf.reset()
        self._history = {
            a.anchor_id: deque(maxlen=self.window) for a in self.anchors
        }
        self._step_count = 0
