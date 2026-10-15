#!/usr/bin/env python3
"""Compare the current Gyrus module with a saved pre-refactor G1 executable.

Title frames intentionally gain a Home button in the new session renderer;
combat, transitions, pause and Settings frames must remain byte-identical.
Use executables with equivalent gameplay rules: the deliberate wave corrections
after commit 05e4787 are expected to differ from the original adaptation.
"""
import argparse
import json
from pathlib import Path
import subprocess
from smoke_arcade import pipe_client


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--legacy', required=True)
    parser.add_argument('--current', default='build-host/gyrus_headless')
    parser.add_argument('--output', type=Path, default=Path('artifacts/multigame/gyrus-compat.json'))
    args = parser.parse_args()
    children = [subprocess.Popen([path], stdin=subprocess.PIPE, stdout=subprocess.PIPE)
                for path in (args.legacy, args.current)]
    old, new = map(pipe_client, children)
    results = []
    try:
        new.request('GAME gyrus')
        for seed in (1, 2040, 4294967295):
            for peer in (old, new):
                peer.request(f'RESET {seed}')
                peer.request('SET invincible 1')
            for label, commands, frame in (
                ('title', [], False),
                ('intro', ['INPUT start=1', 'STEP 1'], True),
                ('combat', ['INPUT turn=1 fire=1', 'STEP 600'], True),
                ('pause', ['INPUT pause=1', 'STEP 1'], True),
                ('settings', ['INPUT menu=1', 'STEP 1'], True),
                ('settings-tuned', ['INPUT menu=3', 'STEP 1', 'INPUT menu=5', 'STEP 1', 'INPUT menu=8', 'STEP 1'], True),
                ('resume', ['INPUT menu=2', 'STEP 1', 'INPUT pause=1', 'STEP 120'], True),
            ):
                for command in commands:
                    for peer in (old, new):
                        peer.request(command)
                os = old.request('STATE')[0]['state']
                ns = new.request('STATE')[0]['state']
                assert os == ns, (seed, label, os, ns)
                if frame:
                    assert old.request('FRAME')[1] == new.request('FRAME')[1], (seed, label)
                results.append({'seed': seed, 'name': label, 'hash': ns['hash'], 'frame': frame})
            for stage in (2, 3, 6, 22, 23, 24):
                for peer in (old, new):
                    peer.request(f'LEVEL {stage}')
                    peer.request('STEP 700')
                os = old.request('STATE')[0]['state']
                ns = new.request('STATE')[0]['state']
                assert os == ns, (seed, stage, os, ns)
                assert old.request('FRAME')[1] == new.request('FRAME')[1], (seed, stage)
                results.append({'seed': seed, 'stage': stage, 'hash': ns['hash'], 'frame': True})
    finally:
        for child in children:
            child.stdin.close()
            child.wait(timeout=5)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps({'passed': True, 'cases': results}, indent=2) + '\n')
    print(json.dumps({'passed': True, 'cases': len(results), 'output': str(args.output)}))


if __name__ == '__main__':
    main()
