#!/usr/bin/env python3
"""Install pinned diagnostic dependencies locally, without system installers."""
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parents[1]
DEPS = ROOT / '.deps'
LOCK = json.loads((ROOT / 'dependencies.lock.json').read_text())

def run(*args):
    subprocess.run([str(a) for a in args], cwd=ROOT, check=True)

def download(spec):
    path = DEPS / 'downloads' / spec['filename']
    path.parent.mkdir(parents=True, exist_ok=True)
    if not path.exists():
        temp = path.with_suffix('.partial')
        req = urllib.request.Request(spec['url'], headers={'User-Agent': 'gyrus-2040-setup'})
        with urllib.request.urlopen(req, timeout=60) as response, temp.open('wb') as out:
            while chunk := response.read(1024 * 1024):
                out.write(chunk)
        temp.replace(path)
    with path.open('rb') as source:
        actual = hashlib.file_digest(source, 'sha256').hexdigest()
    if actual != spec['sha256']:
        raise RuntimeError(f'Checksum mismatch: {path}; remove it and retry')
    return path

def main():
    if platform.system() != 'Darwin' or platform.machine() != 'arm64':
        raise SystemExit('This setup pins macOS arm64 tools. Other hosts may provide their own Pico SDK/toolchain.')
    DEPS.mkdir(exist_ok=True)
    sdk = DEPS / 'pico-sdk'
    if not sdk.exists():
        run('git', 'clone', '--depth', '1', '--branch', LOCK['pico_sdk']['tag'], LOCK['pico_sdk']['url'], sdk)
    actual = subprocess.check_output(['git', '-C', str(sdk), 'rev-parse', 'HEAD'], text=True).strip()
    if actual != LOCK['pico_sdk']['commit']:
        raise RuntimeError('Existing Pico SDK is not at the locked revision')
    run('git', '-C', sdk, 'submodule', 'update', '--init', '--depth', '1', 'lib/tinyusb')
    actual = subprocess.check_output(['git', '-C', str(sdk/'lib/tinyusb'), 'rev-parse', 'HEAD'], text=True).strip()
    if actual != LOCK['pico_sdk']['tinyusb_commit']:
        raise RuntimeError('TinyUSB revision mismatch')
    package = download(LOCK['arm_toolchain'])
    expanded = DEPS / 'arm-expanded'
    if not expanded.exists():
        run('pkgutil', '--expand-full', package, expanded)
    compilers = list(expanded.rglob('arm-none-eabi-gcc'))
    if len(compilers) != 1:
        raise RuntimeError('Expected exactly one compiler in expanded package')
    toolchain = DEPS / 'toolchain'
    if not toolchain.exists():
        toolchain.symlink_to(compilers[0].parent.parent, target_is_directory=True)
    run(toolchain/'bin/arm-none-eabi-gcc', '--version')
    archive = download(LOCK['picotool'])
    picotool = DEPS / 'picotool/picotool/picotool'
    if not picotool.exists():
        with zipfile.ZipFile(archive) as z:
            z.extractall(DEPS / 'picotool')
    picotool.chmod(0o755)
    run(picotool, 'version')
    venv = DEPS / 'venv'
    if not venv.exists():
        run(os.sys.executable, '-m', 'venv', venv)
    run(venv/'bin/python', '-m', 'pip', 'install', 'pyserial==' + LOCK['pyserial'])
    print('Ready: cmake --preset host; cmake --preset firmware')

if __name__ == '__main__':
    main()
