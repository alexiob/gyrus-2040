#!/usr/bin/env python3
"""Record genuine native G1 renderer frames as a silent, real-speed MP4.

Requires ffmpeg and the built headless runner; no Python packages or device.
Simulation advances at 60 Hz, capture at 30 fps. Every frame is CRC checked by
Client before encoding. Only normal inputs are used; no invincibility, injected
scores, stage completion or level skipping. The physical board is untouched.
"""
from __future__ import annotations
import argparse
import json
from pathlib import Path
import shutil
import subprocess
from smoke_arcade import pipe_client

FPS = 30


def tempest_turn(frame):
    phase = frame % 150
    return 1 if phase < 15 or phase >= 135 else -1 if 65 <= phase < 85 else 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--headless', default='build-host/gyrus_headless')
    parser.add_argument('--output', type=Path, default=Path('docs/media/round-arcade-demo.mp4'))
    parser.add_argument('--evidence', type=Path, default=Path('artifacts/demo'))
    parser.add_argument('--seed', type=int, default=2040)
    args = parser.parse_args()
    if not 0 <= args.seed <= 4294967295:
        parser.error('seed must be uint32')
    if not shutil.which('ffmpeg'):
        parser.error('ffmpeg is required')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.evidence.mkdir(parents=True, exist_ok=True)
    game = subprocess.Popen([args.headless, '--seed', str(args.seed)], stdin=subprocess.PIPE,
                            stdout=subprocess.PIPE)
    client = pipe_client(game)
    temporary = args.output.with_name(args.output.stem + '.recording.mp4')
    encoder = subprocess.Popen([
        'ffmpeg', '-hide_banner', '-loglevel', 'error', '-y', '-f', 'image2pipe',
        '-framerate', str(FPS), '-vcodec', 'png', '-i', 'pipe:0',
        '-an', '-sn', '-dn', '-vf', 'scale=720:720:flags=neighbor,setsar=1',
        '-c:v', 'libx264', '-preset', 'slow', '-crf', '21', '-maxrate', '1800k',
        '-bufsize', '3600k', '-profile:v', 'high', '-level:v', '3.1',
        '-pix_fmt', 'yuv420p', '-g', '60', '-map_metadata', '-1', '-movflags', '+faststart',
        str(temporary)], stdin=subprocess.PIPE)
    manifest = {'source': 'native headless renderer; same C engine/assets as RP2040',
                'simulation_hz': 60, 'capture_fps': FPS, 'seed': args.seed,
                'invincibility': False, 'scenes': []}
    total = 0
    def commands(*values):
        for value in values:
            client.request(value)
    def scene(name, frames, *, pilot=None, snapshot=None, snapshots=None, on_frame=None):
        nonlocal total
        start = total
        before = client.request('STATE')[0]['state']
        last_turn = None
        for i in range(frames):
            if pilot:
                turn = pilot(i)
                if turn != last_turn:
                    commands(f'INPUT turn={turn}')
                    last_turn = turn
            if on_frame:
                on_frame(i)
            commands('STEP 2')
            header, png = client.request('FRAME')
            encoder.stdin.write(png)
            if snapshot is not None and i == snapshot:
                (args.evidence / f'{name}.png').write_bytes(png)
            if snapshots and i in snapshots:
                (args.evidence / f'{snapshots[i]}.png').write_bytes(png)
            total += 1
        after = client.request('STATE')[0]['state']
        manifest['scenes'].append({'name': name, 'start': start / FPS,
                                   'duration': frames / FPS, 'before': before, 'after': after})
        print(f'{name}: {frames / FPS:.1f}s, {after["phase"]}', flush=True)
    success = False
    try:
        commands('PAUSE')
        scene('menu-gyrus', 60, snapshot=30)
        commands('INPUT navigate=1')
        scene('scroll-tempest', 36)
        scene('menu-tempest-classic', 45, snapshot=20)
        commands('INPUT menu=9')
        scene('menu-tempest-neon', 36, snapshot=20)
        commands('INPUT menu=1')
        scene('settings', 60, snapshot=30)
        commands('INPUT menu=2', 'STEP 1', 'INPUT navigate=-1')
        scene('scroll-gyrus', 45)
        commands('INPUT start=1')
        scene('gyrus-intro', 45)
        scene('gyrus-gameplay', 600, pilot=lambda i: 1 if i < 210 or i >= 390 else -1,
              snapshot=420)
        commands('INPUT turn=0 pause=1')
        scene('gyrus-pause', 45, snapshot=20)
        commands('HOME')
        scene('return-to-menu', 30)
        commands('INPUT navigate=1')
        scene('select-tempest', 36)
        commands('INPUT start=1')
        scene('tempest-intro', 30, snapshot=15)
        def special(i):
            if i in (300, 540):
                commands('INPUT special=1')
        scene('tempest-gameplay', 720, pilot=tempest_turn,
              snapshots={192: 'tempest-combat', 330: 'tempest-gameplay'}, on_frame=special)
        commands('HOME')
        scene('final-menu', 60)
        encoder.stdin.close()
        if encoder.wait() != 0:
            raise RuntimeError('ffmpeg encoding failed')
        temporary.replace(args.output)
        manifest.update({'frames': total, 'duration': total / FPS,
                         'output': str(args.output), 'bytes': args.output.stat().st_size})
        (args.evidence / 'capture.json').write_text(json.dumps(manifest, indent=2) + '\n')
        success = True
        print(json.dumps({key: manifest[key] for key in ('frames', 'duration', 'output', 'bytes')}))
    finally:
        if encoder.poll() is None:
            encoder.stdin.close()
            encoder.terminate()
            encoder.wait(timeout=5)
        game.stdin.close()
        game.wait(timeout=5)
        if not success:
            temporary.unlink(missing_ok=True)


if __name__ == '__main__':
    main()
