"""Temporal RSSI-denoising correctors — pure numpy, micro:bit portable.

Both correctors take a normalized RSSI *window* of shape (T, N) (T timesteps,
N anchors) and predict a residual correction (N,) for the *current* (last)
timestep. They share a common interface so the estimator can swap backends:

    corrector.lr                      # settable learning rate
    corrector(window) -> (N,)         # forward (residual)
    corrector.train_step(window, target) -> loss

Backends
--------
WindowMLPCorrector : flattens the raw window (T*N,) into a plain MLP.
    Tests the hypothesis "the raw window carries more info than mean/std".
TCNCorrector       : 2-layer causal temporal convolution over the window.
    Tests whether *temporal structure* (colored noise, shadowing dynamics)
    can be exploited beyond simple averaging.
"""

from __future__ import annotations

import numpy as np

from .mlp import MLP


class WindowMLPCorrector:
    """MLP over a flattened raw RSSI window.

    Input  : window (T, N) -> flattened (T*N,)
    Output : residual correction (N,)
    """

    def __init__(
        self,
        in_channels: int,
        out_channels: int,
        window: int,
        hidden: list[int] | None = None,
        learning_rate: float = 0.005,
        weight_decay: float = 0.001,
        seed: int = 0,
    ) -> None:
        self.in_channels = in_channels
        self.out_channels = out_channels
        self.window = window
        hidden = hidden or [48, 24]
        input_dim = window * in_channels
        self.mlp = MLP(
            layer_sizes=[input_dim] + hidden + [out_channels],
            learning_rate=learning_rate,
            weight_decay=weight_decay,
            seed=seed,
        )

    @property
    def lr(self) -> float:
        return self.mlp.lr

    @lr.setter
    def lr(self, value: float) -> None:
        self.mlp.lr = value

    def __call__(self, window: np.ndarray) -> np.ndarray:
        return self.mlp(window.reshape(-1))

    def train_step(self, window: np.ndarray, target: np.ndarray) -> float:
        return self.mlp.train_step(window.reshape(-1), target)


class _Conv1DCausal:
    """Causal 1D convolution layer with manual backprop.

    Input  x : (C_in, T)
    Output y : (C_out, T),  y[o,t] = b[o] + sum_{i,k} W[o,i,k] * x[i, t-k]
    (x[:, <0] treated as 0 -> left/causal padding)
    """

    def __init__(self, in_ch: int, out_ch: int, kernel: int, rng: np.random.Generator) -> None:
        self.in_ch = in_ch
        self.out_ch = out_ch
        self.K = kernel
        scale = np.sqrt(2.0 / (in_ch * kernel + out_ch))
        self.W = rng.normal(0, scale, (out_ch, in_ch, kernel))
        self.b = np.zeros(out_ch)
        self._x: np.ndarray | None = None
        self.dW = np.zeros_like(self.W)
        self.db = np.zeros_like(self.b)

    @staticmethod
    def _shift_right(x: np.ndarray, k: int) -> np.ndarray:
        """x[i, t-k] with zero padding for t-k < 0."""
        if k == 0:
            return x
        xk = np.zeros_like(x)
        xk[:, k:] = x[:, :-k]
        return xk

    def forward(self, x: np.ndarray) -> np.ndarray:
        self._x = x
        T = x.shape[1]
        y = np.zeros((self.out_ch, T))
        for k in range(self.K):
            xk = self._shift_right(x, k)      # (C_in, T)
            y += self.W[:, :, k] @ xk         # (C_out, T)
        y += self.b[:, None]
        return y

    def backward(self, grad_y: np.ndarray) -> np.ndarray:
        """Accumulate dW/db, return grad wrt input x (C_in, T)."""
        x = self._x
        assert x is not None
        self.db = grad_y.sum(axis=1)
        self.dW = np.zeros_like(self.W)
        dx = np.zeros_like(x)
        for k in range(self.K):
            xk = self._shift_right(x, k)               # (C_in, T)
            self.dW[:, :, k] = grad_y @ xk.T           # (C_out, C_in)
            contrib = self.W[:, :, k].T @ grad_y       # (C_in, T) = dL/dxk
            # xk[i,t] == x[i, t-k]  ->  shift contribution left by k
            if k == 0:
                dx += contrib
            else:
                dx[:, :-k] += contrib[:, k:]
        return dx

    def sgd_update(self, lr: float, weight_decay: float, grad_clip: float) -> None:
        dW = self.dW
        norm = np.linalg.norm(dW)
        if norm > grad_clip:
            dW = dW * (grad_clip / norm)
        db = self.db
        dbn = np.linalg.norm(db)
        if dbn > grad_clip:
            db = db * (grad_clip / dbn)
        self.W -= lr * (dW + weight_decay * self.W)
        self.b -= lr * db


class TCNCorrector:
    """2-layer causal TCN denoiser over an RSSI window.

    conv1 (N -> H, K) -> ReLU -> conv2 (H -> N, K) -> linear
    The residual is read from the *last* timestep of conv2's output.

    Input  : window (T, N)  (time-major; internally transposed to (N, T))
    Output : residual (N,)
    """

    def __init__(
        self,
        in_channels: int,
        out_channels: int,
        window: int,
        hidden_ch: int = 12,
        kernel: int = 3,
        learning_rate: float = 0.005,
        weight_decay: float = 0.001,
        grad_clip: float = 5.0,
        seed: int = 0,
    ) -> None:
        self.in_channels = in_channels
        self.out_channels = out_channels
        self.window = window
        self.lr = learning_rate
        self.weight_decay = weight_decay
        self.grad_clip = grad_clip
        rng = np.random.default_rng(seed)
        self.conv1 = _Conv1DCausal(in_channels, hidden_ch, kernel, rng)
        self.conv2 = _Conv1DCausal(hidden_ch, out_channels, kernel, rng)
        self._relu_mask: np.ndarray | None = None

    def _forward(self, window: np.ndarray) -> np.ndarray:
        x = window.T                          # (N, T)
        z1 = self.conv1.forward(x)            # (H, T)
        a1 = np.maximum(0.0, z1)
        self._relu_mask = (z1 > 0).astype(np.float64)
        self._a1 = a1
        y2 = self.conv2.forward(a1)           # (N, T)
        self._y2 = y2
        return y2[:, -1]                       # residual at current timestep

    def __call__(self, window: np.ndarray) -> np.ndarray:
        return self._forward(window)

    def train_step(self, window: np.ndarray, target: np.ndarray) -> float:
        pred = self._forward(window)
        loss = float(np.mean((pred - target) ** 2))

        T = window.shape[0]
        # Gradient only flows from the last timestep of the output.
        grad_y2 = np.zeros_like(self._y2)     # (N, T)
        grad_y2[:, -1] = 2.0 * (pred - target) / len(target)

        grad_a1 = self.conv2.backward(grad_y2)        # (H, T)
        grad_z1 = grad_a1 * self._relu_mask           # ReLU'
        self.conv1.backward(grad_z1)

        self.conv2.sgd_update(self.lr, self.weight_decay, self.grad_clip)
        self.conv1.sgd_update(self.lr, self.weight_decay, self.grad_clip)
        return loss
