"""
BLE Locator — AI Inference Host

Position estimation server using μAI-Bridge protocol.
Receives RSSI + beacon positions from firmware, runs ML model,
returns estimated (x, y, confidence).

Usage:
    pip install -r requirements.txt
    python uai_host.py --port 5000
"""
import argparse
import struct
import socket
import threading
import numpy as np
from model import PositionEstimator


# μAI-Bridge protocol constants
MAGIC = 0xAB55
MSG_INFER_REQ = 0x01
MSG_INFER_RESP = 0x02
HEADER_SIZE = 8  # magic(2) + type(1) + seq(1) + length(4)


def parse_header(data: bytes):
    if len(data) < HEADER_SIZE:
        return None
    magic, msg_type, seq_id, length = struct.unpack("<HBBI", data[:HEADER_SIZE])
    if magic != MAGIC:
        return None
    return msg_type, seq_id, length


def parse_infer_request(payload: bytes):
    """Parse inference request payload.

    Format: model_name(32 bytes, null-padded) + input_data
    """
    if len(payload) < 32:
        return None, None
    model_name = payload[:32].rstrip(b"\x00").decode("ascii", errors="ignore")
    input_data = payload[32:]
    return model_name, input_data


def parse_scan_data(data: bytes):
    """Parse BLE scan data from firmware.

    Format: [n_beacons(u8)] [rssi(i8) x(f32) y(f32)] * n_beacons
    """
    if len(data) < 1:
        return None, None, None
    n = data[0]
    rssi_list = []
    bx_list = []
    by_list = []
    offset = 1
    for _ in range(n):
        if offset + 9 > len(data):
            break
        rssi = struct.unpack_from("<b", data, offset)[0]
        x = struct.unpack_from("<f", data, offset + 1)[0]
        y = struct.unpack_from("<f", data, offset + 5)[0]
        rssi_list.append(rssi)
        bx_list.append(x)
        by_list.append(y)
        offset += 9
    return np.array(rssi_list), np.array(bx_list), np.array(by_list)


def build_response(seq_id: int, x: float, y: float, confidence: float) -> bytes:
    """Build inference response: header + [x(f32) y(f32) confidence(f32)]."""
    payload = struct.pack("<fff", x, y, confidence)
    header = struct.pack("<HBBI", MAGIC, MSG_INFER_RESP, seq_id, len(payload))
    return header + payload


class InferenceServer:
    def __init__(self, port: int):
        self.port = port
        self.model = PositionEstimator()

    def start(self):
        srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        srv.bind(("0.0.0.0", self.port))
        srv.listen(4)
        print(f"[Host] Listening on port {self.port}")
        while True:
            conn, addr = srv.accept()
            print(f"[Host] Connection from {addr}")
            t = threading.Thread(target=self._handle, args=(conn,), daemon=True)
            t.start()

    def _handle(self, conn: socket.socket):
        buf = b""
        try:
            while True:
                chunk = conn.recv(4096)
                if not chunk:
                    break
                buf += chunk
                while len(buf) >= HEADER_SIZE:
                    parsed = parse_header(buf)
                    if parsed is None:
                        buf = buf[1:]  # resync
                        continue
                    msg_type, seq_id, length = parsed
                    total = HEADER_SIZE + length
                    if len(buf) < total:
                        break  # wait for more data
                    payload = buf[HEADER_SIZE:total]
                    buf = buf[total:]
                    if msg_type == MSG_INFER_REQ:
                        self._on_infer(conn, seq_id, payload)
        except (ConnectionResetError, BrokenPipeError):
            pass
        finally:
            conn.close()
            print("[Host] Client disconnected")

    def _on_infer(self, conn: socket.socket, seq_id: int, payload: bytes):
        model_name, input_data = parse_infer_request(payload)
        if model_name != "ble_locate":
            print(f"[Host] Unknown model: {model_name}")
            resp = build_response(seq_id, 0.0, 0.0, 0.0)
            conn.sendall(resp)
            return

        rssi, bx, by = parse_scan_data(input_data)
        if rssi is None:
            resp = build_response(seq_id, 0.0, 0.0, 0.0)
            conn.sendall(resp)
            return

        x, y, conf = self.model.estimate(rssi, bx, by)
        print(f"[Host] RSSI={rssi.tolist()} -> ({x:.2f}, {y:.2f}) conf={conf:.3f}")
        resp = build_response(seq_id, x, y, conf)
        conn.sendall(resp)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="BLE Locator AI Host")
    parser.add_argument("--port", type=int, default=5000)
    args = parser.parse_args()
    InferenceServer(args.port).start()
