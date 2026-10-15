# Third-party notices

Pico SDK 2.3.1 is used from its pinned upstream repository. Its BSD-3-Clause
license and the TinyUSB submodule's license remain in `.deps/pico-sdk/`.
Distributed firmware includes those components; distribute their license files
with binary releases.

`firmware/lcd_init.inc` repacks panel initialization register values from
Waveshare's `RP2040-Touch-LCD-Code.zip`,
`RP2040-Touch-LCD-Code/RP2040-Touch-LCD-1.28/c/lib/LCD/LCD_1in28.c`.
Source: https://files.waveshare.com/wiki/RP2040-Touch-LCD-1.28/RP2040-Touch-LCD-Code.zip
Retrieved 2026-10-15, ZIP SHA-256 is recorded in the reference manifest.
The companion DEV_Config.h carries this permission (original spelling retained):

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documnetation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of theex Software, and to permit persons to whom the Software is
    furished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.

No original Gyruss ROM, graphics or audio are included in this project.

## Sokol desktop host

The optional macOS host uses `sokol_app.h`, `sokol_gfx.h`, `sokol_glue.h`, and
`util/sokol_gl.h` from [floooh/sokol](https://github.com/floooh/sokol), pinned to
commit `5e94d1291fea504ed911090338cf35bd9ef49a06`, retrieved 2026-10-15.
Downloaded headers remain in ignored `.deps/sokol/`; SHA-256 hashes are recorded
in `dependencies.lock.json`. The headers contain their original zlib/libpng
license notices, including Copyright (c) Andre Weissflog and contributors.
Preserve those notices when distributing the headers. The unmodified library
code is used only by the desktop host, not the RP2040 firmware.

The zlib/libpng terms permit commercial use, modification and redistribution,
require that origin is not misrepresented, altered versions are marked, and
the license notice is preserved in source distributions. The software is
provided as-is without warranty. See the complete notice in each pinned header.

`firmware/linker/sections_stack.incl` and `section_end.incl` adapt the Pico SDK
2.3.1 BSD-3-Clause linker fragments to reserve an 8 KiB core0 stack in main SRAM.
Copies of the Pico SDK, TinyUSB and Sokol license notices are retained in
`LICENSES/` for source and binary distribution.
