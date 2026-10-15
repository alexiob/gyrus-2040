# Gyrus gameplay

The portable C11 core updates at 60 ticks per second. Drawing is a pure function
of game state, using an indexed 240 × 240 framebuffer. The device and desktop
use the same simulation, controls, renderer, palette and sprites. No arcade ROM,
extracted sprites or audio are used. This is an original Gyruss-inspired
adaptation; enemy trajectories are authored for this screen and input hardware.

The ship orbits the perimeter and fires inward automatically. Autofire is
enabled by default, so no button hold is required; touch alone can start,
steer, pause and resume. The debug setting can disable autofire for testing. Four ten-enemy formations enter along destination-specific paths, assemble in
depth, and make repeated attack/return flights. Ordinary fighters remain alive
when they leave the round viewport; they return instead of disappearing on a
lifetime timer. Regular combined attacks wait for entrance assembly; Mars has
an authored individual peeling exception. Enemy shots travel outward. Later stages add indestructible meteors and paired laser emitters; a
surviving lone emitter has no connecting beam. Shooting the satellite awards
double fire. Losing a life removes double fire, clears projectiles and gives a
short period of invulnerability. The game ends after the final life.

Autofire defaults to five volleys per second (paired shots after a power-up).
Pause → Settings adjusts fire rate from 1–10 volleys/s, enemy pace 25–200%, and
tilt sensitivity 50–200%. Changes apply when resumed and survive new games,
returning to defaults after reboot. Enemy pace scales waves, attack phases,
flight timing and hostile shots together, while player motion remains at 60 Hz.
Enemy rotation, dives, return paths, meteors and hostile projectiles were slowed
by roughly one-third after physical play testing. Split touch steering is
inverted: left touch turns clockwise; right touch turns counterclockwise.
The ship is drawn above HUD/messages with an ivory edge and dark halo.

Ordinary enemies score 100/150/200 points; the final two fleeing fighters score
1,000 each. Whole incoming groups destroyed before any survivor settles award
1,000 / 1,500 / 2,000 / 2,500 points, and 3,000 for an extra fifth group.
The satellite scores 1,000 (2,000 while already powered), guards 500 and laser emitters 200. An extra life is awarded at
30,000 points and every further 60,000, up to nine lives. Scores saturate at
99,999,999. Scores and control preference persist within a running session;
there are no flash saves.

## Formations and stage completion

Each fighter has a stable formation/member identity. Killing another fighter or
reusing a pool slot cannot change its path or holding berth. Six compact,
authored path families replace the old identical spiral: Neptune's paired arcs,
Uranus's side/top hooks, Saturn's single-file trains, Jupiter's broad sweeps,
Mars's serial peeling loops, and Earth's corner foldbacks. Control points and
timing are adaptations; later-planet trajectories are not ROM reconstructions.
[Evidence and limits](research/gyruss-waves.md).

Combat only completes when every scheduled fighter has been destroyed or has
made a genuine final escape. Invisible attackers remain unresolved. Only the
last two survivors may flee, after an attack/return opportunity and a short
endgame delay. The delay and flight speeds are authored tuning.

A depleted initial formation can receive one final group of ten, using a fixed
randomly selected entrance from the first five destination families. This build
uses **more than 30 initial entrance kills** as its once-only trigger after the
first four groups finish entering. Contemporary sources disagree on the exact
trigger; this threshold and evaluation timing are explicitly inferred, not
verified original logic. Normal stages therefore schedule 40 or 50 fighters.
Chance stages retain four groups of ten and terminal departures.

There is no global timeout that wins ordinary combat. A no-fire regression runs
100,000 updates without clearing or losing any of the 40 living fighters.
`STATE` exposes spawned, destroyed, escaped, entrance progress and six lifecycle
mode counts. `SET clear_stage 1` is an explicit debug-only completion operation
for deterministic transition captures; normal gameplay never invokes it.

The route begins two warps from Neptune, then continues through Uranus, Saturn,
Jupiter, Mars and Earth. Each arrival leads to a chance stage: destroy 40
spiraling enemies without collision deaths. Perfect completion awards an extra
10,000 points. Subsequent planetary groups use three regular warps and one
chance stage. Completing the route repeats with increased difficulty.

See [controls](controls.md) for the three steering modes and pause menu, and
[debug protocol](debug-protocol.md) for capture, state, stage selection and
scripted input. The onboard IMU drives tilt steering. This board has no onboard speaker or
audio output stage.

Tilt is the third selectable steering mode. Hold the device comfortably still
for half a second at start/resume; that pose becomes neutral. Bank left/right
to change orbit direction, with speed increasing beyond a three-degree dead
zone and reaching its limit around25degrees. Accelerometer gravity corrects
gyroscope drift. A stale, implausible or interrupted motion stream stops steering
and requires another neutral calibration. BOOT can also fire when automatic firing is disabled; touch-center still
opens the pause menu. The desktop provides virtual tilt with J/L and re-centers
with0/K.
