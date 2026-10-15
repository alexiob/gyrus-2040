# Round Arcade: shared engine and Tempest

Branch: `feat/multigame-tempest`. Work stays in the canonical project folder;
`main` remains the working Gyrus baseline. The user requested a shared engine,
multiple games, a retro horizontal scrolling game menu with graphical titles,
and a hardware-native Tempest implementation. They chose **classic Tempest
rules with optional Tempest 2000-style visual effects**.

## Research and scope

The original Atari TM-190 second-printing manual and contemporary Dave Theurer
memo inform [classic research](../../research/tempest-arcade.md). Jaguar's
original manual and period material inform [2000 research](../../research/tempest-2000.md).
Keep downloaded manuals, flyers, screenshots and reference material local and
ignored. Track provenance manifests. No ROMs, ripped graphics, or music enter
the runtime. Document authored web coordinates and timing rather than claiming
an exact ROM reconstruction.

The first Tempest game keeps original enemy roles, lane topology, shots,
Superzapper and dangerous web dives. Neon color cycling, particles, and tunnel
presentation are cosmetic options. Jump, AI droid, collectible power-up chains,
Jaguar bonus modes and Duel remain research, not classic gameplay features.

## Shared engine boundary

Use portable C11, fixed 60 Hz updates, bounded pools and no per-frame allocation.
Keep hardware clocks at 125 MHz CPU/peripheral, 48 MHz USB and 62.5 MHz display
SPI. Preserve existing sensor rotation, coherent sampling, DMA ownership,
SRAM-only BOOT sampling, stack reservations and the serial transport safeguards.

Neutral `engine` headers own input, geometry, UI views, drawing/font and session
contracts. Retain the existing Gyrus simulation API and compatibility wrappers
so the current tests and deterministic simulation can detect regressions.

An immutable game registry supplies one descriptor per game: identity, title,
palette, init/step/render/view, setting/level mutation, state serialization,
canonical hash and UI synchronization. A session dispatches through those
operations. Firmware, desktop and headless frontends never switch on game IDs.
Only one simulation occupies a tagged union; small per-game high-score/settings
records survive switching. The registry is compile-time, not a dynamic plugin
loader. Adding a game adds its descriptor/state/module to that registry.

One session owns retained debug input, input pulse consumption, physical and
keyboard input merging, controls state, UI telemetry, launcher navigation and
external scheduler freeze. Games own gameplay state and rules. Control mode
and tilt calibration are shared; per-game fire/enemy settings and highs remain
independent. No flash persistence is added.

### Rendering and palettes

Extract exact integer pixel/line/fill/text helpers and bitmap font for reuse.
Share pause/Settings drawing through a small value-only menu view. Gyrus keeps
its authored sprites and scenes. Tempest draws lines and bounded geometric
sprites, with indexed glow passes for the cosmetic mode.

Each rendered frame returns an immutable ROM palette pointer. Store that
pointer with its indexed frame slot before publishing the ready queue; core 1
must never consult the currently selected game while presenting an older frame.
Only RGB565 strips in SRAM are DMA sources. Keep the existing two indexed frames
and two strip buffers (130,560 bytes). Rendering remains pure; cosmetic animation
comes from simulation/session tick values, not wall-clock reads or mutations.

## Launcher and common interaction

Boot into **ROUND ARCADE**, showing one centered title card and neighboring
card hints. Use an authored graphical Gyrus wordmark and a sharp vector Tempest
wordmark; do not use plain text as the only game identity. Subtle stars/scanline
accents, restrained cyan/amber and a short eased slide provide a retro look.

Swipe horizontally or tap left/right arrows to change games. Touch PLAY or
press BOOT to start the selection. Touch SETTINGS for the common menu. The
Tempest card offers CLASSIC / NEON style selection. Keep these targets large
and inside the round aperture. A moving bottom ship marker previews tilt and
remains a tap-to-recenter target. Consume held touch/BOOT across selections and
menu transitions, including transitions between equal screen types.

