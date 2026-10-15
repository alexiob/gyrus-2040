# RP2040 Board Diagnostics Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build, flash and measure a native C diagnostic on the connected Waveshare RP2040-Touch-LCD-1.28.

**Architecture:** A portable C11 module owns window geometry, touch decoding and diagnostic images. Pico-only modules own the board, GC9A01 SPI/DMA and I2C sensors. USB CDC provides commands and machine-readable results; the hardware stays at 125 MHz.

**Tech Stack:** Pico SDK 2.3.1, native macOS arm64 GNU Arm compiler, CMake/Ninja, CTest, Python serial harness, picotool.

**Spec:** `docs/superpowers/specs/2026-10-15-gyruss-rp2040-design.md` (user approved architecture and explicitly authorized setup/build/flash/tests on 15 October 2026).

## Global Constraints

- C11; no heap in drawing or transfer loops.
- Explicit SPI1 and GPIO13 LCD reset; override SDK GPIO12 macro.
- 125 MHz system/peripheral clock; USB stays 48 MHz.
- Commands 8-bit; native RGB565 pixels 16-bit MSB-first; DMA completion differs from wire idle.
- API rectangles half-open; input uses checked, timed burst reads on one I2C owner.
- Diagnostic milestone only; Sokol/gameplay and full dual-core frame pipeline remain later milestones.
- Device flashing is explicitly authorized. Preserve existing flash before replacement if readable.

## Review Focus

- Window zero size/overflow and bottom-right pixel: reject invalid rectangles, inclusive conversion correct.
- Missing I2C device or short read: report error, never invent a valid touch.
- DMA timeout or lingering shift register: bounded recovery, no early CS/DC transition.
- USB disconnect/malformed command: no unbounded buffering or reboot by arbitrary bytes.
- Sensor traffic during display DMA: service touch on main core while a transfer is in progress, record read latency.

### Task 1: Portable diagnostic model and build/toolchain

Files: CMakeLists.txt, CMakePresets.json, include/gyrus/diagnostic.h, src/diagnostic.c, tests/test_diagnostic.c, tools/setup.sh, dependencies.lock.json.
Interfaces: `diag_window_encode(x,y,w,h,out[8])`; `diag_touch_decode(bytes[5],sample*)`; `diag_pattern_pixel(pattern,x,y)` returns RGB565.
- [ ] Write tests asserting rejected empty/out-of-range windows, (239,239,1,1) -> endpoints 239, RGB565 bars, finger release and invalid coordinates.
- [ ] Run host test; observe missing/unsupported behavior fail.
- [ ] Implement portable model and CMake host/firmware selection; obtain pinned SDK/toolchain locally without modifying existing global compiler.
- [ ] Run CTest with sanitizers; compiler must execute and produce Cortex-M0+ objects; commit.

### Task 2: Native board firmware and serial diagnostics

Files: firmware/board.h, firmware/display.h, firmware/display.c, firmware/sensors.h, firmware/sensors.c, firmware/main.c, firmware/lcd_init.inc, THIRD_PARTY.md.
Interfaces: display init, submit constant/frame pixels by polling/DMA, completion polling with timeout, baud selection; checked sensor probe/read; bounded line protocol.
- [ ] Establish pre-flash USB/BOOTSEL identity and preserve readable flash to ignored local backup.
- [ ] Use Task 1 tested window/touch functions in real driver. Retain attributable vendor LCD initialization table, implement control/stream logic separately.
- [ ] Add RGB bars, checkerboard, touch cursor, status and commands info/pattern/bench/touch/boot. Benchmark polling and DMA separately; include actual clocks, payload, samples and min/mean/max.
- [ ] Build ELF/UF2/map, inspect SRAM and warnings, flash only the identified RP2040, verify running USB CDC identity; commit.

### Task 3: Hardware exercise and evidence

Files: tools/diagnose.py, docs/hardware/2026-10-15-bringup.md, ignored artifacts/hardware logs and backup.
Interfaces: serial JSON-line commands/replies from Task 2, nonblocking reader with finite deadlines.
- [ ] Run info/probe, display patterns, SPI benchmarks at actual 15.625/31.25/62.5 MHz, window tests and touch sampling during DMA.
- [ ] Request the user's physical touch and visual observations while collecting actual samples; do independent benchmark work while waiting.
- [ ] Record automated evidence versus user observations versus untested electrical/scanout properties. Keep raw logs and reproducible commands.
- [ ] Run full host suite and clean firmware build; review branch; fix important findings; commit evidence and instructions.

## Execution decision

Execute inline in the managed worktree. The user's explicit instruction to proceed through flashing/testing supplies authorization for this milestone; no additional approval round is needed. Physical touch and visual confirmation require user participation, not another deployment permission.
