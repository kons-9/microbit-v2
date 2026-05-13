"""decode.py のユニットテスト"""

import struct
import sys
from pathlib import Path
from io import StringIO

# テスト対象のモジュールをインポートできるようにパスを追加
sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "tools"))

import decode


def make_record(log_type: int, payload: bytes, timestamp_ms: int = 0) -> bytes:
    """テスト用バイナリレコードを構築"""
    hdr = struct.pack(
        decode.HEADER_FMT,
        1,              # version
        log_type,       # type
        len(payload),   # size
        0,              # reserved
        timestamp_ms,   # timestamp_ms
        0,              # padding
    )
    record = hdr + payload
    # 4バイトアライン
    pad_len = (4 - (len(record) % 4)) % 4
    return record + b'\xff' * pad_len


class TestParseHeader:
    def test_basic(self):
        raw = struct.pack(decode.HEADER_FMT, 1, 2, 8, 0, 1234, 0)
        hdr = decode.parse_header(raw)
        assert hdr.version == 1
        assert hdr.type == 2
        assert hdr.size == 8
        assert hdr.timestamp_ms == 1234


class TestFormatTimestamp:
    def test_zero(self):
        assert decode.format_timestamp(0) == "00:00.000"

    def test_one_second(self):
        assert decode.format_timestamp(1000) == "00:01.000"

    def test_complex(self):
        # 1分23秒456ms = 83456ms, but u16 wraps at 65535
        assert decode.format_timestamp(65535) == "01:05.535"


class TestDecodeCrash:
    def test_basic(self):
        payload = struct.pack("<IIII", 3, 0x08001234, 0x08005678, 0x20004000)
        result = decode.decode_crash(payload)
        assert "fault_type: 3" in result
        assert "0x08001234" in result
        assert "0x08005678" in result

    def test_with_stack(self):
        payload = struct.pack("<IIII", 1, 0, 0, 0)
        payload += struct.pack("<II", 0xDEADBEEF, 0xCAFEBABE)
        result = decode.decode_crash(payload)
        assert "DEADBEEF" in result
        assert "CAFEBABE" in result

    def test_incomplete(self):
        result = decode.decode_crash(b'\x00' * 4)
        assert "incomplete" in result


class TestDecodeEvent:
    def test_basic(self):
        # EventEntry: event_id(1) + reserved(3) + param(4) = 8 bytes
        payload = struct.pack("<BBBBI", 42, 0, 0, 0, 0xDEADBEEF)
        result = decode.decode_event(payload)
        assert "event_id: 42" in result
        assert "DEADBEEF" in result

    def test_incomplete(self):
        result = decode.decode_event(b'\x00' * 4)
        assert "incomplete" in result


class TestDecodeBle:
    def test_basic(self):
        addr = bytes([0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF])
        rssi = struct.pack("<b", -72)
        event_type = bytes([1])
        payload = addr + rssi + event_type
        result = decode.decode_ble(payload)
        assert "AA:BB:CC:DD:EE:FF" in result
        assert "-72" in result

    def test_incomplete(self):
        result = decode.decode_ble(b'\x00' * 4)
        assert "incomplete" in result


class TestDecodeOta:
    def test_basic(self):
        # state(1) + reserved(1) + progress(2) + bytes(4) = 8
        payload = struct.pack("<BBHI", 2, 0, 500, 32768)
        result = decode.decode_ota(payload)
        assert "state: 2" in result
        assert "50.0%" in result
        assert "32768" in result


class TestDecodeStream:
    def test_single_event(self, capsys):
        # EventEntry: event_id(1) + reserved(3) + param(4) = 8 bytes
        payload = struct.pack("<BBBBI", 42, 0, 0, 0, 0x12345678)
        data = make_record(decode.LogType.EVENT, payload, timestamp_ms=1500)
        decode.decode_stream(data)
        captured = capsys.readouterr()
        assert "EVENT" in captured.out
        assert "event_id: 42" in captured.out
        assert "1 records decoded" in captured.out

    def test_multiple_records(self, capsys):
        ev_payload = struct.pack("<BBBBI", 1, 0, 0, 0, 100)
        ble_payload = bytes([0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF]) + struct.pack("<bB", -55, 0)

        data = make_record(decode.LogType.EVENT, ev_payload, 100)
        data += make_record(decode.LogType.BLE, ble_payload, 200)

        decode.decode_stream(data)
        captured = capsys.readouterr()
        assert "EVENT" in captured.out
        assert "BLE" in captured.out
        assert "2 records decoded" in captured.out

    def test_skips_ff_padding(self, capsys):
        padding = b'\xff' * 16
        payload = struct.pack("<BBBBI", 5, 0, 0, 0, 0)
        data = padding + make_record(decode.LogType.EVENT, payload, 0)

        decode.decode_stream(data)
        captured = capsys.readouterr()
        assert "1 records decoded" in captured.out

    def test_empty_data(self, capsys):
        decode.decode_stream(b'\xff' * 32)
        captured = capsys.readouterr()
        assert "0 records decoded" in captured.out
