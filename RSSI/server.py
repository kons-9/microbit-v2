"""HTTP RSSI receiver and online position estimator.

The server accepts either the ESP32-C3 scanner payload::

    {"address":"...", "rssi":-55, "data":"..."}

or an already decoded observation batch::

    {"observations":{"anchor_1":-55, "anchor_2":-62}}

The scanner's BLE advertisement contains a little-endian company ID 0x1234
followed by the six-byte anchor ID. Configure the mapping with ``--anchor``.
"""

from __future__ import annotations

import argparse
import json
import threading
import time
from dataclasses import dataclass
from http import HTTPStatus
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Any

try:
    from .sim.config import AnchorConfig
    from .sim.estimators.enhanced import EnhancedAICorrectedEstimator
except ImportError:
    from sim.config import AnchorConfig
    from sim.estimators.enhanced import EnhancedAICorrectedEstimator


COMPANY_ID = b"\x34\x12"
DEFAULT_WINDOW_MS = 500


def normalize_ble_id(value: str) -> str:
    """Normalize ``aa:bb:...`` or ``aabb...`` into lowercase colon form."""
    compact = value.replace(":", "").replace("-", "").strip().lower()
    if len(compact) != 12 or any(char not in "0123456789abcdef" for char in compact):
        raise ValueError(f"invalid BLE ID: {value}")
    return ":".join(compact[index:index + 2] for index in range(0, 12, 2))


def parse_anchor(value: str) -> tuple[str, str]:
    """Parse ``anchor_id=BLE_ID`` from the command line."""
    anchor_id, separator, ble_id = value.partition("=")
    if not separator or not anchor_id.strip():
        raise argparse.ArgumentTypeError("anchor must use anchor_id=BLE_ID")
    try:
        return anchor_id.strip(), normalize_ble_id(ble_id)
    except ValueError as error:
        raise argparse.ArgumentTypeError(str(error)) from error


def extract_anchor_id(data_hex: str) -> str | None:
    """Extract the six-byte locator ID from BLE AD structures."""
    try:
        data = bytes.fromhex(data_hex)
    except ValueError:
        return None

    index = 0
    while index < len(data):
        field_length = data[index]
        if field_length == 0:
            break
        field_end = index + 1 + field_length
        if field_end > len(data) or field_length < 3:
            break
        field_type = data[index + 1]
        if field_type == 0xff:
            field_data = data[index + 2:field_end]
            if field_data[:2] == COMPANY_ID and len(field_data) >= 8:
                return normalize_ble_id(field_data[2:8].hex())
        index = field_end
    return None


def _rssi(value: Any) -> float:
    result = float(value)
    if not -127.0 <= result <= 20.0:
        raise ValueError("RSSI must be between -127 and 20 dBm")
    return result


@dataclass(frozen=True)
class Estimate:
    x: float
    y: float
    timestamp_ms: int


