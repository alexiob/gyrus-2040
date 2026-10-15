# Native Gyruss adaptation for RP2040 round LCD

**Status: architecture approved by the user on 15 October 2026. Diagnostic implementation authorized; game implementation remains a later milestone.**
Date: 15 October 2026.

## Intent and success

Build a native C adaptation of Konami's 1983 Gyruss for Waveshare's
RP2040-Touch-LCD-1.28. The user confirmed the arcade reference, asked for a
downloaded research pack and portable high-quality C, and emphasized studying
the actual display circuit, CPU/RAM/GRAM clocks, bottlenecks and speed tricks.
The initial brief anticipated connecting hardware later; the board is now
connected. Development must also remain productive on desktop, with firmware
cross-buildable independently of device availability.

Success for the first implementation is a playable desktop orbital shooter,
the same deterministic gameplay compiled into an RP2040 UF2, meaningful host
tests, a board diagnostic mode, and documented unverified hardware checks.
A build alone will never be described as tested on the device.

Assumptions proposed for review: newly authored compact graphics, touch orbit
steering with automatic fire on the board, keyboard/mouse controls on desktop,
a single-player free-play game, and adapted circular HUD. Exact ROM-derived
graphics, audio, timings and all arcade enemy trajectories are not prerequisites
for the first playable build.

## Approaches and decision

1. **Recommended:** portable C11 simulation + indexed software renderer;
   Pico SDK SPI1/DMA board host; Sokol desktop host. This gives control of
   memory and identical scene composition across platforms.
2. Same core with SDL3 instead of Sokol on desktop. A reasonable C alternative,
   with a larger desktop dependency but simpler pixel-window integration.
3. LVGL as the common graphics layer. Useful for elaborate GUI screens, but
   unnecessary widget overhead and another dependency in the game renderer.

Choose option 1. Sokol's supported graphics APIs do not include this MCU.
The portable boundary is a rendered indexed image and platform-neutral game
input, not a graphics API that assumes GPU hardware.

## Modules and dependencies

| Module | Responsibility | Dependencies |
| --- | --- | --- |
| `core/game` | State transitions, fixed-step simulation, score, lives, RNG | Standard fixed-width C types, game data |
| `core/math` | Bounded fixed-point arithmetic, angles, lookup tables | Standard C only |
| `core/levels` | Explicit route, formations, authored paths and difficulty | Immutable constant data |
| `render` | Clipping, indexed sprite/font blits, starfield, circular HUD | Game snapshot, asset tables |
| `assets` | Authored palette sprites, font, orientation/explosion frames | Constant C data generated offline if useful |
| `platform/headless` | Deterministic stepping, frame export, benchmark/test runner | Core + renderer only |
| `platform/desktop` | Window, keyboard/mouse, texture upload, timing | Sokol + core + renderer |
| `platform/rp2040/board` | Pins, clocks, initialization, diagnostics | Pico SDK |
| `platform/rp2040/display` | GC9A01 commands, region planner, palette expansion, SPI1/DMA ownership | Board + Pico SDK; no gameplay calls |
| `platform/rp2040/input` | CST816S events, optional QMI8658 sampling and calibration | Board + Pico I2C/GPIO |

Do not include platform headers in core/render public interfaces. No runtime
file loading is required on the MCU. Drivers accept explicit contexts and
buffer lengths, rather than implicit mutable global game state.

## Gameplay and timing

Run simulation at 60 fixed steps per second, with a rational/integer deadline
accumulator that avoids drift. Presentation can run at 30 or 60 Hz independently.
Bound catch-up work and resynchronize after pause or long host stalls; do not
turn an overloaded renderer into a different gameplay speed.

Use uint16_t modular angles (one turn = 65,536 units), documented fixed-point
positions/velocities, a seeded integer RNG and fixed entity pools. Define
rounding and overflow behavior. Render a snapshot without changing simulation
state. Replay inputs plus seed reproduce gameplay; tests compare canonical
fields/serialization, not raw structs with padding.

Game flow: title/start -> warp introduction -> combat -> planet arrival ->
chance stage -> next destination, with death/respawn and game-over states.
The route and uncertainty about original stage numbering are documented in
[game-reference.md](../../research/game-reference.md). Use explicit countdowns
rather than special-case arithmetic spread across modules.

The first playable milestone includes a complete Neptune combat/chance loop.
The full route can then reuse authored pattern templates with per-planet
variation. Add satellites/double fire, meteors and paired beams as distinct
behaviors. Arcade fidelity is a later measured refinement, not an invented
claim attached to approximate paths.

## Graphics and memory

Render at 240 x 240 using 8-bit palette indices and a fixed 256-entry RGB565
palette. Reserve one transparency index in sprite sources. Author assets as
small indexed frames. Precompute orientations and trig tables; keep the HUD and
ship extents inside the visible circle. Use the same software drawing code
for desktop, headless and firmware.

Use two 57,600-byte indexed frames and two 240 x 16 RGB565 transfer strips.
Together with the palette these use exactly 128 KiB. The total static planning
budget is 212 KiB, leaving 52 KiB headroom; verify actual sizes at link time.
Reject a design that silently substitutes two full RGB565 frames and consumes
almost all SRAM.

