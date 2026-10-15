#!/usr/bin/env python3
"""G1 debug client: headless subprocess, loopback TCP, or USB CDC serial."""
from __future__ import annotations
import argparse
import json
import os
from pathlib import Path
import select
import socket
import struct
import subprocess
import sys
import time
import zlib


class ProtocolError(RuntimeError):
    pass


def png_from_frame(header: dict, payload: bytes) -> bytes:
    width, height = header.get('width'), header.get('height')
    palette_bytes, pixel_bytes = header.get('palette_bytes'), header.get('pixel_bytes')
    if (type(width) is not int or type(height) is not int or
            not 1 <= width <= 240 or not 1 <= height <= 240 or
            type(palette_bytes) is not int or not 2 <= palette_bytes <= 512 or palette_bytes % 2 or
            type(pixel_bytes) is not int or pixel_bytes != width * height or
            len(payload) != palette_bytes + pixel_bytes):
        raise ProtocolError('Invalid frame dimensions or payload length')
    if zlib.crc32(payload) != header.get('crc32'):
        raise ProtocolError('Frame CRC32 mismatch')
    palette = []
    for (word,) in struct.iter_unpack('<H', payload[:palette_bytes]):
        r, g, b = (word >> 11) & 31, (word >> 5) & 63, word & 31
        palette.append(bytes(((r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2))))
    pixels = payload[palette_bytes:]
    if any(p >= len(palette) for p in pixels):
        raise ProtocolError('Pixel index exceeds palette')
    rows = b''.join(b'\x00' + b''.join(palette[p] for p in pixels[y*width:(y+1)*width]) for y in range(height))
    def chunk(kind: bytes, data: bytes) -> bytes:
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data))
    return (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 2, 0, 0, 0)) +
            chunk(b'IDAT', zlib.compress(rows, 9)) + chunk(b'IEND', b''))


class Client:
    def __init__(self, read, write, timeout: float = 5):
        self.read = read
        self.write = write
        self.timeout = timeout
        self.buffer = bytearray()
        self.request_id = 0

    def _exact(self, count: int, deadline: float) -> bytes:
        while len(self.buffer) < count:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise ProtocolError('Response timed out')
            data = self.read(max(1, count-len(self.buffer)), remaining)
            if not data:
                raise ProtocolError('Connection closed or timed out')
            self.buffer.extend(data)
        result = bytes(self.buffer[:count])
        del self.buffer[:count]
        return result

    def _line(self, deadline: float) -> bytes:
        result = bytearray()
        while len(result) <= 2048:
            byte = self._exact(1, deadline)
            if byte == b'\n':
                return bytes(result)
            result.extend(byte)
        raise ProtocolError('Response header too long')

    def request(self, command: str) -> tuple[dict, bytes | None]:
        self.request_id += 1
        if '\n' in command or '\r' in command:
            raise ProtocolError('One command per request')
        try:
            line = f'G1 {self.request_id} {command}\n'.encode('ascii')
        except UnicodeEncodeError as exc:
            raise ProtocolError('Commands must be ASCII') from exc
        if len(line) > 256:
            raise ProtocolError('Request exceeds 255 bytes before newline')
        self.write(line)
        deadline = time.monotonic() + self.timeout
        try:
            header = json.loads(self._line(deadline))
        except (ValueError, UnicodeDecodeError) as exc:
            raise ProtocolError('Invalid JSON response') from exc
        if not isinstance(header, dict) or header.get('v') != 1 or header.get('id') != self.request_id:
            raise ProtocolError('Response version or request ID mismatch')
        if header.get('ok') is not True:
            raise ProtocolError(str(header.get('error', 'request rejected')))
        if header.get('format') != 'indexed8':
            return header, None
        palette_bytes, pixel_bytes = header.get('palette_bytes'), header.get('pixel_bytes')
        if (type(palette_bytes) is not int or type(pixel_bytes) is not int or
                not 2 <= palette_bytes <= 512 or not 1 <= pixel_bytes <= 57600):
            raise ProtocolError('Invalid binary payload length')
        payload = self._exact(palette_bytes + pixel_bytes, deadline)
        if self._exact(1, deadline) != b'\n':
            raise ProtocolError('Missing frame trailer')
        return header, png_from_frame(header, payload)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    endpoint = parser.add_mutually_exclusive_group(required=True)
    endpoint.add_argument('--headless', metavar='BINARY', help='launch a local headless executable')
    endpoint.add_argument('--serial', metavar='DEVICE', help='USB CDC device; requires pyserial')
    endpoint.add_argument('--tcp', metavar='PORT', type=int, help='connect to 127.0.0.1:PORT')
    parser.add_argument('--timeout', type=float, default=10)
    parser.add_argument('--baud', type=int, default=115200)
    parser.add_argument('--output', '-o', type=Path, help='PNG destination for FRAME (default capture.png)')
    parser.add_argument('--batch', type=Path, help='file of commands, one per line; # comments allowed')
    parser.add_argument('command', nargs='*', help='HELLO, STATE, INPUT key=value..., STEP n, SET key n, LEVEL n, RESET seed, PAUSE, RESUME, FRAME')
    args = parser.parse_args()
    if args.timeout <= 0:
        parser.error('--timeout must be positive')
    if args.tcp is not None and not 1 <= args.tcp <= 65535:
        parser.error('--tcp must be a port in 1..65535')
    process = connection = None
    try:
        if args.headless:
            process = subprocess.Popen([args.headless], stdin=subprocess.PIPE, stdout=subprocess.PIPE)
            assert process.stdin is not None and process.stdout is not None
            fd = process.stdout.fileno()
            def write(data):
                process.stdin.write(data)
                process.stdin.flush()
        elif args.tcp is not None:
            connection = socket.create_connection(('127.0.0.1', args.tcp), args.timeout)
            fd = connection.fileno()
            write = connection.sendall
        else:
            import serial
            connection = serial.Serial(args.serial, args.baud, timeout=args.timeout, write_timeout=args.timeout)
            # A previous client may have abandoned a binary frame mid-stream.
            time.sleep(0.05)
            connection.reset_input_buffer()
            fd = connection.fileno()
            write = connection.write
        def read(count, timeout):
            ready, _, _ = select.select([fd], [], [], timeout)
            return os.read(fd, count) if ready else b''
        client = Client(read, write, args.timeout)
        commands = []
        if args.batch:
            commands.extend(line.strip() for line in args.batch.read_text().splitlines() if line.strip() and not line.lstrip().startswith('#'))
        if args.command:
            commands.append(' '.join(args.command))
        if not commands:
            commands = ['STATE']
        for command in commands:
            head, separator, tail = command.partition(' ')
            header, png = client.request(head.upper() + (separator + tail if separator else ''))
            if png is not None:
                destination = args.output or Path('capture.png')
                destination.write_bytes(png)
                header['saved'] = str(destination)
            print(json.dumps(header, sort_keys=True))
        return 0
    except (OSError, ProtocolError, ImportError) as exc:
        print(f'gyrusctl: {exc}', file=sys.stderr)
        return 1
    finally:
        if connection is not None:
            connection.close()
        if process is not None:
            if process.stdin is not None:
                process.stdin.close()
            try:
                process.wait(timeout=2)
            except subprocess.TimeoutExpired:
                process.terminate()
                process.wait(timeout=2)


if __name__ == '__main__':
    raise SystemExit(main())
