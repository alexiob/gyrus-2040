# G1 game debug protocol

The RP2040 USB CDC endpoint, desktop loopback socket, and headless process use the
same ordered request/response protocol. The connection contains protocol bytes
only; host diagnostics go to stderr. No flashing, raw memory access, or arbitrary
function execution is exposed. The desktop binds **127.0.0.1**, default port
20400 (`--debug-port 0` disables it). Only one client is served at a time. Each desktop response has one absolute
five-second write deadline; an unresponsive client is disconnected.

## Requests and JSON replies

Send one ASCII request, terminated by LF (CRLF also works):

```text
G1 1 HELLO
G1 2 STATE
G1 3 INPUT turn=-1 fire=1
G1 4 STEP 60
G1 5 SET invincible 1
G1 6 LEVEL 12
G1 7 FRAME
```

`G1` identifies version 1. The decimal request ID is 0..4294967295 and is echoed
in the response. Requests are at most 255 bytes before LF. Spaces or tabs
separate tokens. Names and commands are case sensitive. No quoting is supported.
Overlong or invalid ASCII lines are discarded through LF, then parsing resumes.
An empty line does nothing. Numbers are decimal integers without a leading `+`.

A successful ordinary response is a single JSON line:

```json
{"v":1,"id":4,"ok":true}
```

Failure replies use `ok:false` and an `error` string (`syntax`, `range`,
`rejected`, `unavailable`, `line_too_long`, `invalid_ascii`, `invalid_frame`,
`invalid_state`, or `internal`). If the request header cannot be parsed or the
line exceeds its bound, the response ID is 0. A rejected request does not mutate
the game. Process requests sequentially; wait for the entire response before
sending the next request, especially over USB.

| Command | Arguments | Behavior |
| --- | --- | --- |
| `HELLO` | none | Reports protocol name, maximum line length and step count. |
| `STATE` | none | Reports a `state` JSON object including a deterministic hash. |
| `INPUT` | one or more `key=value` tokens | Changes only the supplied input fields. |
| `STEP` | integer 1..3600 | Calls the simulation update exactly this many times. |
| `SET` | field, signed 32-bit integer | Mutates one allowlisted scalar after range validation. |
| `LEVEL` | integer 1..999 | Selects a route stage and enters its introduction. |
| `RESET` | seed 0..4294967295 | Resets to the title with that deterministic seed; clears injected input. |
| `PAUSE` | none | Freezes automatic simulation ticks. |
| `RESUME` | none | Resumes automatic simulation ticks. |
| `FRAME` | none | Returns indexed pixels and an RGB565 palette as described below. |

`INPUT` keys: `turn` (-1, 0, 1), `angle_delta` (-32768..32767), and `fire`,
`start`, `pause`, `quit` (0 or 1). Duplicate or unknown keys reject the entire
request. `turn` and `fire` persist until changed. The other fields are pulses,
consumed on the next simulation tick. For example `INPUT start=1`, then `STEP 1`
starts a run. Positive rotation is clockwise; angle 0 points up and 16384 right.
A full turn is 65536 angle units. Inputs can be combined with physical controls.

The shared adapter permits `SET` only for `score` / `high_score`
(0..99999999), `lives` (0..99), `ship_angle` (0..65535), `invincible`,
`autofire`, and `double_fire` (0 or 1), plus `control_mode` (0 hold, 1 border
drag, 2 tilt). If the adapter has an attached controls state, selecting a mode
updates both simulation UI and live controls. Stage changes use `LEVEL`; the entity
arrays and game phase cannot be overwritten. `STATE` reports tick, RNG state,
score, high score, lives, stage, planet, warp, double fire, phase name, ship
angle, invincibility, autofire, frozen status, control mode, tilt calibration readiness, reboot progress,
entity counts, and state hash.
The hash covers simulation state; freeze status and retained debug inputs live
outside it.

`STEP` counts update calls; starting a run or toggling the pause menu can consume
a call without incrementing the reported game tick.