Core 0 handles input, simulation and rendering. Core 1 owns the display pipeline.
Compare a newly published immutable frame to the last fully presented image,
build a bounded dirty-region list, release the old buffer for rendering, and
send the published image via alternating palette-expanded strips. Never
release a source while it is still being read. Use SDK synchronization and
memory barriers for ownership transitions.

Keep blocking diagnostic and single-core DMA paths as baselines. The optimized
dual-core path must preserve the same pixel results. Full-frame fallback is
required for dense changes and for recovering after panel reset.

## Hardware display contract

Use the schematic's GPIO13 LCD reset, explicit SPI1 and actual achieved clocks.
Override the erroneous GPIO12 SDK macro before inclusion. Use the exact SDK
board type for flash/platform defaults, not its general-purpose `spi_default`.

Baseline boot: 125 MHz system/peripheral clock, conservative SPI writes,
vendor-compatible reset/init, RGB test pattern, touch ID/readout. Qualify a
62.5 MHz SPI pixel profile on hardware; do not default to the vendor demo's
270 MHz CPU setting. 133/66.5 MHz and PIO are separately selectable experiments.

Driver API expectations: initialize, set brightness, submit immutable frame,
query/wait completion without blocking the simulation core, collect statistics,
and recover from a timed-out transfer. Rectangles are half-open at the API and
converted to inclusive controller endpoints. Command bytes are 8-bit; RGB565
pixel DMA uses 16-bit MSB-first words with no redundant byte swap.

DMA completion and SPI wire idle are separate. CS/DC or word-size changes
require wire idle. RX FIFO cleanup and sticky overrun handling are explicit.
The display driver alone owns the SPI peripheral; game code never accesses it.

For the detailed clocks, wire budget, buffering, dirty rectangles, circular
spans, XIP/SRAM considerations and experiments, see
[display-performance.md](../../research/display-performance.md).

No TE signal is exposed in the schematic. The implementation must not promise
tear-free scanout. Scanline readback can be investigated after reliable basic
read transactions; it is not required for first bring-up.

## Input and interaction

Default board play: touch in an annulus sets a target angle, with a bounded
turn rate and shortest-path steering, a central dead zone, and automatic fire.
Touch release stops steering; retain the last angle. The title uses a deliberate
tap to start. A center long press pauses/resumes; pause suppresses gameplay
steering and firing. Establish the final annulus/dead-zone sizes on the device.

Desktop: left/right or A/D orbit, Space fires, Enter starts, P pauses; mouse
drag in the circular viewport exercises touch steering and automatic fire.
The input adapter chooses a mode deterministically when keyboard and pointer
events overlap. Both adapters emit the same core command structure.

Read touch on IRQ with bounded fallback polling and transaction timeouts.
Only core 0 owns I2C1. Detect finger release and stale samples; do not perform
blocking I2C inside interrupts. IMU register/identity diagnostics are part of
bring-up; tilt steering is optional follow-up after touch works, with stationary
bias calibration and drift/dead-zone testing.

## Audio and persistence

The documented board has no onboard audio output. Keep an event interface for
shots/explosions but do not invent a speaker pin or require external hardware
for the first build. Desktop sound and an external PWM/I2S output can be added
later. Original arcade audio is not part of the downloaded asset set.

Initial high score lives in RAM. Add flash persistence only with a deliberate
wear/power-loss design and a presentation-safe point; programming XIP flash
while both cores are running from it is not a routine per-frame operation.

## Errors and verification

I2C reads check length, ID and timeout. Input failure returns neutral input and
a diagnostic state rather than stale unbounded motion. DMA failure enters a
bounded recovery path and requests a full panel redraw. Optional IMU failure
does not prevent touch play. Capacity exhaustion is deterministic and recorded;
no array overrun or fallback heap allocation is permitted.

Host verification covers clipping and 1-pixel window geometry, angle wrap,
fixed-point bounds, entity pool exhaustion, collision edge cases, replay
determinism, scoring/state transitions, chance-stage safety and dirty-region
coverage relative to a full reference frame. Simulate delayed display
completion to check frame/strip ownership and backpressure.

Use warnings, ASan/UBSan on host, CTest, static analysis and clean native/ARM
builds. Inspect ELF/map/stack budgets. Benchmark high-entity and dense-starfield
scenes rather than a title screen alone. Hardware acceptance additionally
requires electrical captures, corruption/tearing inspection, actual clocks,
sensor interaction and sustained operation on USB/battery.

## Delivery boundary

The architecture was approved and the diagnostic milestone is now implemented.
It includes native toolchain setup, portable model/tests, board SPI/DMA and
touch drivers, flashed UF2, serial harness and measured hardware results. See
[bring-up report](../../hardware/2026-10-15-bringup.md). The game, Sokol simulator
and indexed dual-core pipeline remain later milestones.

No device connection is needed for host implementation, deterministic tests,
asset authoring, ARM cross-compilation or memory-size checks. Device-dependent
performance and interaction remain visibly marked until measured.
