#!/usr/bin/env python3
"""
flash_log decoder — Flash ログバイナリデータをテキストに復元する

使い方:
  # UARTから `cat log --hex` の出力をファイルに保存して:
  python3 decode.py flash_dump.bin

  # または stdin から:
  cat flash_dump.bin | python3 decode.py -

レコードフォーマット:
  [RecordHeader: 8 bytes]
    version:      u8
    type:         u8
    size:         u8  (payload bytes)
    reserved:     u8
    timestamp_ms: u16 (little-endian)
    padding:      u16

  [Payload: <size> bytes]
    型ごとに異なるフォーマット
"""

import struct
import sys
from dataclasses import dataclass
from enum import IntEnum
from pathlib import Path


class LogType(IntEnum):
    CRASH = 1
    EVENT = 2
    BLE = 3
    OTA = 4


HEADER_FMT = "<BBBBHH"
HEADER_SIZE = struct.calcsize(HEADER_FMT)  # 8


@dataclass
class RecordHeader:
    version: int
    type: int
    size: int
    reserved: int
    timestamp_ms: int
    padding: int


def parse_header(data: bytes) -> RecordHeader:
    fields = struct.unpack(HEADER_FMT, data[:HEADER_SIZE])
    return RecordHeader(*fields)


def format_timestamp(ts_ms: int) -> str:
    """下位16bitタイムスタンプを MM:SS.mmm 形式に"""
    total_ms = ts_ms
    secs = total_ms // 1000
    ms = total_ms % 1000
    mins = secs // 60
    secs = secs % 60
    return f"{mins:02d}:{secs:02d}.{ms:03d}"


def decode_crash(payload: bytes) -> str:
    if len(payload) < 16:
        return f"  [incomplete crash data: {len(payload)} bytes]"

    fault_type, pc, lr, sp = struct.unpack_from("<IIII", payload, 0)
    lines = [
        f"  fault_type: {fault_type}",
        f"  pc:  0x{pc:08X}",
        f"  lr:  0x{lr:08X}",
        f"  sp:  0x{sp:08X}",
    ]

    # stack dump (残り)
    stack_offset = 16
    stack_words = (len(payload) - stack_offset) // 4
    if stack_words > 0:
        stack_vals = struct.unpack_from(f"<{stack_words}I", payload, stack_offset)
        stack_str = " ".join(f"{v:08X}" for v in stack_vals)
        lines.append(f"  stack: {stack_str}")

    return "\n".join(lines)


def decode_event(payload: bytes) -> str:
    if len(payload) < 8:
        return f"  [incomplete event data: {len(payload)} bytes]"

    event_id = payload[0]
    param = struct.unpack_from("<I", payload, 4)[0]
    return f"  event_id: {event_id}, param: 0x{param:08X}"


def decode_ble(payload: bytes) -> str:
    if len(payload) < 8:
        return f"  [incomplete BLE data: {len(payload)} bytes]"

    addr = payload[0:6]
    rssi = struct.unpack_from("<b", payload, 6)[0]
    event_type = payload[7]
    addr_str = ":".join(f"{b:02X}" for b in addr)
    return f"  addr: {addr_str}, rssi: {rssi} dBm, type: {event_type}"


def decode_ota(payload: bytes) -> str:
    if len(payload) < 8:
        return f"  [incomplete OTA data: {len(payload)} bytes]"

    state = payload[0]
    progress = struct.unpack_from("<H", payload, 2)[0]
    total_bytes = struct.unpack_from("<I", payload, 4)[0]
    return f"  state: {state}, progress: {progress/10:.1f}%, bytes: {total_bytes}"


DECODERS = {
    LogType.CRASH: decode_crash,
    LogType.EVENT: decode_event,
    LogType.BLE: decode_ble,
    LogType.OTA: decode_ota,
}


def decode_stream(data: bytes) -> None:
    offset = 0
    record_num = 0

    while offset + HEADER_SIZE <= len(data):
        # 未書き込み領域 (0xFF) をスキップ
        if data[offset] == 0xFF:
            offset += 1
            continue

        hdr = parse_header(data[offset : offset + HEADER_SIZE])

        # 妥当性チェック
        if hdr.version == 0 or hdr.version > 10:
            # 壊れたデータ — 次のアラインまでスキップ
            offset += 4
            continue

        offset += HEADER_SIZE

        if offset + hdr.size > len(data):
            print(f"[#{record_num}] TRUNCATED (need {hdr.size} bytes, have {len(data) - offset})")
            break

        payload = data[offset : offset + hdr.size]
        offset += hdr.size

        # 4バイトアライン
        offset = (offset + 3) & ~3

        record_num += 1
        ts_str = format_timestamp(hdr.timestamp_ms)
        try:
            type_name = LogType(hdr.type).name
        except ValueError:
            type_name = f"UNKNOWN({hdr.type})"

        print(f"[{ts_str}] {type_name} (v{hdr.version}, {hdr.size}B)")

        decoder = DECODERS.get(LogType(hdr.type) if hdr.type in LogType._value2member_map_ else None)
        if decoder:
            print(decoder(payload))
        else:
            # Raw hex dump
            hex_str = " ".join(f"{b:02X}" for b in payload)
            print(f"  raw: {hex_str}")

    print(f"\n--- {record_num} records decoded ---")


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <binary_file | ->")
        print("  Decodes flash log binary dump to human-readable text.")
        sys.exit(1)

    path = sys.argv[1]
    if path == "-":
        data = sys.stdin.buffer.read()
    else:
        data = Path(path).read_bytes()

    if len(data) == 0:
        print("Empty input.")
        sys.exit(0)

    decode_stream(data)


if __name__ == "__main__":
    main()
