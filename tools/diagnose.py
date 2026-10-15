#!/usr/bin/env python3
"""Capture diagnostic JSON lines with finite serial deadlines; never flashes."""
import argparse
import json
from pathlib import Path
import sys
import time
import serial
from serial.tools import list_ports

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('commands', nargs='*', default=['info'])
    parser.add_argument('--port')
    parser.add_argument('--listen', type=float, default=0)
    parser.add_argument('--timeout', type=float, default=25)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    ports = [p.device for p in list_ports.comports() if p.vid == 0x2e8a]
    if args.port:
        port = args.port
    elif len(ports) == 1:
        port = ports[0]
    else:
        raise SystemExit(f'Expected one Raspberry Pi serial device, found {ports}; use --port')
    args.output.parent.mkdir(parents=True, exist_ok=True) if args.output else None
    log = args.output.open('a') if args.output else None
    def emit(line):
        print(line, flush=True)
        if log:
            log.write(line + '\n')
            log.flush()
    completions = {'info':'info','touch':'touch','windows':'windows','bench':'bench_done','boot':'boot',
                   'stream on':'stream','stream off':'stream'}
    try:
        with serial.Serial(port, 115200, timeout=0.2, write_timeout=2) as dev:
            dev.dtr = True
            time.sleep(0.4)
            dev.reset_input_buffer()
            for command in args.commands:
                dev.write((command + '\n').encode('ascii'))
                expected = completions.get(command, 'pattern' if command.startswith(('pattern ', 'speed ')) else 'error')
                deadline = time.monotonic() + args.timeout
                while time.monotonic() < deadline:
                    raw = dev.readline()
                    if not raw:
                        continue
                    line = raw.decode('utf-8', errors='replace').strip()
                    if not line:
                        continue
                    emit(line)
                    try:
                        event = json.loads(line)
                    except json.JSONDecodeError:
                        continue
                    if event.get('event') == 'error' and expected != 'error':
                        raise RuntimeError(f'Board rejected {command}: {event}')
                    if event.get('event') == expected:
                        break
                else:
                    raise TimeoutError(f'No {expected} response to {command!r} on {port}')
            deadline = time.monotonic() + args.listen
            while time.monotonic() < deadline:
                raw = dev.readline()
                if raw:
                    emit(raw.decode('utf-8', errors='replace').strip())
    finally:
        if log:
            log.close()

if __name__ == '__main__':
    try:
        main()
    except (OSError, serial.SerialException, RuntimeError, TimeoutError) as error:
        print(f'ERROR: {error}', file=sys.stderr)
        raise SystemExit(1)
