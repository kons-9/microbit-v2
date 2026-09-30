"""Base interface for position estimators."""

from __future__ import annotations

from abc import ABC, abstractmethod

import numpy as np

from ..config import AnchorConfig


class Estimator(ABC):
    """Abstract base class for all position estimators."""

    def __init__(self, anchors: list[AnchorConfig]) -> None:
        self.anchors = anchors
        self.anchor_positions = np.array([[a.x, a.y] for a in anchors])
        self.anchor_ids = [a.anchor_id for a in anchors]

    @abstractmethod
    def update(self, rssi_obs: dict[str, float]) -> np.ndarray:
        """Process one observation and return estimated position (x, y).

        Args:
            rssi_obs: Dict mapping anchor_id -> RSSI [dBm].

        Returns:
            Estimated position as np.ndarray of shape (2,).
        """
        ...

    def reset(self) -> None:
        """Reset internal state (for filters with memory)."""
        pass

    def rssi_to_distance(self, rssi: float, anchor: AnchorConfig) -> float:
        """Convert RSSI to estimated distance using path-loss model inverse."""
        exponent = (anchor.rssi_d0 - rssi) / (10.0 * anchor.n)
        return anchor.d0 * (10.0 ** exponent)
