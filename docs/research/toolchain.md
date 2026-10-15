# Toolchain audit and portable C choices

Audited locally on 15 October 2026. This is a setup proposal; dependencies are
not installed or integrated into a product build yet.

## What runs on this Mac

| Tool | Observed result |
| --- | --- |
| Apple Clang | 21.0.0, arm64 target; executable runs |
| Xcode SDK | `xcrun --show-sdk-path` succeeds |
| CMake | 4.4.4 |
| Ninja | 1.13.2 |
| Python | 3.14.6 |
| Git | Available via Homebrew |
| Poppler tools | `pdfinfo`, `pdftotext`, `pdftoppm` available and used |
| ARM GCC on PATH | `/usr/local/bin/arm-none-eabi-gcc`, Mach-O x86_64; **fails** with `bad CPU type in executable` |

There was no Git repository or source tree in this workspace at the start.
No existing build/test flow was available to preserve.

Before firmware implementation can be verified, select a current native
macOS arm64 **arm-none-eabi** toolchain with the C runtime libraries, CMake
integration, objcopy and size tools. Use a project-local toolchain directory
and explicit toolchain search path so the broken `/usr/local/bin` executable
cannot silently win. Verify the compiler executes and can link a Cortex-M0+
program before building the full game. Do not replace the user's system
compiler symlink as part of project setup.

Start with [Raspberry Pi's setup guidance](https://www.raspberrypi.com/documentation/microcontrollers/c_sdk.html)
and the [Arm toolchain landing page](https://developer.arm.com/Tools%20and%20Software/GNU%20Toolchain)
to select the current host-compatible distribution; avoid stale download URLs.

## Selected firmware foundation

Recommend **Pico SDK 2.3.1**, pinned to
`079c6f39023649b154152db30f1d781e884879bc`. The official
[release](https://github.com/raspberrypi/pico-sdk/releases/tag/2.3.1) was checked
and the exact-board header inspected. Use `PICO_BOARD=waveshare_rp2040_touch_lcd_1.28`
with an explicit LCD reset override to GPIO13 and explicit SPI1/pins in the
project board module. Retain the SDK board's conservative 4 MiB flash budget.

Use CMake + Ninja, C11, Pico SDK hardware SPI/DMA/I2C/PWM/clocks APIs, USB serial
diagnostics and UF2 output. Add PIO only if benchmarks justify it. Start without
an RTOS. Both the game core and renderer must build without including Pico
headers; only the board/platform modules depend on the SDK.

## Desktop graphics choice

[Sokol](https://github.com/floooh/sokol) is suitable for a thin desktop host.
It wraps desktop/mobile/browser graphics APIs, not this MCU's serial LCD.
Its current macOS implementation translation unit must compile as Objective-C
and link platform frameworks. Keep that bridge confined to one `.m` file;
gameplay, rendering and data remain C. On other hosts, use the documented C
backend configuration. The dynamic-texture example is downloaded for research.

Pin Sokol to `5e94d1291fea504ed911090338cf35bd9ef49a06` and the inspected sample
revision to `dff9f8c4e0a551edb641e0fa5c9613b0f4fd258d`. Build a simple nearest-
neighbor full-screen triangle presenting the shared indexed renderer's pixels
after palette expansion. This avoids pretending that the RP2040 has shaders
or a GPU. Keep a headless host that exports frames and runs deterministic
simulations without a windowing dependency.

Alternatives considered:

| Option | Advantage | Cost / reason not selected initially |
| --- | --- | --- |
| Sokol desktop + small shared software renderer + Pico DMA | Matches requested C direction; precise MCU memory/control; same rendered scene on both targets | Need a small macOS Objective-C bridge and a project renderer |
| SDL3 desktop + same MCU backend | Straightforward pixel/keyboard/audio host with C APIs | Another large desktop dependency; good alternative if host simplicity outweighs the Sokol preference |
| LVGL on MCU and desktop | Established embedded C GUI, small-buffer display integration and touch widgets | Widget/layout machinery is unnecessary for this game's hot drawing path; vendor demo is a reference, not a default runtime |

[LVGL](https://github.com/lvgl/lvgl) remains a reasonable future choice for
complex settings screens. The supplied demo bundles LVGL 8.1 per the board
wiki and uses a 270 MHz configuration; neither should be adopted blindly.
Using it would require a deliberate version and memory/performance assessment.

## C quality contract proposed for implementation

- Public headers have explicit types, capacities and ownership contracts.
- No heap allocations in game, drawing or transfer loops; fixed entity pools
  and compile-time resource limits.
- Fixed-step deterministic simulation with explicit RNG seed; no dependency on
  wall-clock calls, global platform state, libm trig or desktop floating point.
- Documented fixed-point arithmetic ranges and rounding; explicit wrap for
  angles; wider intermediates where required to avoid signed overflow.
- Pure rendering from immutable game snapshots; no gameplay mutations while
  drawing, no dependence on presentation rate.
- Check I2C/DMA errors and timeouts; return status instead of silently using
  uninitialized sensor values. Recovery is bounded and logged outside IRQs.
- Strong warnings on project code, including conversion/shadow/prototype
  checks where supported; isolate external-header warnings. Use sanitizers on
  the desktop/headless tests, static analysis and a clean ARM cross-build.
- Validate entity counts, clipping, fixed-point edge cases, stage transitions,
  replay determinism and DMA ownership; avoid tests that merely restate code.
- Inspect linker map and stack usage; reserve headroom rather than treating all
  264 KiB as an available framebuffer allocation.

## Build artifacts to provide during implementation

Planned independent CMake build directories for headless tests, desktop and
firmware. Dependency downloads must be pinned and explicit; headless tests
must work without downloading a GPU host or an ARM SDK. Firmware builds must
produce ELF, UF2, linker map and section-size report. Document exact configure,
build, test and BOOTSEL copy commands once those targets actually exist.

Do not present speculative build commands as working instructions. No firmware
or simulator build has been attempted in this research-only stage.
