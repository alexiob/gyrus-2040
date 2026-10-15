# Gyrus 2040

A native C11 orbital shooter inspired by **Gyruss**, built for the
[Waveshare RP2040-Touch-LCD-1.28](https://www.waveshare.com/wiki/RP2040-Touch-LCD-1.28).
Play on its round touchscreen, or use the Sokol/Metal desktop simulator and
portable headless runner for development.

## Demo

https://github.com/user-attachments/assets/ca1884a3-ee16-48ce-9918-4bd2ffbf2fec

*On-device gameplay · 35 seconds · silent · 4.0 MiB.*
The same optimized [MP4 is included in the repository](docs/media/gyrus-2040-demo.mp4).

[Play](#play-on-the-board) · [Build](#build-and-run) · [Flash](#flash-the-board) ·
[Debug](#debugging) · [Hardware](#hardware-and-performance) · [Documentation](#documentation)

## Play on the board

**Autofire is enabled by default.** Tap **START**, choose a steering mode in
**Settings**, and steer. Tap the centre of the screen to pause.

### Steering

| Mode | How it works |
| --- | --- |
| **Left / Right** | Hold the left side to turn clockwise; hold the right side to turn counterclockwise. Release to stop. |
| **Border Drag** | Swipe around the rim to move the ship by the amount of your swipe. |
| **Tilt** | Lower the left or right edge like banking a tray. Hold still for half a second to calibrate in a comfortable pose. |

Settings shows tilt calibration progress and a live steering indicator. The
small ship arrow at the bottom of the title menu also follows your tilt.
**Tap that arrow, then hold still, to set a new neutral position.** A fresh
calibration survives starting or resuming a game.

### Menus and buttons

| Action | Input |
| --- | --- |
| Start a game | Tap **START** or press **BOOT**. |
| Pause | Tap the centre during play. |
| Change controls or difficulty | Open **Settings** from the title or pause menu. |
| Resume | Tap **Resume** or press **BOOT** in the pause menu. |
| Return from Settings | Tap **Back** or press **BOOT**. |
| Quit to the title | Choose **Quit** in the pause menu. |
| Reboot from the menu | Hold **Reboot** continuously for five seconds. |

The physical **RESET** button resets the chip immediately. BOOT can also fire
manually when autofire is disabled through the debug interface.

### Tune the game

Use the **− / +** buttons in Settings; no reflash is needed.

| Setting | Range | Default |
| --- | --- | --- |
| Fire rate | 1–10 volleys/s | 5 volleys/s |
| Enemy speed | 25–200%, in 25% steps | 100% |
| Tilt sensitivity | 50–200%, in 25% steps | 100% |

Selections survive quitting and starting new games. Reboot restores defaults.

### Gameplay

The game includes spiral formations, inward fire, diving enemies, meteors,
satellite double fire, laser pairs, six planetary destinations, chance stages,
extra lives, and game over. Wave clears lead into an inward shrinking-ship warp;
planet arrivals precede chance stages, and bonus results appear before departure.

The art and timings are authored for this round display. The
[game research](docs/research/game-reference.md) and
[transition observations](docs/research/gyruss-transitions.md) explain which
behaviors are supported by the original arcade references.

![Game framebuffer captured over USB](docs/hardware/game-combat.png)

## Build and run

### Requirements

The bundled setup targets **macOS on Apple silicon**. Install **Python 3,
CMake, Ninja, and Git**, then run:

```sh
python3 tools/setup.py
```

Setup downloads pinned dependencies into `.deps/`: the Arm toolchain, Pico SDK,
picotool, pyserial, and Sokol headers. It uses no privileged installer and does
not replace the system compiler.

### Desktop simulator

```sh
cmake --preset host
cmake --build --preset host
ctest --preset host
open build-host/gyrus_desktop.app
```

| Input | Action |
| --- | --- |
| Arrow keys or A / D | Turn. |
| Mouse | Emulate touchscreen input. |
| Enter / Escape | Start / pause. |
| M | Cycle steering modes. |
| J / L | Bank left / right in tilt mode. |
| 0 or K | Recenter tilt. |
| Space | Emulate BOOT. |

The debug server listens on `127.0.0.1:20400`. To select another port:

```sh
build-host/gyrus_desktop.app/Contents/MacOS/gyrus_desktop --debug-port 20401
```

The core, renderer, controls, and protocol use C11; the macOS window/Metal bridge
uses Objective-C. Other hosts can build the portable headless runner with
`-DGYRUS_BUILD_DESKTOP=OFF`; the bundled dependency installer downloads macOS
arm64 tools.

### RP2040 firmware

```sh
cmake --preset firmware
cmake --build --preset firmware
```

| Output | Purpose |
| --- | --- |
| `build-rp2040/gyrus_game.uf2` | Playable game. |
| `build-rp2040/gyrus_board_diag.uf2` | Display patterns, touch tests, and SPI benchmarks. |

The build also produces ELF files, linker maps, and disassembly.

## Flash the board

1. Unplug the board, hold **BOOT** while reconnecting USB, then release it.
2. Wait for the board to enumerate in **BOOTSEL** mode. With one matching board
   connected, verify it and load the game:

```sh
.deps/picotool/picotool/picotool info --vid 0x2e8a --pid 0x0003
.deps/picotool/picotool/picotool load -v -x build-rp2040/gyrus_game.uf2 --vid 0x2e8a --pid 0x0003
```

A running game can also enter BOOTSEL through USB. Replace `BOARD_SERIAL` with
your board's serial number:

```sh
.deps/picotool/picotool/picotool reboot -u --ser BOARD_SERIAL -f
```

## Debugging

The **G1 protocol** supports state inspection, bounded field changes, logical
input injection, freeze/step, level selection, seed reset, and CRC-checked
framebuffer capture. It works over USB CDC, desktop loopback TCP, or headless
stdin/stdout.

### USB and desktop examples

Replace `/dev/cu.usbmodemXXXX` with your board's serial port.

```sh
# Read the connected board's state and capture its screen.
.deps/venv/bin/python tools/gyrusctl.py --serial /dev/cu.usbmodemXXXX STATE
.deps/venv/bin/python tools/gyrusctl.py --serial /dev/cu.usbmodemXXXX -o capture.png FRAME

# Select tilt controls and jump to a stage in the desktop simulator.
python3 tools/gyrusctl.py --tcp 20400 SET control_mode 2
python3 tools/gyrusctl.py --tcp 20400 LEVEL 6

# Freeze the debug scheduler and advance exactly 120 updates.
python3 tools/gyrusctl.py --tcp 20400 PAUSE
python3 tools/gyrusctl.py --tcp 20400 STEP 120
```

### Compare device and native output

This repeatable smoke test checks **36 scenarios**, comparing simulation state
hashes and every captured framebuffer pixel:

```sh
.deps/venv/bin/python tools/smoke_game.py --serial /dev/cu.usbmodemXXXX
```

Serial access has one owner at a time. The CLI clears stale input when connecting;
after an output deadline abort, close and reopen the connection. Protocol
`PAUSE` freezes the debug scheduler; the game's on-screen pause is a separate menu.

For headless scenarios, use `--batch commands.txt` to keep all commands in one
process; a headless process exits with its state. See the
[complete protocol reference](docs/debug-protocol.md) for commands and examples.

## Hardware and performance

See the [official Waveshare hardware specifications and documentation](https://www.waveshare.com/wiki/RP2040-Touch-LCD-1.28)
for the board, pinout, schematics, and vendor resources.

| Component | Configuration |
| --- | --- |
| MCU | Dual-core RP2040; CPU/peripheral clocks at 125 MHz, without CPU overclocking. |
| Memory | 264 KiB SRAM; firmware linked for 4 MiB flash. The tested board reports 16 MiB physical flash. |
| Display | 1.28-inch round touchscreen, 240 × 240 pixels, RGB565 color. |
| Display link | SPI at 62.5 MHz; USB clock at 48 MHz. |
| Motion | QMI8658A accelerometer and gyroscope with synchronized six-axis reads. |
| Simulation | Fixed 60 Hz input and gameplay on core 0. |
| Presentation | About 60 frames/s in observed runs; roughly 14.4 ms per optimized transfer. |
| Pixel buffers | Two indexed frames and two RGB565 strips: 130,560 bytes total. |

Core 0 renders indexed frames while core 1 converts 16-row strips and sends
them over SPI DMA. After one full display initialization, circular transfer
windows send **49,120 rather than 57,600 pixels**, reducing pixel traffic by
14.7%. Rendering and touch processing continue independently of DMA. To build
for 30 Hz presentation, set `-DGYRUS_PRESENT_HZ=30`.

Gameplay pools and pixel buffers allocate no memory per frame. The linker
reserves 8 KiB for core 0's USB/debug stack in main SRAM and 4 KiB for core 1 in
scratch SRAM, with the heap bounded below core 0's stack. Because BOOT shares
flash CS, its SRAM-only sampler uses coordinated multicore flash lockout.
No TE wire is exposed, so buffering cannot guarantee synchronized panel scanout.

## Documentation

| Topic | Read more |
| --- | --- |
| Controls, calibration, and IMU | [Controls guide](docs/controls.md) |
| Gameplay and scoring | [Gameplay guide](docs/gameplay.md) |
| Debug commands and automation | [G1 protocol](docs/debug-protocol.md) |
| Pixel art and deterministic generation | [Assets](docs/assets.md) |
| Display architecture and bottlenecks | [Display performance](docs/research/display-performance.md) |
| Original game and transition references | [Game research](docs/research/game-reference.md) · [Transitions](docs/research/gyruss-transitions.md) |
| Source material and provenance | [Reference index](reference/README.md) · [Download manifest](reference/manifest.json) |
| Third-party components | [Notices](THIRD_PARTY.md) |
| Demo encoding and playback | [Video details](docs/media/README.md) |

<details>
<summary>Hardware validation reports</summary>

- [Physical tilt axes and tap-to-recenter](docs/hardware/2026-10-15-tilt-recenter.md)
- [Settings, 3D tilt, and transition verification](docs/hardware/2026-10-15-controls-transitions.md)
- [On-device settings verification](docs/hardware/2026-10-15-settings.md)
- [Game implementation and validation](docs/hardware/2026-10-15-game.md)
- [Board bring-up and diagnostic results](docs/hardware/2026-10-15-bringup.md)

</details>

## Scope and local files

This project uses original pixel art inspired by Gyruss. No arcade ROMs,
extracted game assets, or original game audio are included. The board has no
onboard speaker, and the demonstration video has no audio track.

Downloaded references stay local and ignored; their upstream rights are retained.
Builds, dependencies, flash backups, and development captures also stay local
under the ignored `build-*`, `.deps/`, and `artifacts/` directories. The original
flash backup was independently verified over its first 1 MiB, covering the
firmware written during bring-up.
