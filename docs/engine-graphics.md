# Shared graphics and launcher

The shared indexed renderer lives in `src/engine`. It has no input polling,
wall-clock access, allocation or retained animation state. A render call reads
its const view and writes exactly one 240 × 240 byte-indexed framebuffer.
Palette pointers remain immutable ROM data; frame publication/presentation owns
which pointer travels with a completed indexed frame.

## Resources and compatibility

- `draw.c`: clipped pixels, rectangular fills, the existing integer Bresenham
  line rule, 5×7 text placement, decimal conversion, and final circular mask.
  The first 24 `engine_palette` entries match the Gyrus RGB565 palette exactly;
  entries 24–255 default to black. The launcher only emits the first 24 indices.
- `font.c` / `font_data.inc`: one shared 96-glyph ASCII table, seven rows per
  character. Unsupported characters map to the blank glyph. `gyrus_glyph()`
  remains a compatibility wrapper. The generator emits the shared font and
  retains every Gyrus sprite/palette byte.
- `math.c`: the existing 256-sample Q15 sine table, quarter-turn cosine and
  polar projection. Radius is clamped to ±200×256, multiplication and division
  preserve C truncation toward zero, and the centre stays (120,120).
  `game_sin_q15()`, `game_cos_q15()` and `game_polar()` delegate to these functions.
- `ui.c`: shared Settings and pause menus consume only `engine_menu_view`:
  settings values plus control/tilt/reboot telemetry. Existing button rectangles,
  dimming table, labels and progress clamps are preserved. The function overlays
  the caller's background; the caller applies the final circular mask.

`src/render.c` keeps the original Gyrus sprites/scenes while using shared draw
helpers and menu views. The standalone Gyrus title remains unchanged. Session
rendering can add `engine_ui_home_button()` to an active game's title or game-over
screen; that common back target occupies [14,48) × [94,128). Keeping it outside
standalone `game_render()` preserves legacy capture compatibility.

`game_set_setting()` delegates validation/mutation to `engine_settings_set()`.
It resets the Gyrus fire cooldown and fractional firing remainder only when the
accepted fire-rate value actually changes. Failed settings and repeated accepted
values preserve those timers.

Coordinates supplied to line/fill helpers are bounded game/UI coordinates,
including off-screen carousel positions; pixel writes clip to the framebuffer.
Drawing primitives do not validate arbitrary untrusted full-range integer
coordinates or colour values. Public game renderers remain responsible for
emitting valid palette indices and applying `draw_mask_round()` before presenting.

## Launcher composition

`engine_hub_render()` consumes the value-only `engine_hub_view`, whose catalogue
contains immutable game descriptors. It dispatches the descriptor's `draw_title`
callback and never switches on a game identifier. An invalid selected index
falls back to the first catalogue entry; a missing/empty catalogue renders a
safe empty state. No game simulation is initialized or mutated by drawing.

The selected card centre is `120 + slide_pixels`; neighbours are exactly 220
pixels to each side. Small outer rails provide visible neighbour hints at rest.
The session owns selection and easing; the renderer uses the supplied offset
without advancing it. Tick values drive only restrained star brightness and
optional title accent cycling.

The header and text remain small and calm around two authored wordmarks:
Gyrus uses beveled, three-tone chrome vector strokes with small wing accents;
Tempest uses narrow angular vector strokes against a perspective cage. Both
are independently authored geometry, rather than enlarged menu-font labels or
copied game artwork. Cards include the descriptor title/year and selected high
score. The effects control is drawn only for `ENGINE_CAP_EFFECTS`.

The active touch rectangles, shared with the interaction contract, are:

| Action | Half-open rectangle |
|---|---|
| Previous game | [12,60) × [76,140) |
| Next game | [180,228) × [76,140) |
| Effects (capable game only) | [66,174) × [112,138) |
| Play | [60,180) × [145,177) |
| Settings | [60,180) × [184,208) |
| Tilt recenter | [68,172) × [216,240) |

The effects button moves with its card during the slide. Interaction should
settle/consume selection gestures before accepting a new card action. Play and
Settings stay in place. The bottom ship marker clamps preview to ±1000 and maps
it to ±36 pixels when calibrated tilt mode is active, matching the old title's
preview travel. Marker geometry remains inside the visible round aperture at
both extremes. Opening Settings draws the common menu over the star backdrop
without leaving launcher arrows or cards visible at its sides.

## Verification and captures

`tests/test_engine_render.c` checks render purity/repeatability, framebuffer
canaries, valid indices, round masking, negative/off-screen primitive clipping,
full-width decimal conversion, shared font/palette compatibility and polar
clamps. It checks selected/previous/next title callback positions at rest, early,
middle and late slide offsets in both directions; invalid selection and empty
catalogue handling; visible neighbour rails; title ink coverage; and Settings
purity. Title coverage is a regression guard, not a substitute for visual review.

During extraction, 36 frames were captured from a separately compiled original
renderer before any changes, covering seeded simulated play, titles, pauses,
Settings, all three control modes, calibration and reboot progress. Every one of
the 57,600 bytes in every corresponding post-extraction frame matched. The corpus
is local under `artifacts/multigame/render/gyrus-before/` and `gyrus-after/`.
Regenerating assets also reproduced `src/assets.c` and `font_data.inc` exactly.

Reviewed native-resolution PNGs and 2× nearest-neighbour previews live under
`artifacts/multigame/render/`: `hub-gyrus`, `hub-tempest-classic`,
`hub-tempest-neon`, `hub-slide-early`, `hub-slide-middle`, `hub-settings`,
`hub-tilt-left`, and `hub-tilt-right`. `hub-contact.png` combines them for quick
inspection. These are generated project graphics, not downloaded references.


## Mask and fill performance

The circle mask uses a 240-byte const ROM table of exact row boundaries and
clears only the outside spans with `memset`. It preserves the original inclusive
circle equation, including the asymmetric right edge of a 0–239 framebuffer.
Fill clips each rectangle once, then writes contiguous row spans; native-size
glyph pixels have a direct-store fast path. Full-width fills use one contiguous
write. Clipping avoids signed endpoint overflow, including extreme integer
origins/sizes.

Independent tests compare all 57,600 mask pixels to the original squared-distance
equation, and clipped fills to per-pixel 64-bit rectangle inequalities. A local
optimized native microbenchmark measured mask time from 31.426 to 1.722 µs/frame
and a partially clipped 180×170 fill from 7.896 to 0.512 µs/rectangle. These are
host measurements, not RP2040 timings; the board must measure the actual frame
budget after flashing.
