#!/usr/bin/env python3
"""Replay launcher and both games against a second native runner or USB board.

Captures CRC-checked PNGs and state hashes; never flashes. Leaves USB at the
launcher running normally. Run the legacy Gyrus corpus with smoke_game.py too.
"""
from __future__ import annotations
import argparse
import json
import os
from pathlib import Path
import select
import subprocess
import time
from gyrusctl import Client, ProtocolError


def pipe_client(child):
    def write(data):
        child.stdin.write(data)
        child.stdin.flush()
    return fd_client(child.stdout.fileno(), write)


def fd_client(fd, write):
    def read(count, timeout):
        ready, _, _ = select.select([fd], [], [], timeout)
        return os.read(fd, count) if ready else b''
    return Client(read, write, 10)


def scenarios():
    yield 'hub-gyrus', ['GAME gyrus', 'HOME', 'SET control_mode 0']
    yield 'hub-tempest', ['INPUT navigate=1', 'STEP 40']
    yield 'hub-tempest-neon', ['SET effects 1']
    yield 'hub-settings', ['INPUT menu=1', 'STEP 1']
    yield 'hub-settings-tuned', ['INPUT menu=4', 'STEP 1', 'INPUT menu=5', 'STEP 1']
    yield 'hub-settings-back', ['INPUT menu=2', 'STEP 1']
    yield 'tempest-title', ['GAME tempest', 'RESET 2040']
    yield 'tempest-entry-start', ['INPUT start=1', 'STEP 1']
    yield 'tempest-entry-middle', ['STEP 30']
    yield 'tempest-play', ['SET invincible 1', 'STEP 200']
    yield 'tempest-zap-all', ['INPUT special=1', 'STEP 1']
    yield 'tempest-zap-one', ['INPUT special=1', 'STEP 2']
    yield 'tempest-paused', ['INPUT pause=1', 'STEP 1']
    yield 'tempest-settings', ['INPUT menu=1', 'STEP 1']
    yield 'tempest-settings-tuned', ['INPUT menu=4', 'STEP 1', 'INPUT menu=5', 'STEP 1']
    yield 'tempest-resume', ['INPUT menu=2', 'STEP 1', 'INPUT pause=1', 'STEP 1']
    yield 'tempest-natural-clear', ['RESET 2040', 'SET invincible 1', 'SET autofire 0', 'LEVEL 1', 'STEP 1020']
    yield 'tempest-dive-start', ['STEP 30']
    yield 'tempest-dive-middle', ['STEP 35']
    yield 'tempest-exit-flight', ['STEP 45']
    yield 'tempest-next-entry', ['STEP 60']
    yield 'tempest-next-entry-middle', ['STEP 25']
    yield 'tempest-next-play', ['STEP 35', 'SET autofire 1']
    for stage in (2, 3, 4, 11, 16, 17, 20, 33, 49, 65, 81, 99):
        yield f'tempest-stage-{stage}', [f'LEVEL {stage}', 'INPUT turn=1', 'STEP 600']
    yield 'tempest-home', ['HOME', 'SET effects 1', 'STEP 40']
    yield 'tempest-neon-entry', ['SET invincible 1', 'INPUT start=1', 'STEP 25']
    yield 'tempest-neon-play', ['STEP 275']
    yield 'tempest-neon-zap', ['INPUT special=1', 'STEP 1']
    yield 'tempest-neon-clear', ['SET autofire 0', 'LEVEL 1', 'INPUT turn=0', 'STEP 1020']
    yield 'tempest-neon-dive', ['STEP 65']
    yield 'tempest-neon-flight', ['STEP 45']
    yield 'gyrus-return', ['GAME gyrus', 'RESET 2040', 'SET invincible 1', 'INPUT start=1', 'STEP 240']
    yield 'gyrus-settings', ['INPUT pause=1', 'STEP 1', 'INPUT menu=1', 'STEP 1']
    yield 'hub-return', ['HOME', 'STEP 40']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--serial')
    parser.add_argument('--headless', default='build-host/gyrus_headless')
    parser.add_argument('--output', type=Path, default=Path('artifacts/multigame/smoke'))
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    children, serial_port = [], None
    report = {'cases': [], 'passed': False}
    try:
        local = subprocess.Popen([args.headless], stdin=subprocess.PIPE, stdout=subprocess.PIPE)
        children.append(local)
        native = pipe_client(local)
        if args.serial:
            import serial
            serial_port = serial.Serial(args.serial, 115200, timeout=0, write_timeout=2)
            time.sleep(0.1)
            serial_port.reset_input_buffer()
            peer = fd_client(serial_port.fileno(), serial_port.write)
        else:
            other = subprocess.Popen([args.headless], stdin=subprocess.PIPE, stdout=subprocess.PIPE)
            children.append(other)
            peer = pipe_client(other)
        for client in (native, peer):
            client.request('PAUSE')
            client.request('GAME tempest')
            client.request('RESET 2040')
            client.request('GAME gyrus')
            client.request('RESET 2040')
            catalogue = client.request('GAMES')[0]['catalogue']
            assert [g['slug'] for g in catalogue['games']] == ['gyrus', 'tempest']
            before = client.request('STATE')[0]['state']
            try:
                client.request('GAME 99')
                raise AssertionError('Invalid game accepted')
            except ProtocolError as exc:
                assert str(exc) == 'rejected'
            after = client.request('STATE')[0]['state']
            before.pop('hardware', None)
            after.pop('hardware', None)
            assert after == before
        for label, commands in scenarios():
            for command in commands:
                for client in (native, peer):
                    client.request(command)
            ns = native.request('STATE')[0]['state']
            ps = peer.request('STATE')[0]['state']
            phases = {'tempest-entry-start': 'intro', 'tempest-entry-middle': 'intro',
                      'tempest-natural-clear': 'clear', 'tempest-dive-start': 'dive',
                      'tempest-dive-middle': 'dive', 'tempest-exit-flight': 'flight',
                      'tempest-next-entry': 'intro', 'tempest-next-entry-middle': 'intro',
                      'tempest-next-play': 'play', 'tempest-neon-entry': 'intro',
                      'tempest-neon-clear': 'clear', 'tempest-neon-dive': 'dive',
                      'tempest-neon-flight': 'flight'}
            if label in phases:
                assert ns['phase'] == ps['phase'] == phases[label], (label, ns, ps)
            if 'hash' in ns:
                assert ns['hash'] == ps['hash'], (label, ns, ps)
            else:
                for key in ('selected', 'phase', 'fire_rate', 'enemy_speed', 'effects'):
                    assert ns[key] == ps[key], (label, key, ns, ps)
            nh, png = native.request('FRAME')
            ph, peer_png = peer.request('FRAME')
            assert png == peer_png, f'{label}: framebuffer mismatch'
            (args.output / f'{label}.png').write_bytes(png)
            report['cases'].append({'name': label, 'state': ps, 'crc32': ph['crc32']})
        report['passed'] = True
        for client in (native, peer):
            client.request('HOME')
            client.request('SET control_mode 0')
            client.request('RESUME')
    finally:
        (args.output / 'results.json').write_text(json.dumps(report, indent=2) + '\n')
        if serial_port:
            serial_port.close()
        for child in children:
            child.stdin.close()
            child.wait(timeout=5)
    print(json.dumps({'passed': True, 'cases': len(report['cases']), 'output': str(args.output)}))


if __name__ == '__main__':
    main()
