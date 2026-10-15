# Original Tempest reference and native adaptation

Research date: **2026-10-16**. Target: original Atari arcade Tempest (1981),
native C on RP2040, 240 x 240 circular LCD. Optional Tempest 2000-inspired
presentation must not change classic rules. This is a documented adaptation,
not an emulation or a claim of ROM-exact reproduction.

## Evidence and provenance

Primary sources downloaded in ignored `reference/tempest/arcade/`:

- [Atari TM-190 second-printing operator/service manual](https://gauck.com/arcade/Tempest/Tempest.pdf):
  local `atari-tempest-tm190-manual.pdf`, 60 PDF pages. Cover identifies the
  printing. Visually inspected PDF 18 (printed 10, settings), PDF 22-24
  (printed 14-16, gameplay/enemy table). Corresponding `manual-page-*.png`
  renders preserve these pages. Separate archival first/second-printing scans
  are retained; their pagination differs, and no complete revision comparison
  was performed.
- [Dave Theurer, Atari inter-office memo, 27 August 1981](https://arcarc.xmission.com/Web%20Archives/Andys%20Arcade%20%28Sep-09-2003%29/arc/atari/tempest/tempest_game_specifications.pdf):
  `atari-tempest-game-specifications.pdf`; all eight surviving scanned pages
  inspected as `spec-page-1.png` through `spec-page-8.png`. Printed pages occur
  in order 1, 15, 14, 13, 12, 11, 3, 2. **Printed 4-10 are missing**, explicitly noted
  by the archive. The separately retained higher-resolution `fullscan` is also
  eight pages, not a complete specification. Treat this as pre-release design
  evidence, not proof of every shipped revision.
- [Atari Coin Connection, October 1981 page 2](https://elibrary.arcade-museum.com/magazines/acc/1981-October-5-06/Atari-Coin-Connection-5-06-1981-10-002.pdf)
  supports contemporary Skill-Step, three difficulty settings and vector-display
  positioning. Two original Atari flyer scans preserve cabinet/art direction;
  the museum catalogs the coupon flyer as 1982 although the artwork says 1981.

All byte counts, SHA256 hashes, source URLs, derivative-page origins and usage
restrictions are in [arcade-manifest.json](../../reference/tempest/arcade-manifest.json).
Public availability is not a reuse license: scans/art are **reference only**.
No ROM, extracted game assets, executable game data, music or original audio was
obtained. Runtime geometry, glyphs, effects and sounds should be newly authored.

## Confirmed arcade rules

TM-190 printed 14-16 establishes 99 skill levels on 16 webs; cycle colors start
1 blue, 17 red, 33 yellow, 49 cyan, 65 invisible, 81 green. Spinner moves a claw along the
near rim; fire travels inward. Eight player shots maximum; slots return after
hits, far-rim exit or limited spike penetration. Superzapper: first use clears
landed enemies, second kills one random enemy; neither clears shots, spikes or
unlanded enemies. Recharge occurs on reaching the next web.

| Enemy | First level / points | Essential behavior from manual |
| --- | --- | --- |
| Flipper |1 /150|Climbs; flips across lanes/on rim; contact or shots kill; vulnerable during flip.|
| Tanker |3 /100|Carries 1-2 enemies; releases on being shot or nearing rim.|
| Spiker / spike |4 /50;1-3|Builds persistent spike; shots erode it.|
| Fuseball |11 /250,500,750|Rides rails and jumps toward player; shootable between rails.|
| Pulsar |20 /200|Moves vertically/flips; pulse kills player occupying its lane.|

Manual settings (printed 10): 2-5 starting lives; suggested 3, medium difficulty,
bonus each 20,000. Bonus alternatives 10,000-70,000 or none. Skill-Step selects
odd-numbered eligible starts; previous progress is retained briefly. Exact
spawn clocks, speeds, pulse periods and respawn rules are **not established**.

Theurer memo printed 12-14 adds useful implementation constraints: flipper flips
have constant angular rate, so sharp corners afford less reaction time;
Superzapper has a delay, so last-instant use is unsafe. Difficulty proposal:
easy has 1/8 fewer enemies, 7/8 speed, one fewer enemy shot and half-speed rim flips
on levels 1-16; hard has 1/8 more enemies, 9/8 speed and one more enemy shot (cap 4).
It places shooting pulsars at 20 on hard versus 60 otherwise. These are memo
values, not independently measured shipped-ROM parameters. Enemy shots score 0.

## Topology, sequence and remaining uncertainties

The primary manual says 16 **rails**, not 16 lanes. Do not silently equate them:
a closed polygon with 16 boundaries has 16 lanes, while an open chain of 16 rails
has 15. Exact per-web counts and vertex coordinates remain unverified. Store
`rail_count`, `lane_count`, `closed` and explicit neighbor relationships per
web. A crossing on screen is not necessarily a gameplay junction.

The [Games Database catalog](https://www.gamesdatabase.org/game/arcade/tempest.aspx)
provides this conventional first-cycle sequence, **secondary evidence**:

|Level|Shape|Provisional topology for authored approximation|
|---|---|---|
|1|Circle|Closed|
|2|Square|Closed|
|3|Plus|Closed|
|4|Bow-tie|Closed; preserve ordered perimeter|
|5|Stylized cross|Closed|
|6|Triangle|Closed|
|7|Clover|Closed|
|8|V|Open|
|9|Steps|Open|
|10|U|Open|
|11|Flat line|Open|
|12|Heart|Closed|
|13|Star|Closed|
|14|W|Open|
|15|Fan|Open; particularly needs visual validation|
|16|Infinity / sideways 8|Closed; crossing is not a junction|

Use these names/order as a reconstruction guide, not copied geometry. Repeating
`(level-1)%16` is a reasonable authored first version. Manual confirms color
cycles, but does not prove randomization/behavior after 99. That remains outside
this implementation's arcade-fidelity claims.

The [Museum of the Game catalog](https://www.arcade-museum.com/Videogame/tempest)
corroborates open shapes with endpoints and describes level clear when enemies
are gone or only rim flippers remain. During the subsequent dive, avoid or shoot
remaining spikes: contact is fatal. Preserve this hazard phase; a generic safe
warp loses a central Tempest mechanic. Museum terminology calls rim enemies
“red shooters”; color varies by cycle, so identify by enemy/state, not color.

First-printing printed 17 lists spikes as 2 points and suggests Superzap,
contradicting that page's exclusion paragraph. Second printing changes the row
to 1-3 points and omits Superzap. Prefer the second-printing exclusion rule.
Both inspected manuals place pulsar debut at 20.

Pulsar debut is a known discrepancy: manual table says 20; common secondary
accounts say 17. Keep 20 as the documented baseline and expose the schedule as
data for later correction. The manual does not explain the fuseball 250/500/750
position thresholds or spike 1-3 scoring formula; those values require validation.

## Recording references and limits

- [Centre for Computing History cabinet footage](https://www.youtube.com/watch?v=jaFxHIV4Vyk),
  published 2016-03-13. Uploader identifies filming at its Cambridge museum.
  This is the strongest available physical-cabinet lead, but board revision,
  operator settings and unmodified-ROM status are unknown. Inspected the cabinet exterior at 00:20 and a blue square-like web at 01:00
  in the browser. This supports physical-cabinet/vector presentation only;
  **no measured timings or complete shape-sequence claims** derive from it.
- [Sharky's Sharkade, Tempest(1981) Arcade](https://www.youtube.com/watch?v=jfaCrdBABUY),
  published 2017-09-02: useful unverified lead; emulator/cabinet and revision
  unknown. Not watched frame by frame or downloaded.

Manual figures and original flyer art were visually inspected. Videos have not
been downloaded. Future validation should capture levels 1-16, open-web endpoint
movement, flipper rim clear/dive, spike dive collision, two Superzapper uses and
pulsar arrival before tightening fidelity claims.

## Authored 240-round implementation requirements

1. **Topology first.** All movement, flipping, pulse targeting and shooting uses
   lane/rail IDs, not nearest rendered pixels. Closed webs wrap; open ends clamp
   player movement and reflect enemy seeking as an explicitly authored rule.
   Render inner/outer ordered polylines with corresponding rails. Use authored
   normalized integer coordinates; preserve sharp bends and lane readability.
2. **Fixed time and bounded memory.** Use a 60 Hz simulation accumulator, bounded
   entity arrays and deterministic seeded spawns. Store depth and lane progress
   in Q16.16 or Q8.8 integers; multiply with 32-bit intermediates after range
   analysis. A LUT maps depth to perspective scale, avoiding per-entity float
   division/trigonometry. Rendering interpolates points between precomputed
   inner/outer rail vertices. Screen coordinates never determine collision.
3. **Explicit behavior states.** Flipper climbs/flips/rim-seeks; tanker splits
   into independently allocated carried entities; spiker leaves lane spike
   length; fuseball distinguishes rail-safe and between-rail vulnerable states;
   pulsar separates warning/pulse/recovery. Track enemy projectiles separately.
   Rim attacks remain lethal. Keep combat, clear, spike-dive, inter-web flight,
   death/respawn and game-over separate. Pending spawns prevent premature clear;
   rim flippers may be left behind only under the documented clear exception.
4. **Classic resource semantics.** Shot pool 8; Superzapper charges progress
   `2 ->1 ->0` with clear-all then random-one semantics and explicit exclusions.
   Do not reset on death without evidence. Scoring, enemy introduction and
   difficulty belong in independent constant tables, with authored timings
   labeled in comments. Default 3 lives and 20k bonus are defensible presets.
5. **Readable vector display.** Center near(120,124), fit web+claw within roughly
   radius 98, and reserve compact score/lives/zap indicators inside the round
   safe area. One-pixel dim-blue rails,2 pixel bright player/enemy silhouettes,
   black background and selected-lane highlights preserve hierarchy. Keep
   minimum visible enemy size 3-5 pixels near the far rim. Throttle optional
   trails/sparks/shake independently of simulation; classic hazards must remain
   legible. Invisible cycles need an accessibility option, labeled adaptation.
6. **Touch is an adaptation.** A relative horizontal drag, with fractional-lane
   accumulator and capped velocity, emulates spinner intent across both open
   and closed webs. Polar absolute touch mapping is unsuitable for open/concave
   webs and finger occlusion. Autofire while active replaces holding FIRE;
   offer manual fire if controls permit. Reserve a clearly marked center tap
   for Superzapper, trigger on press edge, and suppress repeats until release.
   Gesture thresholds/cadence require device testing, not arcade claims.

Acceptance scenarios: closed wrap/open clamp; tanker split at rim and on hit;
fuseball immunity on rails; pulse kills only its current lane; spike erodes and
survives combat into a lethal dive; rim flipper contact kills; first/second zap
respect exclusions; shot count never exceeds 8; death consumes exactly one life;
all 16 webs remain readable within the circular mask. Exact arcade timing remains
an explicit follow-up, not a prerequisite for a complete authored classic mode.