Gameplay keeps split-hold, relative rim swipe and tilt steering, plus default
autofire. Center tap pauses. On Tempest, BOOT is a one-shot Superzapper action;
a visible touch ZAP target below the well provides the same action without
holding a physical button. Repeated held contact must not drain both charges.
Physical Quit in pause returns to the launcher. A visible back target on a
per-game title/game-over screen also returns to the launcher. Reboot remains a
continuous five-second hold. Physical RESET remains a chip reset.

Settings keeps the established fire rate, enemy speed and tilt sensitivity
ranges and defaults. Controls and tilt calibration remain available before
starting. Default Tempest visuals are classic; changing effects changes no
simulation hash, RNG, enemies, score, collision or timing.

## Tempest simulation

- Represent each web by ordered rim/far-rail vertices and explicit closed/open
  adjacency. Never wrap an open end or assume all webs have the same lane count.
- Provide sixteen recognizable authored web shapes, cycling with level
  progression and documented limits. Exact coordinates and late-level timing
  are adaptations where original references do not establish them.
- The claw moves along the near rim and shoots toward the far end. Cap active
  player shots at eight. Returning to the rim after death clears immediate
  threats and provides a bounded respawn grace period.
- Include Flippers (climb, flip, run around the rim), Tankers (release carried
  enemies), Spikers (grow persistent spikes then retreat), Fuseballs (travel
  on rails and are vulnerable during lane crossings), and Pulsars (periodic
  lethal lane pulses). Use the documented introduction stages and score values.
- A first Superzapper clears eligible on-web enemies; a second destroys one
  eligible target; further uses do nothing. It leaves enemy shots and spikes
  intact, and refills on the next web. Enemies not yet on the web are excluded.
- End combat when qualifying web threats clear, including the documented
  rim-Flipper completion case. Animate a dive; surviving spikes can still kill
  the claw while it travels. Advance exactly once, even after pausing or death.
- Maintain score, lives, game over, extra lives, staged difficulty, deterministic
  seeds and level selection for debugging. Document any unverified timing or
  late-level adaptation rather than silently substituting Jaguar rules.

## Debug and compatibility

Keep a single bounded G1 parser and the existing USB/TCP/stdin transports.
Add game selection, catalogue/session inspection and a return-to-launcher
command; add a pulsed `special` input. Use the same selection API as the UI.
State and screenshot operations work for either game and the launcher.

Preserve Gyrus core hashes and standalone adapter behavior, including RESET,
settings, pause, stage selection and legacy `quit` returning to its game title.
A separate engine home action handles physical return to the launcher. New
engine-selected Gyrus replays must match the pre-refactor executable; the new
launcher intentionally has new pixels. Keep current output filenames initially
so flashing/build scripts continue working; the firmware now contains both games.

## Verification and acceptance

1. All current 21 contracts continue to pass with ASan/UBSan.
2. Compare selected Gyrus against the saved pre-refactor headless executable:
   state hashes, JSON and full frame bytes across representative seeds, levels,
   settings, transitions and menus. Do not merely compare two new builds.
3. Test catalogue selection, invalid IDs, high-score/settings separation,
   held-input quarantine, carousel swipes, return home and recentering.
4. Test Tempest topology, shot cap, enemy roles, scoring, Zapper immunity/charges,
   pulses, spike dive, pause/resume, deterministic replay and cosmetic purity.
5. Guard renderer buffers/palette indices/circular clipping and test frame
   palette ownership through a game switch.
6. Build native, desktop and RP2040 targets. Inspect linker/stack use, then test
   the connected board at the established clocks; compare native/device captures
   and measure render/presentation and simulation time. Preserve USB recovery.
7. Deliver research, engine/game documentation, graphical launcher/title
   captures and a branch commit history. Keep the main-branch README demo intact.

## Transition and action effects

Tempest also needs an explicit animated web-entry sequence, web-clear/dive/exit
effects and action feedback during combat. Entry timing is an authored fixed
phase before enemy spawning. Cosmetic events use bounded storage, deterministic
render-only variation and no gameplay RNG. Shot, hit, enemy arrival, Pulsar and
Superzapper feedback must be visible in the vector style, with richer bounded
trails/glow/sparks in neon. Pause freezes their visual lifetime. Existing game
rules, topology, scoring and optional-style equivalence remain required.
