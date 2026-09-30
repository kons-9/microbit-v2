"""Lightweight MLP for RSSI correction — pure numpy, no framework dependency."""

from __future__ import annotations

import numpy as np


class MLP:
    """Simple feedforward neural network with ReLU hidden layers.

    Architecture: input -> [hidden_1, ReLU] -> ... -> [hidden_n, ReLU] -> output (linear)
    """

    def __init__(
        self,
        layer_sizes: list[int],
        learning_rate: float = 0.001,
        grad_clip: float = 5.0,
        weight_decay: float = 0.0,
        seed: int = 0,
    ) -> None:
        """
        Args:
            layer_sizes: e.g. [12, 32, 16, 4] for 12-input, 2 hidden, 4-output.
            learning_rate: SGD step size.
            grad_clip: Maximum gradient norm per parameter matrix.
            weight_decay: L2 regularization coefficient.
            seed: Random seed for weight initialization.
        """
        self.lr = learning_rate
        self.grad_clip = grad_clip
        self.weight_decay = weight_decay
        rng = np.random.default_rng(seed)

        # Xavier initialization
        self.weights: list[np.ndarray] = []
        self.biases: list[np.ndarray] = []
        for i in range(len(layer_sizes) - 1):
            fan_in = layer_sizes[i]
            fan_out = layer_sizes[i + 1]
            scale = np.sqrt(2.0 / (fan_in + fan_out))
            self.weights.append(rng.normal(0, scale, (fan_in, fan_out)))
            self.biases.append(np.zeros(fan_out))

    def forward(self, x: np.ndarray) -> np.ndarray:
        """Forward pass. Stores activations for backprop.

        Args:
            x: Input array, shape (input_dim,).

        Returns:
            Output array, shape (output_dim,).
        """
        self._activations = [x.copy()]
        self._pre_activations = []

        h = x
        for i, (W, b) in enumerate(zip(self.weights, self.biases)):
            z = h @ W + b
            self._pre_activations.append(z)
            if i < len(self.weights) - 1:  # ReLU for hidden layers
                h = np.maximum(0, z)
            else:  # Linear output
                h = z
            self._activations.append(h)

        return h

    def backward(self, target: np.ndarray) -> float:
        """Backward pass with MSE loss, followed by SGD weight update.

        Args:
            target: Target output, shape (output_dim,).

        Returns:
            MSE loss value.
        """
        output = self._activations[-1]
        loss = float(np.mean((output - target) ** 2))

        # Output layer gradient (MSE derivative)
        grad = 2.0 * (output - target) / len(target)

        for i in range(len(self.weights) - 1, -1, -1):
            if i < len(self.weights) - 1:
                # ReLU derivative
                grad = grad * (self._pre_activations[i] > 0).astype(float)

            # Parameter gradients
            dW = np.outer(self._activations[i], grad)
            db = grad

            # Gradient clipping (per-parameter max norm)
            dW_norm = np.linalg.norm(dW)
            if dW_norm > self.grad_clip:
                dW = dW * (self.grad_clip / dW_norm)
            db_norm = np.linalg.norm(db)
            if db_norm > self.grad_clip:
                db = db * (self.grad_clip / db_norm)

            # Propagate gradient to previous layer
            grad = grad @ self.weights[i].T

            # SGD update with weight decay (L2 regularization)
            self.weights[i] -= self.lr * (dW + self.weight_decay * self.weights[i])
            self.biases[i] -= self.lr * db

        return loss

    def train_step(self, x: np.ndarray, target: np.ndarray) -> float:
        """Single forward + backward step.

        Args:
            x: Input, shape (input_dim,).
            target: Target output, shape (output_dim,).

        Returns:
            MSE loss.
        """
        self.forward(x)
        return self.backward(target)

    def __call__(self, x: np.ndarray) -> np.ndarray:
        return self.forward(x)
