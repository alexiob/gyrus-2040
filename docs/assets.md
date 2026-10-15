# Original pixel assets

The game uses newly authored indexed pixel art. The visual reference is the classic
circular arcade shooter: black space, tiny bright silhouettes, inward fire, coloured
attack formations, fiery rocks and explosions, and a six-planet journey. No ROM,
original sprite sheet, or extracted game pixels are included. The screenshots and
flyer in `reference/game` were viewed as visual research only.

## Rebuild

```sh
python3 tools/generate_assets.py
```

The generator requires only Python's standard library. Pixel matrices define the
ships; nearest-neighbour inverse rotation produces their heading frames. Small
planet surfaces and six explosion ages are generated deterministically. Its PNG
writer produces true pixel previews without smoothing. Re-running the generator
recreates `src/assets.c` and all files in `assets/` byte for byte.

`assets/contact-sheet.png` shows the visual family. `preview-gameplay.png` and
`preview-title.png` are illustrative **240 × 240 compositions**, not captured game
frames. Their `-4x` companions use exact pixel scaling for review. Individual PNG
files contain horizontal frame strips in API order. The C file is the runtime
source; the PNGs are review exports and are not linked into firmware.

## Runtime API

Include `gyrus/assets.h` and compile `src/assets.c`. The returned immutable
`gyrus_asset` holds a width, a height and a pointer to row-major, one-byte palette
indices. Index zero is transparent for sprite drawing and black for the scene.
The palette is normal RGB565; a display driver applies any transport byte swap.

```c
const gyrus_asset *ship = gyrus_sprite(GYRUS_SPRITE_PLAYER, heading);
/* Render source (x,y) as gyrus_palette[ship->pixels[y*ship->width+x]],
   skipping palette index zero. Position relative to the canvas centre:
   left = centre_x - ship->width/2, top = centre_y - ship->height/2. */
```

Headings begin at up and increase clockwise. Player frames use 32 headings;
scout, fighter, and moth frames use 16. To convert an unsigned full-turn angle
in the range 0–65535: `heading = ((angle + 1024u) >> 11) & 31u` for the player,
or `((angle + 2048u) >> 12) & 15u` for enemies. The API wraps a supplied heading
modulo the frame count. Invalid IDs return a null pointer and zero frame count.

| Asset | Canvas | Frames | Appearance |
| --- | --- | --- | --- |
| Player | 17 × 17 | 32 | Ivory hull, cyan wing tips, blue cockpit, red strakes |
| Scout | 15 × 15 | 16 | Red swept wings, amber nose and engines |
| Fighter | 15 × 15 | 16 | Amber delta with orange spine |
| Moth | 15 × 15 | 16 | Magenta split wings with pink leading edges |
| Satellite | 15 × 15 | 8 | Three cyan orbiting pods |
| Asteroid | 15 × 15 | 8 | Orange rock, brown craters, amber edge |
| Laser emitter | 15 × 15 | 8 | Cyan crystal; frames 4–7 use a yellow charge warning |
| Explosion | 21 × 21 | 6 | White/yellow core to expanding red sparks |
| Earth, Mars, Jupiter, Uranus, Neptune | 23 × 23 | 1 each | Distinct shaded surfaces |
| Saturn | 29 × 23 | 1 | Tipped amber ring and banded sphere |
| Life | 9 × 9 | 1 | Small interceptor |
| Player bullet | 1 × 4 | 1 | White core, cyan ends |
| Enemy bullet | 3 × 3 | 1 | White core, amber diamond |
| Powerup | 11 × 11 | 1 | Cyan/white energy cell |

The 24-colour palette deliberately reserves high contrast for active play.
Ivory/white/cyan identify the player and friendly fire; red/amber/magenta identify
attackers. Dark steel, deep blue and teal support faint stars. Yellow is suitable
for charge or imminent danger. The full 256-entry palette is exported so any
byte value remains a safe lookup; unused entries are black.

`gyrus_glyph(char)` returns seven immutable row bytes. Bit 4 is the leftmost pixel
of a 5 × 7 glyph. It covers uppercase letters, digits, common HUD punctuation,
and lowercase aliases. Advance six pixels for a one-pixel letter gap. Unsupported
and non-ASCII input returns a blank glyph. The letterforms are original simple
five-column matrices defined beside the sprite source in the generator.

## Cost and verification

There are 120 sprite frames using 31,621 bytes of indexed pixels, a 512-byte
RGB565 palette, and a 672-byte ASCII font. The Cortex-M0+ object compiled with
`-Os` contains **33,936 bytes of read-only data and 88 bytes of code**, with zero
`.data` and zero `.bss`. Descriptors and lookup tables account for the remainder.
All runtime art stays in flash: no heap, decompression buffer, runtime rotation,
or asset copying is required. The small cost makes unpacking a denser atlas
unnecessary for this project.

Validation performed:

- Generated pixel dimensions and all palette indices checked by the generator.
- Every sprite and every frame exercised through the C API under AddressSanitizer
  and UndefinedBehaviorSanitizer, including frame wrapping and invalid IDs.
- All signed and unsigned `char` values exercised against glyph lookup.
- Native C11 and Cortex-M0+ compilation with `-Wall -Wextra -Werror -pedantic`.
- All 25 generated files checked for byte-identical regeneration.
- Native compositions and contact sheet viewed for silhouette, contrast, spacing
  and palette consistency.

`assets/manifest.json` is the machine-readable dimension and pixel-byte report.