`PAUSE`/`RESUME` control the external scheduler; the game's own pause menu is
controlled with `INPUT pause=1`. `STEP` bypasses the scheduler freeze but still
obeys the game's phase behavior. Headless mode advances only on `STEP` regardless
of scheduler freeze. For deterministic desktop work, send `PAUSE` first, then
`RESET`, `INPUT` and `STEP` commands. A FRAME is rendered and streamed
synchronously on the simulation thread, so the capture is internally consistent
and changes no simulation state.

## Binary screenshot framing

FRAME success begins with one JSON line, for example:

```json
{"v":1,"id":7,"ok":true,"format":"indexed8","width":240,"height":240,"palette_bytes":512,"pixel_bytes":57600,"crc32":1234567890}
```

Immediately after that LF comes:

1. `palette_bytes` bytes: RGB565 words in **little endian**, indexed from 0.
2. `pixel_bytes` bytes: one unsigned palette index per pixel, row major, top left first.
3. Exactly one LF byte as a trailer.

The CRC32 covers palette bytes followed by pixel bytes, excluding the header and
trailer. It is standard reflected IEEE CRC32 (polynomial 0xEDB88320, initial
0xFFFFFFFF, final XOR 0xFFFFFFFF; the same value as Python `zlib.crc32`). Width
and height are 1..240, palette length 2..512 bytes and even, and pixel length must
match width times height, at most 57600. Palette indices must be in range.
Binary payloads may contain any byte including LF; consume the declared length,
never scan for a delimiter within payloads. Firmware emits pixel data in at most
256-byte write chunks. An unavailable or invalid frame returns only a JSON error
line, without binary data.

## Client examples

The client uses the Python standard library for subprocess/TCP and PNG output.
USB serial additionally needs the pinned pyserial dependency.

```sh
python3 tools/gyrusctl.py --headless build/gyrus_headless STATE
python3 tools/gyrusctl.py --tcp 20400 PAUSE
python3 tools/gyrusctl.py --tcp 20400 INPUT fire=1 turn=1
python3 tools/gyrusctl.py --tcp 20400 STEP 120
python3 tools/gyrusctl.py --tcp 20400 --output capture.png FRAME
python3 tools/gyrusctl.py --serial /dev/cu.usbmodemXXXX STATE
```

A headless process has no state after exit. Use `--batch commands.txt` for a
persistent scenario in one subprocess. A batch file contains plain commands,
one per line, without `G1` or IDs. Blank lines and lines beginning with `#` are
ignored. An optional positional command runs after the batch. Example:

```text
RESET 1234
INPUT start=1
STEP 1
SET invincible 1
INPUT fire=1 turn=1
STEP 600
STATE
FRAME
```

On desktop, arrows/A,D turn, Space emulates BOOT fire, Enter starts, Escape
pauses, and M cycles the steering modes (left/right hold, border drag, tilt).
In tilt mode hold J/L to bank left/right; release to return level. The host
provides a simulated accelerometer and gyro through the same controls path as
the board. Press 0 or K to level and recalibrate; hold still for 500 ms at start
or after resume. Arrow keys remain a manual steering override in every mode.
The mouse emulates the touch panel,
including title and pause menu hit regions. Desktop simulation uses a fixed
60 Hz accumulator and the shared core, palette and indexed framebuffer.

## Embedding

`gyrus/debug.h` is a transport independent bounded parser. State callbacks have
a 1536-byte buffer (at most 1535 JSON bytes plus NUL); the client caps JSON
response lines at 2048 bytes. Supply synchronous
write, state, set, action, input and frame callbacks, then feed arbitrary byte
fragments. The write callback must accept all bytes before returning; it cannot
retain pointers to stack buffers. FRAME palette/pixel pointers must remain
stable until `gyrus_debug_feed` returns. Unsupported callback operations return
`unavailable` or `rejected`.

`gyrus/debug_game.h` supplies the allowlisted adapter shared by firmware and
hosts. Initialize it with game/framebuffer pointers and an output callback;
obtain callbacks with `gyrus_debug_game_callbacks`, then initialize the parser.
FRAME uses the same `game_render` function, including its controls UI, as the
physical display and desktop. An automatic scheduler checks `adapter.frozen` before ticking and calls
`gyrus_debug_game_tick` to consume pulse inputs. Poll/feed requests between
simulation ticks on the same thread. Keep normal logs off the debug transport.
