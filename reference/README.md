# Local reference pack

Retrieved 15 October 2026. [manifest.json](manifest.json) records source URLs,
resolved URLs where available, byte counts, and SHA-256 hashes. Downloads are
kept locally and ignored by Git. Public availability does not grant a license
to redistribute arcade artwork or manuals as game assets.

## Hardware

| Local file | Purpose |
| --- | --- |
| [Waveshare wiki snapshot](hardware/waveshare-board.html) | Resource links and board overview |
| [Board schematic and silkscreen](hardware/board-schematic.pdf) | Pin routing, LCD connector, power and sensors; schematic p.1 |
| [GC9A01 controller datasheet](hardware/GC9A01A.pdf) | Pixel interface, window addressing, controller RAM, timing |
| [CST816S datasheet](hardware/CST816S.pdf) | Capacitive touch controller |
| [CST816S register description](hardware/CST816S-registers.pdf) | Coherent touch reads, finger count, IRQ/sleep controls |
| [QMI8658A datasheet](hardware/QMI8658A.pdf) | IMU configuration and timing |
| [RP2040 datasheet](hardware/rp2040-datasheet.pdf) | Clock tree, SRAM banking, DMA, SPI, PIO, electrical limits |
| [Pico C SDK manual](hardware/pico-c-sdk.pdf) | Official SDK APIs and build environment |
| [RP2040 hardware design guide](hardware/hardware-design-with-rp2040.pdf) | Clock, flash and power context |
| [Vendor C/Arduino/MicroPython demos](hardware/waveshare-demo.zip) | Actual board initialization and pin map |
| [Vendor LVGL demo](hardware/waveshare-lvgl-demo.zip) | DMA and touch integration comparison |

`hardware/vendor-excerpts/` contains selected original files from the basic
archive; `hardware/lvgl-excerpts/` contains selected LVGL integration files.
These excerpts are references only. The ZIPs are the authoritative downloaded
copies; their internal paths and original headers are preserved in the basic
excerpts. Text extracts next to the hardware PDFs are for searching, and are
not substitutes for the figures.

The board wiki currently links its RP2040 schematic from an **RP2350-named URL
directory**. The document itself is labeled RP2040-Touch-LCD-1.28. This was
checked visually; it is not the RP2350 circuit.

Primary source: [Waveshare board wiki](https://www.waveshare.com/wiki/RP2040-Touch-LCD-1.28).

## Arcade game and visual examples

| Local file | Purpose |
| --- | --- |
| [Konami-listed service manual](game/konami-service-manual.pdf) | 32-page scan; cover identifies Centuri. PDF pp.6-8 cover gameplay/scoring/settings |
| [Additional service manual](game/centuri-service-manual.pdf) | 27-page scan, alternate operator reference |
| [Conversion kit instructions](game/conversion-kit-instructions.pdf) | 5-page cabinet conversion reference |
| [Title screenshot](game/screenshot-title.jpg) | Original typography and composition reference |
| [Gameplay screenshot](game/screenshot-gameplay.jpg) | Sprite scale and starfield reference |
| [Additional screenshot](game/screenshot-formation.jpg) | Formation/composition reference |
| [Flyer front](game/flyer-front.jpg) | Contemporary promotional artwork |
| [Flyer back](game/flyer-back.jpg) | Manufacturer's contemporary promotional context |

Source indexes: [Museum of the Game](https://www.arcade-museum.com/Videogame/gyruss),
[Konami flyer](https://flyers.arcade-museum.com/videogames/show/455).
Small museum screenshots are examples, not complete sprite sheets. No sprite
sheet extracted from a ROM is included.

Additional viewing/research links:

- [Arcade gameplay recording](https://www.youtube.com/watch?v=CrrMQBLqqVo)
- [Journey-to-Earth recording](https://www.youtube.com/watch?v=f7nzhlQlLAg)
- [Arcade walkthrough](https://strategywiki.org/wiki/Gyruss/Walkthrough) - secondary guide; verify timings against gameplay
- [Amiga port source](https://github.com/jotd666/gyruss) - useful research lead, not a selected C dependency; no reuse license established here

Video links were identified, not downloaded or frame-analyzed. The secondary
walkthrough has incomplete trajectory descriptions. Exact arcade paths and
timings remain unverified.

## Software research snapshots

[software/revisions.json](software/revisions.json) pins the revisions inspected.

- Sokol README and license: architecture/platform requirements.
- `sokol-dyntex-sapp.c`: uploading a changing CPU-generated image to desktop graphics.
- Pico SDK README and license: native firmware workflow.
- `pico-sdk-board.h`: exact-board definition, including the LCD reset discrepancy.
- `pico-sdk-spi.c` and `.h`: actual divider calculation, FIFO cleanup, data formats.
- `pico-spi-dma-example.c` and license: DREQ-paced DMA integration example.
- `mame-gyruss.cpp` and `mame-COPYING`: hardware/video/input reference, **not** the original game's source or level logic.

These snapshots are not wired into a build. Keep the corresponding upstream
licenses when vendoring selected components during implementation. MAME's
driver file has its own BSD-3-Clause header; the entire MAME project has broader
licensing, and its license does not license original arcade ROM content.
