# Tempest classic mode

Tempest is a native C11 game module using the shared engine, not a ROM emulator.
It implements classic lane shooting with newly authored vector glyphs, web
coordinates, spawn schedules and sound-free presentation. Original arcade
research and revision caveats are recorded in [the reference
note](research/tempest-arcade.md).

## Playing

Move the bright claw around the near rim with split touch, relative drag or
calibrated tilt. Autofire defaults to five shots per second. BOOT or the visible
**ZAP** rectangle (x90-149, y176-201) triggers one Superzapper use. Touch the
center of the well to pause. Shared Settings retains fire rate 1-10, enemy speed
25-200% and tilt sensitivity 50-200%. Pause provides Resume, Settings and Quit;
the engine owns returning to the launcher and physical input quarantine.

The first Zapper use clears all enemies that have landed on the web. The second
kills one eligible enemy, selected deterministically from the game RNG. Neither
removes spikes, enemy shots or waiting enemy dots. A Zapper-destroyed tanker
does not release its payload. Physical controls debounce a held Zapper contact
into one pulse; the game consumes every true engine pulse, including adjacent G1
requests. Charges refill on the next web and remain spent after a death.

Starting a run and arriving at each new web plays a one-second entry: the well
assembles toward the camera while the claw flies from the far end to the rim. No
enemies spawn or shots fire until entry completes. Debug `LEVEL` deliberately
enters combat immediately.

Destroy or outlast each web's climbing enemies. Rim Flippers can remain when the
web clears; other live enemies and unlanded dots prevent completion. After a
short flashing clear interval the claw dives toward the far rim with vector
travel streaks: **remaining spikes are still lethal**. Steer into a clear lane
or shoot to shorten a spike. A successful dive awards 1,000 authored transition
points, then the old web recedes as the next web approaches during a recharge
flight, followed by its entry sequence. A dive death consumes one life and
proceeds to the next web after the death/flight sequence. Pause freezes the
exact dive state. A normal death clears immediate rim threats and enemy shots,
retains deep enemies/spikes and grants two seconds of respawn grace. Three lives
are supplied initially; each 20,000 points awards one additional life up to
five.

Enemy families retain these classic roles:

| Enemy | Begins | Score | Behavior |
| --- | --- | --- | --- |
| Flipper | 1 | 150 | Climbs, flips lanes and pursues the claw around the rim. |
| Tanker | 3 | 100 | Splits into two carried enemies when shot or near the rim. |
| Spiker | 4 | 50 | Grows a persistent spike, then retreats. |
| Fuseball | 11 | 250/500/750 | Travels on rails; vulnerable during crossings. |
| Pulsar | 20 | 200 | Periodically electrifies its entire lane. |

Player shots intercept enemy shots for zero points and shorten spikes for two
points per hit. Enemy collision, an enemy shot reaching the claw's lane, a
Pulsar pulse in its lane or a dive spike collision costs a life. Fuseball point
thresholds, constant spike damage/score, the transition award, spawning,
velocities, grace period and pulse timing are authored choices. They are not
claimed as arcade-exact. Pulsar level 20 follows the inspected Atari manuals;
secondary accounts sometimes say 17. Tankers carry Flippers initially, Fuseballs
from 33 and Pulsars from 41; the payload schedule is an authored approximation.
Superzapper activation is immediate in this adaptation; the design memo's
original activation delay has not been reconstructed.

## Webs and graphics

Sixteen recognizable authored webs repeat: Circle, Square, Plus, Bow Tie, Cross,
Triangle, Clover, V, Steps, U, Flat, Heart, Star, W, Fan and Infinity. V, Steps,
U, Flat, W and Fan are open chains. Player movement clamps at their ends; enemy
flips reverse there. Closed webs wrap. Crossings on the screen never create an
extra lane junction. Rail/lane counts vary explicitly: Plus 12, Triangle 15,
most closed webs 16, open webs 17 rails / 16 lanes. This is authored topology
rather than verified original vertex/count data.

Classic web colors change at 1/17/33/49/65/81 to blue/red/yellow/cyan/invisible/
green. The selected lane remains highlighted on invisible webs. After 81 the
green palette and bounded difficulty continue. The native level API accepts
1-4095; 4095 advances to 1. The shared G1 `LEVEL` command retains its 1-999
bound. This deliberate late-level adaptation makes no claim about the original
game's level 99 randomization.

