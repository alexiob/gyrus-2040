# Game reference and level design

Confirmed by the user: **Gyruss, Konami's 1983 arcade game**, rather than a
console adaptation. The round display is well suited to its orbital playfield.

## Evidence to preserve

The [contemporary service manual](../../reference/game/konami-service-manual.pdf),
PDF pp.6-8 (printed pp.3-5), establishes an orbital ship controlled with an
eight-way joystick, inward shooting, four entering enemy formations followed
by attacks from surviving enemies, and chance stages where the player cannot
be destroyed. The center of a three-satellite group awards double fire.
Meteors are indestructible; paired beam satellites are hazards.

Its scoring examples include satellites worth 500/1,000/1,500 points before
double fire, then 1,000/1,500/2,000; beam satellites worth 200; and 100 per
chance-stage enemy. Chance stages occur at phases 3, 7, 11, 15, 19 and onward.
The manual lists formation bonuses and a possible 10,000-point perfect-clear
award. Treat operator settings and regional revisions explicitly when refining
scoring; do not combine incompatible default tables.

The [museum catalog](https://www.arcade-museum.com/Videogame/gyruss) describes
the route as Neptune, Uranus, Saturn, Jupiter, Mars, Earth: two combat warps
before the first Neptune arrival, three before each later planet, with a
challenge after arrival and a faster return leg after Earth. It also identifies
the original as a vertical raster game with a single fire button and stereo
sound. Its separate claim of reaching Earth on level 25 conflicts with a simple
count from that route and with the manual's chance-stage sequence. Keep that
discrepancy visible rather than hard-coding 25 as an established fact.

The [MAME Gyruss driver](../../reference/software/mame-gyruss.cpp) documents
arcade hardware, inputs, video layouts and operator settings. It defines 8 x 16
hardware sprites (16 x 8 after rotation) and 8 x 8 characters, with a visible raster region that becomes
portrait after rotation. It does **not** contain the game's ROM-resident
formation paths or level scripts. No exact path reconstruction has been done.

## Proposed stage model

Use an explicit planet/warp/chance state machine. The following is a **derived
design table**, counting only combat and chance stages and excluding title,
arrival and warp animations:

| Destination | Combat stages | Chance stage | Countdown on combat stages |
| --- | --- | --- | --- |
| Neptune | 1-2 | 3 | 2, 1 |
| Uranus | 4-6 | 7 | 3, 2, 1 |
| Saturn | 8-10 | 11 | 3, 2, 1 |
| Jupiter | 12-14 | 15 | 3, 2, 1 |
| Mars | 16-18 | 19 | 3, 2, 1 |
| Earth | 20-22 | 23 | 3, 2, 1 |
| Return to Neptune | 24-26 | 27 | 3, 2, 1; increased difficulty |

This preserves the manual's chance-stage progression and the stated journey.
Confirm any original on-screen level numbering against recorded gameplay
before calling it arcade-exact. Use destination and remaining warps as the
authoritative gameplay state; stage numbers are presentation metadata.

Proposed implementation phases:

1. A playable Neptune loop: ship, inward shots, spiral entrants, central
   formation, outward attacks, death/respawn, score and chance stage.
2. A full route using table-driven, authored paths and per-planet tuning.
3. Satellite power-up, indestructible meteors, paired beam hazards and return
   difficulty; refine scoring and formation sequencing against evidence.

Paths should be compact C constant tables of duration and polar/radial control
points, with shared templates plus mirror/rotation transforms. Each enemy runs
an explicit ENTER -> HOLD -> ATTACK -> RETURN/EXIT state machine. Keep gameplay
parameters separate from sprite frames so trajectories can be tuned without
changing drawing code. Exact enemy counts, velocities, firing cadence and
path control points are tuning work, not facts established by this research.

## Graphics references and native adaptation

Downloaded: title/gameplay screenshots, an additional gameplay example, and
front/back manufacturer flyer scans in `reference/game/`. These are visual
references, not a complete asset pack. Use the service-manual cover for another
contemporary illustration. See the [reference index](../../reference/README.md).

For a self-contained first build, author small indexed sprites, a bitmap font,
explosion frames and starfield data in the project. Preserve the arcade's visual
roles: distinct inward-facing player ship, readable enemy silhouettes, bright
bullets against black, recognizable satellite/meteor hazards. Keep the orbit
within the visible circular mask, with HUD text inside the circle's safe area.
Use discrete precomputed ship orientations instead of runtime arbitrary rotation.

The original portrait HUD and rectangular corners do not fit this panel as a
literal pixel-for-pixel port. Adapt layout to 240 x 240 while retaining orbital
gameplay. Initially test a radius near 100 pixels and a small readable HUD;
the final radius depends on sprite extents and the actual visible edge.

Videos are cataloged in the reference index but have not been downloaded,
watched frame by frame, or used to claim measured timings. No ROMs, extracted
sprite sheets or original sound recordings have been downloaded. Exact
artwork/music reproduction is outside the current evidence pack; the initial
runtime can be fully self-contained with newly authored assets.

## Wave implementation update · 16 October 2026

The [wave evidence study](gyruss-waves.md) refines the lifecycle above. The
implementation now retains offscreen returning fighters, gates coordinated
attacks on entrance assembly, uses stable formation identities and six authored
path families, and limits terminal escapes to the final two. A once-only fifth
group and entrance bonuses are implemented with the documented source limits.
The old global enemy-age expiry has been removed. Exact original control points
and the fifth-group trigger remain unverified; current tuning is labeled in
the [gameplay guide](../gameplay.md).
