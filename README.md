# Gyrus 2040

A proposed native C adaptation of **Gyruss (Konami, 1983)** for the
Waveshare **RP2040-Touch-LCD-1.28**, with a desktop simulator for development
before the hardware is connected.

## Current state

Research and a concrete architecture proposal are prepared. There is **no game
implementation, build system, simulator, or flashable firmware yet**. No device
has been connected or measured. Performance numbers are calculated bounds,
not benchmark results.

- [Downloaded references and provenance](reference/README.md)
- [Display schematic, CPU/memory analysis, and optimization design](docs/research/display-performance.md)
- [Game reference and level structure](docs/research/game-reference.md)
- [Toolchain audit and library decisions](docs/research/toolchain.md)
- [Proposed architecture for review](docs/superpowers/specs/2026-10-15-gyruss-rp2040-design.md)

## Recommended direction

Use a portable C11 simulation and indexed software renderer. Use **Pico SDK +
SPI1 DMA** on the board, and **Sokol** to present the same pixels on desktop.
Keep GPU/windowing APIs outside the game. The RP2040 has no Sokol graphics
backend.

Use two 240 x 240 indexed frames, a fixed RGB565 palette, and two small DMA
conversion strips. Start at a measured, conservative SPI rate and qualify
62.5 MHz at a 125 MHz CPU/peripheral clock. Keep simulation at 60 Hz even when
presentation must run at 30 Hz. Details and alternatives are in the design.

## Important discoveries

1. **Pin discrepancy:** the pinned Pico SDK board header says LCD reset GPIO12;
   the Waveshare schematic and vendor C driver say **GPIO13**, with GPIO12 used
   for LCD MISO. Use explicit project pins and override the SDK LCD reset macro.
2. **Clock quantization:** requesting 40 MHz SPI at 125 MHz gives **31.25 MHz**.
   Increasing CPU/peripheral clock to 133 MHz while requesting 62.5 MHz gives
   **33.25 MHz**, not 62.5 MHz. Always report the actual achieved SPI clock.
3. **No exposed TE signal:** the schematic's display connector has no tearing
   synchronization output. MCU double buffering cannot guarantee tear-free
   panel scanout.
4. **Compiler issue:** `/usr/local/bin/arm-none-eabi-gcc` is an x86_64 executable
   that fails with `bad CPU type in executable` on this Mac. It must be replaced
   or bypassed with a compatible toolchain before a firmware build can pass.

## Reference handling

The local reference pack includes manuals, screenshots, flyers, datasheets,
vendor archives, and selected open-source examples. These are research
references, not game assets or installed build dependencies. Their upstream
rights and licenses remain in effect. Downloads are excluded from Git by
default; their URLs, sizes, and SHA-256 checksums are in
[reference/manifest.json](reference/manifest.json). No arcade ROMs were fetched.

Research date: **15 October 2026**.
