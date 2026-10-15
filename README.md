# Gyrus 2040

Native C project for a Gyruss-inspired game on the Waveshare
**RP2040-Touch-LCD-1.28**. The architecture is approved; its board diagnostic is
implemented and tested on the connected device. Gameplay and the Sokol
simulator are the next milestones.

## Tested board diagnostic

The firmware drives the GC9A01 display with native RGB565/16-bit SPI DMA,
reads capacitive touch while DMA runs, shows color bars/checkerboard/grid and a
touch cursor, and exposes a bounded USB CDC diagnostic protocol.

- CPU/peripheral/USB clocks: **125/125/48 MHz**, no CPU overclock.
- Fast SPI: **62.5 MHz**, measured **16.148 ms** mean per full-screen DMA
  transfer (61.93 transfers/s). This is transfer capacity, not game frame rate.
- User confirmed clean patterns and correct touch tracking/release at the fast rate.
- Separate 576-transfer stress run: no driver errors.
- CST816S ID 0xB5 and QMI8658 ID 0x05 detected; IMU motion not yet qualified.
- Actual board flash: **16 MiB**; conservative firmware link budget: 4 MiB.
- Host sanitizer tests cover geometry, touch, malformed commands, stuck-transfer
  recovery, and bounded serial input.

See [hardware results and full reproduction steps](docs/hardware/2026-10-15-bringup.md)
and [machine-readable measurements](docs/hardware/2026-10-15-results.json).

## Build

On macOS arm64 with Python 3, CMake, Ninja and Git available:

```sh
python3 tools/setup.py
cmake --preset host
cmake --build --preset host
ctest --preset host
cmake --preset firmware
cmake --build --preset firmware
```

Setup downloads checksum-pinned local tools into `.deps/`; it does not modify
the system compiler or run a privileged installer. Output:
`build-rp2040/gyrus_board_diag.uf2`, ELF, linker map and disassembly.
The SDK uses C++ runtime support files internally; all project firmware and
portable diagnostic code is C11.

## Flash and interact

Hold BOOT while reconnecting USB, wait for the `RPI-RP2` drive, then:

```sh
.deps/picotool/picotool/picotool load -v -x build-rp2040/gyrus_board_diag.uf2 --vid 0x2e8a --pid 0x0003
```

This replaces the firmware beginning at flash offset zero. Back up existing
firmware first; the original device backup from this session is kept in the
ignored `artifacts/hardware/` directory. Select a unique board with `--ser` if
multiple devices are connected.

After USB re-enumerates:

```sh
.deps/venv/bin/python tools/diagnose.py info touch windows
.deps/venv/bin/python tools/diagnose.py bench
.deps/venv/bin/python tools/diagnose.py 'speed fast' 'pattern checker'
.deps/venv/bin/python tools/diagnose.py 'stream on' --listen 30
.deps/venv/bin/python tools/diagnose.py 'stream off'
```

Touch tracking is always enabled and shows a red marker on a grid. `boot`
returns to BOOTSEL; wait for re-enumeration before picotool. Serial access must
have one owner. Use `--port` when more than one Raspberry Pi serial port exists.

## Architecture and research

- [Approved architecture](docs/superpowers/specs/2026-10-15-gyruss-rp2040-design.md)
- [Completed diagnostic plan](docs/superpowers/plans/2026-10-15-board-diagnostics.md)
- [Display performance design](docs/research/display-performance.md)
- [Game reference and level structure](docs/research/game-reference.md)
- [Original toolchain/library research](docs/research/toolchain.md)
- [Reference pack index](reference/README.md) and [download provenance](reference/manifest.json)
- [Third-party notices](THIRD_PARTY.md)

Important integration details: use **GPIO13** for LCD reset, explicitly select
SPI1, and explicitly set `clk_peri` after changing `clk_sys`. Log achieved SPI
rates; requesting 40 MHz at 125 MHz yields 31.25 MHz. No TE synchronization wire
is exposed, so MCU buffering cannot guarantee tear-free panel scanout.

The future game shares portable indexed drawing code between Pico SDK and a
Sokol desktop host. The diagnostic deliberately establishes the simpler
single-core RGB565/DMA baseline before the indexed dual-core strip pipeline.

Downloaded references (32 files, about 48 MB) include schematics, datasheets,
vendor code, arcade manuals, screenshots and flyers. They remain local and
ignored by Git, with upstream rights retained. The original reference pack
lives in the original project checkout when working in a linked worktree.
No arcade ROMs or extracted game assets are included.
