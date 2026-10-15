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

No original Gyruss ROM, graphics or audio are included in this diagnostic.
