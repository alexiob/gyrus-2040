#!/usr/bin/env python3
"""Compare a connected game's deterministic G1 replay with the native runner.

Single serial owner. Leaves both sessions at the title with normal gameplay.
Writes JSON measurements and CRC-checked screenshots; does not flash firmware.
"""
import argparse
import json
import os
from pathlib import Path
import select
import subprocess
import time
from gyrusctl import Client


def run():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--serial', required=True)
    parser.add_argument('--headless', default='build-host/gyrus_headless')
    parser.add_argument('--output', type=Path, default=Path('artifacts/game/device-smoke'))
    args = parser.parse_args()
    import serial
    args.output.mkdir(parents=True, exist_ok=True)
    child = subprocess.Popen([args.headless], stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    dev = serial.Serial(args.serial, 115200, timeout=0, write_timeout=2)
    def client(fd, write):
        def read(count, timeout):
            ready, _, _ = select.select([fd], [], [], timeout)
            return os.read(fd, count) if ready else b''
        return Client(read, write, 10)
    def local_write(data):
        child.stdin.write(data)
        child.stdin.flush()
    native = client(child.stdout.fileno(), local_write)
    device = client(dev.fileno(), dev.write)
    report = {'serial': args.serial, 'cases': []}
    try:
        time.sleep(0.1)
        dev.reset_input_buffer()
        for peer in (device, native):
            peer.request('PAUSE')
            peer.request('RESET 2040')
            peer.request('SET invincible 1')
            peer.request('SET control_mode 0')
        cases = [
            ('title', []),
            ('intro', ['INPUT start=1', 'STEP 1']),
            ('combat', ['INPUT fire=1 turn=1', 'STEP 600']),
            ('chance', ['LEVEL 3', 'INPUT turn=-1', 'STEP 300']),
            ('lasers', ['LEVEL 6', 'STEP 490']),
            ('earth', ['LEVEL 23', 'STEP 300']),
            ('next-route', ['LEVEL 24', 'STEP 600']),
            ('menu', ['INPUT pause=1', 'STEP 1']),
            ('settings-default', ['INPUT menu=1', 'STEP 1']),
            ('settings-adjusted', ['INPUT menu=3', 'STEP 1', 'INPUT menu=5', 'STEP 1', 'INPUT menu=8', 'STEP 1']),
            ('settings-min', ['SET fire_rate 1', 'SET enemy_speed 25', 'SET tilt_sensitivity 50']),
            ('settings-max', ['SET fire_rate 10', 'SET enemy_speed 200', 'SET tilt_sensitivity 200']),
            ('settings-back', ['INPUT menu=2', 'STEP 1']),
            ('settings-resumed', ['INPUT pause=1', 'STEP 120']),
        ]
        for label, commands in cases:
            for command in commands:
                for peer in (device, native):
                    peer.request(command)
            ds = device.request('STATE')[0]['state']
            ns = native.request('STATE')[0]['state']
            assert ds['hash'] == ns['hash'], (label, ds, ns)
            df, dpng = device.request('FRAME')
            nf, npng = native.request('FRAME')
            assert dpng == npng, f'{label}: device/native pixels differ'
            (args.output / f'{label}.png').write_bytes(dpng)
            report['cases'].append({'name': label, 'state': ds, 'crc32': df['crc32']})
        for peer in (device, native):
            peer.request('RESET 2040')
            peer.request('RESUME')
        report['passed'] = True
    finally:
        (args.output / 'results.json').write_text(json.dumps(report, indent=2)+'\n')
        dev.close()
        child.stdin.close()
        child.wait(timeout=3)
    print(json.dumps({'passed': True, 'cases': len(report['cases']), 'output': str(args.output)}))


if __name__ == '__main__':
    run()
