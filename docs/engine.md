# Shared arcade engine

The engine runs one active simulation at 60 Hz. `engine_state.game` is a tagged
union, selected by the immutable descriptor in `engine_catalogue()`. Descriptors
supply initialization, rules, rendering, settings, UI telemetry, level selection,
canonical hashing and bounded state JSON. Frontends use session operations and do
not branch on game identity. Adding a game requires a state member, descriptor
and registry entry; there is no dynamic loader or frame-time allocation.

Boot shows the ROUND ARCADE launcher. Navigation initializes the neighboring game
and eases its card across 220 pixels. PLAY or BOOT starts the selected game.
Settings is shared; fire rate and enemy speed belong to each game's record, while
control mode, tilt calibration and tilt sensitivity belong to the session. High
scores and Tempest's CLASSIC/NEON preference survive switching during a session.
They are RAM records, not flash persistence. Home saves that record and returns
to the launcher with the selected game ready for a fresh start.

`engine_step` dispatches one explicit input. `engine_tick` dispatches retained
G1 input and consumes angle, start, pause, quit, special, home, navigation and menu
pulses; turn and fire remain held. `engine_update` samples shared physical controls,
merges transport and keyboard input, updates UI telemetry and advances unless
externally frozen. Frozen sessions still sample the IMU, calibration and reboot
hold. Explicit G1 STEP advances a frozen session. Physical Quit means Home;
injected G1 quit retains the selected game's own behavior. Game selection consumes
held physical touch/BOOT without discarding tilt calibration and clears injected
input so no pulses leak between games.

The launcher gates effects selection by descriptor capability. Cosmetic effects
never participate in canonical simulation hashes. Shared menus use the original
fire rate 1–10, enemy speed 25–200 in steps of 25 and tilt sensitivity 50–200 in
steps of 25. Invalid values leave settings unchanged; valid repeated values succeed.

Rendering is pure and returns a pointer to the selected descriptor's immutable
RGB565 palette. Presentation queues must store that palette together with each
indexed framebuffer, so changing games cannot recolor an older queued frame.
Shared UI uses the same first 24 palette entries for both games. The session adds
a Home affordance on game title and game-over screens; standalone Gyrus rendering
continues to produce the original pixels.

`engine_debug_session` exposes the existing bounded G1 parser callbacks. GAME
selects a title, HOME returns to the launcher, and GAMES returns bounded catalogue
and session JSON. Active Gyrus STATE serialization delegates to the original
adapter, preserving field order, values and canonical hashes. RESET clears the
active game's record, input and shared control defaults, initializes its supplied
seed and shows its title; records of other games survive. A full `engine_init`
clears every record. Frame captures pair session pixels with their returned palette.
