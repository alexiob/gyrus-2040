#!/usr/bin/env python3
"""Measure frozen-scene rendering/presentation on the USB board at stock clocks.

Debug setup/capture time is excluded from the measured dwell. The board returns
to the launcher afterward. One owner must hold the serial transport.
"""
import argparse
import json
from pathlib import Path
import time
from smoke_arcade import fd_client


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--serial', required=True)
    parser.add_argument('--output', type=Path, default=Path('artifacts/multigame/performance.json'))
    args = parser.parse_args()
    import serial
    results = []
    with serial.Serial(args.serial, 115200, timeout=0, write_timeout=2) as port:
        time.sleep(.1)
        port.reset_input_buffer()
        client = fd_client(port.fileno(), port.write)
        client.request('PAUSE')
        def measure(name, commands):
            for cmd in commands:
                client.request(cmd)
            time.sleep(.1)
            before = client.request('STATE')[0]['state']
            began = time.monotonic()
            time.sleep(.5)
            after = client.request('STATE')[0]['state']
            duration = time.monotonic() - began
            a, b = before['hardware'], after['hardware']
            results.append({'name': name, 'phase': after['phase'],
                            'render_us': b['render_us'], 'present_us': b['last_present_us'],
                            'present_hz': round((b['presented_frames']-a['presented_frames'])/duration, 2),
                            'dropped': b['dropped_frames']-a['dropped_frames'],
                            'overruns': b['sim_overruns']-a['sim_overruns'],
                            'state': after})
        measure('hub-gyrus', ['GAME gyrus', 'HOME', 'SET control_mode 0'])
        measure('hub-settings', ['INPUT menu=1', 'STEP 1'])
        measure('gyrus-combat', ['GAME gyrus', 'RESET 2040', 'SET invincible 1', 'INPUT start=1 turn=1', 'STEP 600'])
        for neon in (0, 1):
            client.request('GAME tempest')
            client.request('RESET 2040')
            client.request('HOME')
            client.request(f'SET effects {neon}')
            measure(f'hub-tempest-{neon}', [])
            client.request('INPUT start=1')
            client.request('STEP 1')
            client.request('SET invincible 1')
            for shape in range(1, 17):
                measure(f'tempest-{neon}-shape-{shape}', [f'LEVEL {shape+32}', 'INPUT turn=1', 'STEP 500'])
            measure(f'tempest-{neon}-clear', ['SET autofire 0', 'LEVEL 1', 'INPUT turn=0', 'STEP 1020'])
            measure(f'tempest-{neon}-dive', ['STEP 65'])
            measure(f'tempest-{neon}-flight', ['STEP 45'])
            measure(f'tempest-{neon}-entry', ['STEP 60'])
        client.request('HOME')
        client.request('RESUME')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(results, indent=2) + '\n')
    worst = max(results, key=lambda r: r['render_us'])
    print(json.dumps({'scenes': len(results), 'worst': {k: v for k, v in worst.items() if k != 'state'}, 'output': str(args.output)}))


if __name__ == '__main__':
    main()