Graphics are indexed pixels with `engine_palette`, circular clipping and shared
font/menu/title helpers. The renderer uses integer perspective and authored line
glyphs. Eight short-lived vector events provide visible muzzle flashes, hit
bursts, tanker-split fragments, landing glints, Pulsar feedback, Zapper flashes
and death feedback. A Zapper activation also flashes the web and sends a bright
ring through it. All visual events are emitted in both styles without using the
gameplay RNG. Pause freezes event ages and transition motion.

The optional NEON style adds bounded glow strokes, projectile tails and rotating
spark shards, derived from tick/entity/event coordinates. It changes no RNG
calls, gameplay data, collision, scoring or canonical hash. Rendering does not
mutate the state.

## Module boundary and deterministic state

- `include/tempest/game.h` exposes the complete bounded `tempest_state` for the
  engine's active-game union, plus web/game APIs.
- `src/games/tempest/webs.c` contains immutable web tables, adjacency and integer
  projection; `game.c` owns rules, time, RNG and field-wise hashing.
- `render.c` writes a caller-owned 57,600-byte framebuffer.
- `adapter.c` exports `engine_tempest_game`, including every `engine_game_ops`
  callback, immutable palette, shared UI telemetry, settings and JSON state.

Simulation runs at 60 Hz. Depth is Q16 (0 far, 65536 near); claw/lane progress
is Q8. No dynamic allocation or floating point is used. Pools are fixed at 32
enemies, 8 player shots, 12 enemy shots and 17 spike lengths. Tanker payloads
share the enemy pool and are bounded if it is saturated. Presentation has a
fixed eight-event pool and a small Zapper flash counter, updated identically in
both styles. Expired slots are replaced round-robin. Neon enemy sparks also use
a bounded three-spark draw loop. These fields are explicitly excluded from the
semantic hash; no second framebuffer is needed. Perspective computes
`scale=34+222*d*d/65536` with `d` in 0-256, keeping far-rim enemies visible and
avoiding floating-point division.

The canonical hash includes semantic gameplay fields, active entities and
settings; it excludes padding, inactive slots, cosmetic style, sensor/UI
telemetry and all visual event fields. Debug setters accept `fire_rate`,
`enemy_speed`, `tilt_sensitivity`, `autofire`, `invincible`, `effects`, `score`,
`high_score` and `lives`. Invalid values do not mutate state. `level` validates
1-4095 before resetting a web. Scores/highs accept 0-99,999,999 and lives 0-99
for debugger parity; ordinary bonus play remains capped at five. Score injection
moves the next bonus threshold above the injected score rather than
manufacturing backdated bonus lives.

## Verification

The four native test executables cover topology and replay, enemy roles,
renderer boundaries/purity and adapter semantics. Initial runs failed on missing
module symbols, missing high-score restoration and enemy-shot interception;
these cases subsequently passed. Tests use real simulation state with explicit
hazard setups, not mock game rules.

Standalone commands (from the repository root):

```sh
cc -std=c11 -Wall -Wextra -Werror -Wconversion -Wshadow -Wstrict-prototypes \
  -fsanitize=address,undefined -Iinclude tests/test_tempest_roles.c \
  src/games/tempest/game.c src/games/tempest/webs.c src/engine/settings.c \
  -o /tmp/test-tempest-roles
/tmp/test-tempest-roles
cmake --build --preset host
ctest --preset host --output-on-failure
```

The renderer contract exercises all nine phases over 96 stages and checks guard
bytes, palette indices, circular masking and immutable hashes. Adapter tests
cover high-score restoration, same-value fire setting stability, validation,
JSON truncation and shared pause/Settings control screens. Cosmetic replay runs
900 ticks with matching hashes. Roles tests cover tanker shot/rim splits,
Fuseball immunity/crossing vulnerability, pulse/rim deaths, spike persistence,
20k bonus life, rim-only completion, bullet interception, pause/dive freeze and
introduction-stage schedules. Device timing, linker RAM/stack checks and actual
touch/tilt feel are integration checks performed by the platform task.

Entry, exit and feedback regression tests assert the 60-tick entry boundary,
no combat during entry, exact pause/resume position, natural flight-to-next-entry
progression and immediate debug level selection. They verify that entry/flight
frames visibly change, actual Zapper/hit events change pixels, visual metadata
does not change the canonical hash, and pause freezes the complete event pool.
`STATE` includes compact `phase_ticks`, `dive_depth`, `fx` (live event count) and
`zap_flash` fields for deterministic captures.

A reproducible transition run is seed 1234, invincible 1, autofire 0, no steering:
after starting, game ticks 30/1086/1145/1213/1273 show entry/clear/dive/flight/next
entry. Combat starts at 60, clears at 1080, dives at 1110, flies at 1183, enters web 2
at 1243 and resumes combat at 1303. These are authored schedule observations.