class RssiEstimatorService:
    """Thread-safe RSSI window and estimator state."""

    def __init__(
        self,
        anchors: list[AnchorConfig],
        anchor_ble_ids: dict[str, str],
        window_ms: int = DEFAULT_WINDOW_MS,
        calibration_file: Path | None = None,
    ) -> None:
        if len(anchors) < 3:
            raise ValueError("at least three anchors are required")
        self.anchors = anchors
        self.anchor_ble_ids = anchor_ble_ids
        self.ble_to_anchor = {ble_id: anchor_id for anchor_id, ble_id in anchor_ble_ids.items()}
        self.window_ms = window_ms
        self.estimator = EnhancedAICorrectedEstimator(
            anchors,
            dt=0.2,
            field_size=(10.0, 10.0),
            seed=0,
        )
        self.calibrated = False
        self.latest_rssi: dict[str, float] = {}
        self.latest_at: dict[str, float] = {}
        self.latest_estimate: Estimate | None = None
        self._lock = threading.Lock()

        if calibration_file is not None:
            self._calibrate_from_file(calibration_file)
        else:
            self._disable_untrained_mlp()

    def _disable_untrained_mlp(self) -> None:
        """Use the physical RSSI model until a calibration set is supplied."""
        for weights, biases in zip(self.estimator.mlp.weights, self.estimator.mlp.biases):
            weights.fill(0.0)
            biases.fill(0.0)

    def _calibrate_from_file(self, path: Path) -> None:
        payload = json.loads(path.read_text(encoding="utf-8"))
        samples = payload.get("samples", payload) if isinstance(payload, dict) else payload
        if not isinstance(samples, list):
            raise ValueError("calibration file must contain a samples list")

        sequences: list[list[dict[str, float]]] = []
        positions: list[tuple[float, float]] = []
        for sample in samples:
            position = sample["position"]
            observations = sample["observations"]
            if not isinstance(position, list) or len(position) != 2:
                raise ValueError("calibration position must be [x, y]")
            if isinstance(observations, dict):
                observations = [observations]
            sequence = [
                {anchor_id: _rssi(values[anchor_id]) for anchor_id in self.anchor_ids if anchor_id in values}
                for values in observations
            ]
            sequences.append(sequence)
            positions.append((float(position[0]), float(position[1])))

        if not sequences or any(not sequence for sequence in sequences):
            raise ValueError("calibration file contains no observations")
        loss = self.estimator.calibrate(sequences, positions, epochs=120, seed=0)
        if not loss:
            raise ValueError("calibration did not produce training samples")
        self.calibrated = True

    @property
    def anchor_ids(self) -> list[str]:
        return [anchor.anchor_id for anchor in self.anchors]

    def _record(self, anchor_id: str, value: float, received_at: float) -> Estimate | None:
        if anchor_id not in self.anchor_ids:
            raise ValueError(f"unknown anchor: {anchor_id}")
        self.latest_rssi[anchor_id] = _rssi(value)
        self.latest_at[anchor_id] = received_at

        if not all(anchor_id in self.latest_rssi for anchor_id in self.anchor_ids):
            return None
        age_ms = (max(self.latest_at.values()) - min(self.latest_at.values())) * 1000.0
        if age_ms > self.window_ms:
            return None

        position = self.estimator.update(dict(self.latest_rssi))
        estimate = Estimate(float(position[0]), float(position[1]), int(received_at * 1000))
        self.latest_estimate = estimate
        return estimate

    def ingest(self, payload: dict[str, Any]) -> dict[str, Any]:
        received_at = time.monotonic()
        with self._lock:
            observations = payload.get("observations")
            if observations is not None:
                if not isinstance(observations, dict):
                    raise ValueError("observations must be an object")
                estimate = None
                for anchor_id, value in observations.items():
                    estimate = self._record(str(anchor_id), value, received_at) or estimate
            else:
                if "rssi" not in payload:
                    raise ValueError("payload requires rssi or observations")
                anchor_id = payload.get("anchor_id")
                if anchor_id is None and payload.get("data"):
                    ble_id = extract_anchor_id(str(payload["data"]))
                    anchor_id = self.ble_to_anchor.get(ble_id) if ble_id else None
                if anchor_id is None and payload.get("address"):
                    anchor_id = self.ble_to_anchor.get(normalize_ble_id(str(payload["address"])))
                if anchor_id is None:
                    raise ValueError("could not identify anchor from payload")
                estimate = self._record(str(anchor_id), payload["rssi"], received_at)

            result: dict[str, Any] = {
                "accepted": True,
                "observations": dict(self.latest_rssi),
                "calibrated": self.calibrated,
            }
            if estimate is not None:
                result["estimate"] = {"x": estimate.x, "y": estimate.y, "timestamp_ms": estimate.timestamp_ms}
            return result

    def status(self) -> dict[str, Any]:
        with self._lock:
            result: dict[str, Any] = {
                "anchors": self.anchor_ids,
                "observations": dict(self.latest_rssi),
                "calibrated": self.calibrated,
            }
            if self.latest_estimate is not None:
                result["estimate"] = {
                    "x": self.latest_estimate.x,
                    "y": self.latest_estimate.y,
                    "timestamp_ms": self.latest_estimate.timestamp_ms,
                }
            return result


class RequestHandler(BaseHTTPRequestHandler):
    service: RssiEstimatorService

    def _send_json(self, status: HTTPStatus, body: dict[str, Any]) -> None:
        encoded = json.dumps(body, separators=(",", ":")).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(encoded)))
        self.end_headers()
        self.wfile.write(encoded)

    def do_GET(self) -> None:
        if self.path == "/health":
            self._send_json(HTTPStatus.OK, {"ok": True})
        elif self.path in ("/estimate", "/status"):
            self._send_json(HTTPStatus.OK, self.service.status())
        else:
            self._send_json(HTTPStatus.NOT_FOUND, {"error": "not found"})

    def do_POST(self) -> None:
        if self.path != "/ble":
            self._send_json(HTTPStatus.NOT_FOUND, {"error": "not found"})
            return
        try:
            length = int(self.headers.get("Content-Length", "0"))
            payload = json.loads(self.rfile.read(length))
            if not isinstance(payload, dict):
                raise ValueError("request body must be a JSON object")
            self._send_json(HTTPStatus.OK, self.service.ingest(payload))
        except (ValueError, TypeError, json.JSONDecodeError) as error:
            self._send_json(HTTPStatus.BAD_REQUEST, {"error": str(error)})

    def log_message(self, format: str, *args: Any) -> None:
        return


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="BLE RSSI HTTP receiver and position estimator")
    parser.add_argument("--host", default="0.0.0.0")
    parser.add_argument("--port", type=int, default=8080)
    parser.add_argument("--window-ms", type=int, default=DEFAULT_WINDOW_MS)
    parser.add_argument("--calibration", type=Path, help="JSON calibration file")
    parser.add_argument(
        "--anchor",
        action="append",
        type=parse_anchor,
        required=True,
        metavar="ID=BLE_ID",
        help="repeat for each anchor, e.g. anchor_1=12:34:56:78:9a:bc",
    )
    return parser


def main() -> None:
    args = build_parser().parse_args()
    anchor_mapping = dict(args.anchor)
    coordinates = [(0.0, 0.0), (10.0, 0.0), (10.0, 10.0), (0.0, 10.0)]
    if len(anchor_mapping) > len(coordinates):
        raise SystemExit("this initial server supports at most four anchors")
    anchors = [AnchorConfig(anchor_id, *coordinates[index]) for index, anchor_id in enumerate(anchor_mapping)]
    service = RssiEstimatorService(anchors, anchor_mapping, args.window_ms, args.calibration)
    handler = type("ConfiguredRequestHandler", (RequestHandler,), {"service": service})
    server = ThreadingHTTPServer((args.host, args.port), handler)
    print(f"RSSI server listening on http://{args.host}:{args.port}", flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == "__main__":
    main()