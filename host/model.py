"""
Position estimation ML model.

Uses RSSI-weighted centroid as baseline, with optional
k-NN fingerprinting when training data is available.
"""
import numpy as np


class PositionEstimator:
    """RSSI-based indoor position estimation.

    Default mode: weighted centroid (no training data needed).
    The weight of each beacon is proportional to 10^(RSSI/20),
    approximating inverse-square power vs. distance.
    """

    def __init__(self):
        self.fingerprint_db = None  # [(rssi_vector, x, y), ...]

    def estimate(
        self,
        rssi: np.ndarray,
        beacon_x: np.ndarray,
        beacon_y: np.ndarray,
    ) -> tuple[float, float, float]:
        """Estimate position from RSSI values and beacon positions.

        Returns (x, y, confidence).
        """
        if self.fingerprint_db is not None and len(self.fingerprint_db) > 0:
            return self._knn_estimate(rssi)
        return self._weighted_centroid(rssi, beacon_x, beacon_y)

    def _weighted_centroid(
        self,
        rssi: np.ndarray,
        beacon_x: np.ndarray,
        beacon_y: np.ndarray,
    ) -> tuple[float, float, float]:
        """Weighted centroid using RSSI power-based weights."""
        # Convert RSSI (dBm) to linear power scale for weighting
        # RSSI is negative; closer = higher (less negative)
        # Weight = 10^(RSSI/20) gives power-proportional weight
        weights = np.power(10.0, rssi.astype(np.float64) / 20.0)

        # Filter out very weak signals (RSSI < -95 dBm)
        valid = rssi > -95
        if not np.any(valid):
            # All signals too weak — return room center with low confidence
            cx = float(np.mean(beacon_x))
            cy = float(np.mean(beacon_y))
            return cx, cy, 0.1

        w = weights[valid]
        bx = beacon_x[valid]
        by = beacon_y[valid]

        w_sum = np.sum(w)
        x = float(np.sum(w * bx) / w_sum)
        y = float(np.sum(w * by) / w_sum)

        # Confidence based on signal spread and strength
        n_valid = int(np.sum(valid))
        avg_rssi = float(np.mean(rssi[valid]))
        # More beacons + stronger signal = higher confidence
        conf = min(1.0, (n_valid / len(rssi)) * (1.0 - abs(avg_rssi) / 100.0) * 2.0)
        conf = max(0.1, conf)

        return x, y, conf

    def _knn_estimate(self, rssi: np.ndarray) -> tuple[float, float, float]:
        """k-NN fingerprinting estimation (requires training data)."""
        k = min(3, len(self.fingerprint_db))
        distances = []
        for fp_rssi, fp_x, fp_y in self.fingerprint_db:
            dist = float(np.linalg.norm(rssi.astype(float) - fp_rssi.astype(float)))
            distances.append((dist, fp_x, fp_y))
        distances.sort(key=lambda t: t[0])

        top_k = distances[:k]
        # Inverse-distance weighting
        weights = [1.0 / (d[0] + 1e-6) for d in top_k]
        w_sum = sum(weights)
        x = sum(w * t[1] for w, t in zip(weights, top_k)) / w_sum
        y = sum(w * t[2] for w, t in zip(weights, top_k)) / w_sum

        # Confidence inversely proportional to mean distance
        mean_dist = np.mean([d[0] for d in top_k])
        conf = max(0.1, min(1.0, 1.0 / (1.0 + mean_dist / 10.0)))

        return float(x), float(y), float(conf)

    def add_fingerprint(self, rssi: np.ndarray, x: float, y: float):
        """Add a training fingerprint (RSSI vector at known position)."""
        if self.fingerprint_db is None:
            self.fingerprint_db = []
        self.fingerprint_db.append((rssi.copy(), x, y))
