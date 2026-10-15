# Gyruss transition evidence

Research date: 2026-10-15. Target: Konami's 1983 arcade game. These observations
support an original-art adaptation, not an exact port or measured timing model.

## Sources and limits

- [Contemporary service manual](../../reference/game/konami-service-manual.pdf),
  PDF pages 6–8 / printed pages 3–5, inspected as rendered scans. It establishes
  four entering formations, an attack by their survivors, escaping final
  enemies, nonlethal chance stages, 100 points per chance enemy, formation
  bonuses, and a possible 10,000-point four-formation clear award depending on
  operator settings. It does not specify transition choreography or duration.
- [Arcade Game: Gyruss (1983 Konami)](https://www.youtube.com/watch?v=CrrMQBLqqVo),
  Old Classic Retro Gaming. The rendered game is direct visual evidence, but
  the uploader is not Konami and the recording's emulator/ROM/operator setup
  was not independently established. Selected frames were inspected through
  the browser, using playback seeking and frame advance. No video or ROM was
  downloaded. Times below are approximate recording timestamps, not measured
  original frame counts. Ignore YouTube's automatically generated chapters as
  evidence of game state.
- [Existing route research](game-reference.md) retains the manual/museum route
  evidence and explicitly labels the stage table as derived.

## Observed chronology

### Ordinary combat clear

[Recording around 1:27–1:31](https://www.youtube.com/watch?v=CrrMQBLqqVo&t=87s):

1. At approximately 1:27, stage 1 still contains the final enemies.
2. Around 1:28, the playfield is clear and the HUD already says stage 2.
3. Around 1:29, the ship has become much smaller and moved inward from the
   right side toward the center. Stars remain visible behind it.
4. Around 1:31, yellow text announces `1 WARP TO NEPTUNE` in the lower middle;
   orange reappearance particles are visible at the bottom ship position.

An inward, shrinking ship and the next remaining-warp announcement are
supported. A distinct “LEVEL COMPLETE” message was not observed in these
frames. Radial star streaks, easing curves and a quiet clear beat are suitable
adaptations, but these samples do not establish their exact original behavior.

### Final warp / Neptune arrival

[Recording around 2:08–2:16](https://www.youtube.com/watch?v=CrrMQBLqqVo&t=128s):

1. Stage 2 enemies are still present around 2:05.
2. Around 2:10, a blue planet is visible near the center, with the shrunken
   player ship below it. The HUD has already advanced to stage 3.
3. Around 2:11, the ship is tiny and just below the planet, supporting a flight
   inward toward the destination.
4. Around 2:13, the planet has disappeared, the ship is back at the bottom,
   and centered yellow `CHANCE STAGE` text announces the bonus round.
5. By approximately 2:16–2:21, chance-stage formations are entering.

The planet approach preceding the chance round is supported. These sampled
frames do not establish an original planet-name card, exact planet growth,
a route-map screen, or a different Earth arrival choreography.

### Chance result and departure

[Recording around 2:41–2:51](https://www.youtube.com/watch?v=CrrMQBLqqVo&t=161s):

1. The centered yellow result title is `BONUS`, with a multiplier line showing
   `100 X 35` and `3500 PTS`; player ship remains at the bottom.
2. Score at this sampled result frame is 22,150.
3. Around 2:46, the HUD has advanced to stage 4, the score is 25,350, and the
   ship is again small and moving inward toward the center.
4. By around 2:51, the next combat formations are entering.

Another nonperfect result at [6:51](https://www.youtube.com/watch?v=CrrMQBLqqVo&t=411s)
shows the same multiplier presentation for 17 hits / 1,700 points.
These observations support result hold → inward departure → next combat.
They do not establish a route-map interlude. A perfect result was not observed;
the manual supports the potential 10,000 award, but exact perfect-screen text,
colors, animation and award timing remain unverified.

## Recommended 240 × 240 adaptation

These durations and design choices are authored, not claims about original
arcade timings. Keep gameplay simulation frozen through these interludes;
advance each state exactly once and retain pause/resume behavior.

| Phase | Proposed duration | Presentation |
| --- | --- | --- |
| Combat clear | 0.5 s / 30 ticks | Preserve cleared ship angle; quiet field without an added banner |
| Warp | 1.5 s / 90 ticks | Shrink ship and pull radius inward; restrained radial star acceleration |
| Next combat intro | Existing intro | Updated remaining-warp count and destination; return ship to bottom |
| Planet arrival after final warp | 2 s / 120 ticks | Centered original planet art; tiny ship approaches from below; modest planet growth |
| Chance intro | Existing intro | Planet clears; bottom ship; CHANCE STAGE |
| Chance result | About 2 s | Hit count and readable bonus; perfect treatment only when earned; then warp before next combat |

Thus ordinary clear plus warp takes about 2 seconds; a planet clear plus warp
and arrival takes about 4 seconds before the chance intro. Fit title, destination
and hit-count text inside the circular safe area. A “hits / 40” result is a
readability adaptation of the observed multiplier display. Preserve original
route order; a stylized route row is optional authored UI, not established by
this inspection.

## Scoring caveat

The sampled first result visibly increases the score between the result and
next-stage frames, consistent with an end-of-round award. This sample alone
does not prove whether the total is additional to all per-kill points, nor the
exact cadence of the tally. Do not silently change scoring from these sparse
frames; inspect the score immediately before and after individual chance kills
and the entire result tally before claiming an exact accounting rule.
