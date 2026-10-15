# Gyruss wave lifecycle and entrance evidence

Research date: 2026-10-16. Target: Konami's original 1983 arcade game.
This records inspected evidence and implementation guidance; it is not a
reconstruction of ROM paths or arcade frame timings. Research stopped at the
user's request. No ROMs, extracted game data, or video files were downloaded.

## Manufacturer instructions

The [contemporary service manual](../../reference/game/konami-service-manual.pdf)
was inspected as rendered scans, PDF pp.6–8 / printed pp.3–5.
[Public scan](https://www.arcade-museum.com/manuals-videogames/G/Gyruss__1983__Konami.pdf).
PDF p.7 / printed p.4 establishes:

- Four entrance formations appear. Survivors wait in screen depth near the
  center, then attack after all four formations have appeared.
- Some higher-phase formations originate outside the screen; depth emergence
  remains a distinct entrance mode.
- The final **two** fighters attempt to escape and can be shot for 1,000 each.
  The manual gives no escape delay or attack-pass count.
- Chance stages cannot destroy the player. Their four formations and bonuses
  belong to a separate lifecycle; the chance sequence begins at phase 3 and
  repeats every fourth phase.

Ordinary offscreen movement is therefore insufficient evidence that a combat
fighter has permanently escaped. A stage must retain fighters during return
and re-entry. The endgame escape exception does not justify expiring all
fighters after one common lifetime.

## Contemporary eyewitness strategy

Ben Gold's article in [JoyStik, December 1983](https://arcarc.xmission.com/Magazines%20and%20Books/Joystik%20Magazines%20%2810%20Issues%29/Joystik_Vol2-3_83-Dec.pdf)
was downloaded and visually inspected: printed pp.17–21 / PDF pp.19–23.
[Local scan](../../reference/game/joystik-dec1983.pdf);
[provenance](../../reference/gyruss-wave-manifest.json).
The following compact summary separates enemy motion from suggested player
intercept positions:

| Printed page | Evidence |
| --- | --- |
| 18 | Destination-specific entrance patterns repeat across its warps. A depleted central formation can receive a fifth entrance group, using a pattern from the first five planets. Gray attackers circle independently of the player; red fighters reverse repeatedly below the player; blue reversals are gentler. |
| 19–20 | The fifth group is described as the last. Full entrance-group destruction before settling earns escalating formation bonuses; one survivor reaching holding forfeits that group's award. |
| 20 | Neptune's four convergence positions: bottom, bottom-right, bottom-left, top. Uranus: left, right, top, top starts; paired lines of five loop before settling. |
| 20–21 | Saturn player intercept/follow positions: left counterclockwise; bottom; lower-left, later left clockwise; top-right counterclockwise. These are not a complete enemy-coordinate specification. |
| 21 | Mars fighters peel off individually. Player follows right clockwise, left counterclockwise, right clockwise, top-left counterclockwise. Jupiter groups 1/3 share a top-left vulnerability despite different entrances; 2 favors bottom, 4 bottom-left. Earth guidance alternates ascending lower-corner interception with descending upper-corner interception. |

Exact spline coordinates are absent. Some prose concerns player movement,
which must not be silently relabeled as enemy spawn coordinates.

Two additional contemporary articles were available through search-index
excerpts, but their live PDF URLs returned HTTP 404. Their unrendered excerpts
are useful leads with lower verification confidence:

- [Video Games, January 1984, printed pp.35–36](https://www.ataricompendium.com/archives/magazines/video_games/video_games_jan84.pdf):
  attacking ships can disappear past the perimeter and return to central
  formation. It describes broad arcs, repeated center-to-rim passes and
  erratic reversals. Its Neptune guidance says destroying more than three
  quarters of the fleet brings another group. It describes Mars entrances
  as center-originating, similar to Jupiter.
- [Computer Games, February 1984, printed p.50, Bob Guerra](https://www.ataricompendium.com/archives/magazines/vgp/computer_games_feb84.pdf):
  Saturn uses single-file groups of ten, first from upper-left and upper-right,
  then analogous lower entries. Uranus uses mirrored side loops and upper
  left/right variants. Chance groups can circle more than once before leaving;
  absence from an intercept position is not necessarily the end of their path.
  Its fifth-group wording suggests a different trigger from Video Games.

The existence of a possible fifth combat group is corroborated; the exact
threshold, evaluation moment and scheduling relative to attacks remain
unresolved. A once-only `initial kills > 30` rule after four entrance groups
settle is a documented adaptation of one contemporary account, **not verified
arcade logic**. Do not use the conflicting Computer Games wording to claim
that exact trigger. Chance stages remain four groups of ten, without this
combat reinforcement rule.

## Direct recording observations

[Arcade Game: Gyruss (1983 Konami)](https://www.youtube.com/watch?v=CrrMQBLqqVo),
Old Classic Retro Gaming, inspected through browser playback, seeking and
frame advance. This is direct visual evidence of the rendered game; uploader,
emulator revision, operator settings and original frame cadence were not
independently established. Times are approximate recording timestamps.

| Recording time | Observed |
| --- | --- |
| [0:38](https://www.youtube.com/watch?v=CrrMQBLqqVo&t=38s) | First Neptune blue group appears tiny near the center/depth as a shallow split arc. |
| [0:39](https://www.youtube.com/watch?v=CrrMQBLqqVo&t=39s) | The group expands into two curved branches, making a U around the lower-center region. |
| [0:40](https://www.youtube.com/watch?v=CrrMQBLqqVo&t=40s) | Branches tighten/cross near lower-center. This is visibly different from a uniformly rotating spiral. |
| [0:41–0:43](https://www.youtube.com/watch?v=CrrMQBLqqVo&t=41s) | Blue survivors travel around the right, then become small near center while the next gray group emerges from depth. |
| [0:48](https://www.youtube.com/watch?v=CrrMQBLqqVo&t=48s) | Gray survivors are near center while another blue entrance develops. Verified initial color order is blue, gray, blue; fourth color was not checked. |
| [0:58–1:11](https://www.youtube.com/watch?v=CrrMQBLqqVo&t=58s) | Stage 1 persists with small central survivors and much larger roaming attackers. Gray attackers travel broad arcs across upper/right/left regions. Sparse samples do not establish individual identity through every offscreen return. |

No later-planet path was directly reconstructed in this pass. The prior
[transition study](gyruss-transitions.md) covers the separate clear/warp/arrival
sequence and chance result presentation.

## Implementation guidance and limits

Use stable formation/member identity and explicit per-enemy path state:

`ENTER -> HOLD -> ATTACK -> RETURN -> HOLD`, with separate terminal `DEAD`
and endgame `ESCAPE` states. An offscreen return can be invisible while still
counting as an unresolved living fighter. Distinguish visibility from
resolution. An entrance path finishing should settle its survivor; it should
not expire that survivor or independently launch attacks before entrance
sequencing finishes.

For an authored 240 × 240 adaptation, preserve the visual roles established
above: Neptune depth-origin paired crossing arcs; Uranus edge-origin paired
hooks/loops; Saturn single-file trains; Mars individual peeling and alternating
sweep direction. Later Jupiter/Earth control points require approximation until
more direct observation is requested. Mere rotation of the same spiral for
every planet is not a supported description of the original.

Keep group path selection, mirror/rotation, member delay and kind independent
of the compact live-enemy array index. Destroying an earlier fighter must not
change survivors' group paths or holding slots. Store the initially scheduled
entrance count separately from live bodies, and decide any reinforcement before
allowing an empty original roster to clear combat. Chance stages instead finish
when their scheduled flights and terminal departures have resolved; missed
fighters do not enter an endless combat holding/attack cycle.

Escape timing, exact simultaneous attacker count, hold radius, angular speed,
formation launch intervals, loop durations, all later-planet coordinates and
full four-group color tables remain unmeasured. Any selected constants or
control points must be called authored tuning, not arcade-exact values.
