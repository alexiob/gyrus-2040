# Touch and button controls

The input layer runs independently of hardware and translates raw touch, BOOT, and millisecond samples into shared `engine_input` (`game_input` remains a compatibility alias). Each game uses fixed 60 Hz ticks. Firmware supplies native 240 x 240 CST816S coordinates to `engine_update()`, which owns control sampling and dispatch once per tick. The selectable modes are split hold, border drag, and tilt. A mode change takes effect immediately and survives game start/quit within the current boot; settings are not written to flash.

## Play

The initial mode is **LEFT / RIGHT** (`CONTROLS_SPLIT_HOLD`). Split touch steering uses the inverted mapping: hold the left side to orbit clockwise (+1), or the right side counterclockwise (-1). A 24-pixel vertical neutral strip prevents direction jitter. The center circle (radius 36 pixels) is reserved for pause; a short tap there pauses on release. Touch release stops turning. Touches outside the round display do not turn the ship.

**BORDER DRAG** (`CONTROLS_RELATIVE_DRAG`) follows relative angular finger movement around the border, in either direction. The ship does not jump to the finger's initial angle. Starting or resuming a drag anchors at the current finger position. A 56-pixel center deadzone prevents unstable angles; passing through it re-anchors the drag. Crossing 0/360 degrees works continuously. A movement larger than 45 degrees in one sample is treated as a discontinuity and re-anchors without moving the ship. Angle conversion uses an integer lookup table and interpolation; there is no floating-point work in the tick path.

**TILT** (`CONTROLS_TILT`) uses the onboard accelerometer and gyroscope. Open SETTINGS from the title, game-over, or pause menu and choose TILT on its control-mode row. Hold the board in a comfortable neutral pose for half a second. The Settings screen shows calibration progress and a live steering preview, so calibration can be checked before playing. The small ship arrow at the bottom of the launcher and Gyrus title/game-over menu also moves left/right in proportion to the calibrated tilt input; it stays centered until ready and in other steering modes. Tap it in TILT mode to clear the old reference, then hold the board still in a comfortable pose for half a second to learn a new neutral position. The touch target spans the lower strip so the moving icon is easy to tap. A cancelled contact or failed touch read cannot reset calibration. Steering is disabled while calibration is pending. Fresh calibration survives Settings, title, game-over, pause, and play transitions. It restarts on mode change, stale/invalid sensor data, an unusable bank projection, or an explicit `controls_tilt_reset()`.

The RP2040 board's IMU mounting is rotated relative to the LCD. Hardware samples
are converted to a display-aligned body frame before entering the portable controls
layer: display X = sensor Y, display Y = −sensor X, display Z = sensor Z. Both
acceleration and gyro use the same proper rotation. A physical right-edge-lowered
pose measured sensor `[140,623,-795]` mg; a left-edge-lowered pose measured
`[-23,-510,-806]` mg, confirming the horizontal axis and sign. Thus lowering the
right edge makes the menu arrow move right; lowering the left makes it move left.
Front/back pitch does not steer for a face-up neutral. STATE hardware vectors
remain in the raw sensor frame for diagnosis; the desktop virtual IMU already
uses the display frame. The earlier unrotated mapping incorrectly treated sensor
X as screen-left/right.

The neutral reference uses the full three-dimensional gravity vector and learns gyro bias on all three axes. Screen +X is projected onto the plane perpendicular to the neutral gravity vector to define rightward bank; the cross product of those two directions defines the corresponding gyro rotation axis. A board lying flat therefore uses a different gyro combination from an upright board. The implementation supports positive/negative Z flat poses, positive/negative Y upright poses, and inclined poses without depending on a nonzero initial X/Z projection. A board standing exactly on its side uses projected screen Y as a deterministic fallback. The angle and basis calculations use integers.

Banking toward the learned right direction produces clockwise orbit speed; the opposite bank produces counterclockwise speed. The input is proportional to bank angle, with a three-degree neutral deadzone and full speed near 25 degrees. Tilt sensitivity is adjustable from 50% to 200% in 25% steps. Its gain applies after the deadzone and before the 900-angle-unit-per-tick safety cap. `controls_sample.tilt_sensitivity_percent` supplies the current value; zero uses the 100% default and other values clamp to 50..200. Gravity and the bias-corrected gyro projection feed a complementary filter; a second low-pass filter smooths the bank. A gravity-vector pose change above approximately two degrees or gyro magnitude above 15 degrees/second postpones calibration. Shocks/freefall, samples older than 100 milliseconds, and degenerate current bank projections stop steering and require fresh calibration. No unstable angle is derived from a near-zero projection.

`controls_tilt_ready()` and `controls_tilt_progress()` provide readiness and 0..1000 calibration progress. `controls_tilt_preview()` provides signed -1000..1000 steering after sensitivity/deadzone/cap. Fresh IMU input is processed in every TILT screen context, but `game_input.angle_delta` is emitted only during play. BOOT fires manually in Gyrus and triggers the Superzapper in Tempest; center touch pauses both games. The desktop simulation uses the same motion interface and learned-axis rules.

**Autofire is enabled by default**, so playing requires no button hold.
BOOT also fires if autofire is disabled through the debug setting. Both edges are debounced for 25 milliseconds. At the title or game-over screen, one BOOT press starts a game; in the pause menu it resumes; in Settings it performs BACK to the parent menu. A held BOOT or touch is consumed when the screen context changes, so a press used to start/resume does not also fire or accidentally activate a menu item. Release and press again to fire.

## Menus

Tap actions complete on release after at most 650 milliseconds. Moving more than 12 pixels from the original contact, leaving the original action, or a touch read error cancels a pending menu action until a **valid physical release** is confirmed. A synthetic released value from a failed I2C read cannot clear that interlock. Continued contact after an error cannot re-arm Quit or the Reboot hold. Starting a gameplay drag outside the pause circle does not activate pause when the drag passes through the center.

The launcher has graphical game cards, left/right arrows, PLAY and SETTINGS. Swipe its card horizontally to browse; Tempest also has a CLASSIC/NEON style button. The title and game-over screens have START and SETTINGS, plus a common Home arrow. The pause menu has RESUME, SETTINGS, QUIT, and **REBOOT: HOLD 5S**. Control mode is selected only inside Settings. Physical Quit returns to the shared launcher. A legacy injected `quit` returns to the selected game title. Reboot requires a continuous, stationary five-second hold; a brief tap does nothing. Reboot is a one-shot `reboot_requested` event for the firmware to handle. `controls_reboot_progress()` exposes 0 to 1000 for visible hold feedback.

Settings starts with the control-mode selector and provides minus and plus buttons for Fire Rate (1..10, default5), Enemy Speed (25%..200% in25% steps, default100%), and Tilt Sensitivity (50%..200% in25% steps, default100%). BACK returns to the title, game-over, or pause menu that opened Settings. Controls emits one `game_input.menu_action` on a valid short release; the game owns changing and bounding each value. Holding a button does not repeat changes. Leaving the button, moving more than12 pixels, a touch bus error, or entering a different menu while already touching cancels that contact. BOOT also performs BACK from Settings.

The exact half-open hit rectangles are in `include/gyrus/controls.h` and must also be used by the renderer:

| Context | Action | X | Y |
| --- | --- | --- | --- |
| Title / game over | Start | 60..179 | 92..135 |
| Title / game over | Settings | 48..191 | 145..176 |
| Title / game over | Recenter tilt | 68..171 | 216..239 (inside round rim) |
| Pause | Resume | 60..179 | 80..103 |
| Pause | Settings | 60..179 | 115..138 |
| Pause | Quit | 60..179 | 149..172 |
| Pause | Reboot: hold5s | 68..171 | 183..206 |
| Settings | Control mode | 48..191 | 52..75 |
| Settings | Fire minus | 42..79 | 94..119 |
| Settings | Fire plus | 160..197 | 94..119 |
| Settings | Enemy minus | 42..79 | 137..162 |
| Settings | Enemy plus | 160..197 | 137..162 |
| Settings | Tilt minus | 42..79 | 180..205 |
| Settings | Tilt plus | 160..197 | 180..205 |
| Settings | Back | 78..161 | 209..230 |

## Hardware findings and safe BOOT sampling

The Waveshare RP2040-Touch-LCD-1.28 schematic was inspected as a rendered page and traced alongside the downloaded vendor configuration code:

- K2, labelled RUN, connects the RP2040 RUN signal directly to ground. RUN is silicon pin 26, not a software GPIO. Pressing the physical RESET button immediately resets/holds the processor; firmware cannot intercept the press, use it to quit, or measure a five-second duration. No software remapping of physical RESET is claimed.
- K1, labelled BOOT, pulls QSPI_SS_N low through R10 (1K). This is also the external W25Q128JVSIQ flash CS signal, at RP2040 pin 56 and flash pin 1. BOOT is not a normal spare GPIO. R9 is marked NC.
- Vendor `DEV_KEY_Config()` only configures ordinary GPIO inputs used for touch/IMU interrupt lines; it does not provide a safe BOOT reader or intercept RUN.

`firmware/buttons.c` samples BOOT through a callback that resides entirely in SRAM. The callback saves the complete QSPI control register, temporarily makes CS high impedance, waits for the input to settle, reads the active-low input, then restores the original control register. `flash_safe_execute()` first disables interrupts and locks out the other core. The function refuses to sample before core 1 registers as a lockout victim. A failure clears the returned raw fire state.

Integration requirements:

1. Link `pico_flash` and `pico_multicore`.
2. Core 1 must call `flash_safe_execute_core_init()` before publishing its ready flag. Core 0 waits for that flag before calling `buttons_read_fire()`.
3. Call `buttons_read_fire()` from core 0's main loop, never an interrupt. Do not hold a resource that core 1 needs to enter lockout. No DMA channel may read XIP/flash during the callback. LCD transfers from SRAM are compatible; directly DMA-reading asset data from flash is not.
4. Keep normal dual-core lockout enabled; do not define `PICO_FLASH_ASSUME_CORE1_SAFE` merely to bypass coordination.
5. On a failed touch read, set `controls_sample.touch_invalid=true`. `controls_cancel_touch()` is also supported before the update and explicitly ignores that next synthetic sample; both mechanisms quarantine the contact until a subsequent valid physical release. Using them together is supported and regression-tested. On BOOT sample failure pass `fire_pressed=false`.
6. Firmware alone handles `reboot_requested` with its system reboot facility. The pure controls module never resets hardware.

Reference material resides in the original checkout under `reference/hardware/board-schematic.pdf` and `reference/hardware/vendor-excerpts/RP2040-Touch-LCD-Code/RP2040-Touch-LCD-1.28/c/lib/Config/DEV_Config.c`. The worktree intentionally does not duplicate the downloaded PDFs. Hardware button behavior still needs physical acceptance testing after the coordinated firmware build; host tests validate the input state machine and math, not electrical timing.

## Motion driver configuration

`firmware/sensors.c` identifies QMI8658 at address 0x6b (fallback 0x6a), disables acquisition while configuring it, and verifies the key settings by readback before reporting `imu_ok`. CTRL1=0x40 enables incrementing little-endian reads; CTRL2=0x16 selects +/-4g; CTRL3=0x56 selects +/-512 degrees/second. Both use ODR code 6, nominally125Hz for QMI8658C and about112Hz in the QMI8658A's 6DOF mode. CTRL5=0x77 enables matching low-pass filters. CTRL7=0x83 enables both sensors in SyncSample mode; the driver waits180ms for startup once, never in the tick path.

At startup, the driver disables internal AHB clock gating using the documented CAL1_L=1 / CTRL9 command0x12 handshake, with bounded50ms completion/acknowledgment polling. Unacknowledged configuration disables IMU availability. `sensors_motion(controls_motion *)` reads STATUSINT.Avail to start the sensor's shadow-register lock, waits the specified12 microseconds, then reads a bounded12-byte burst from0x35 through GZ_H. The final byte automatically releases the lock, so an ODR update cannot mix old and new axes/bytes. A timed-out read marks the lock uncertain; recovery explicitly releases and discards that old sample before accepting another. This follows the QST datasheet's sections13.2.1–13.2.3; a timestamp-only heuristic is not used. Scaling is8192LSB/g and64LSB/(degree/second), yielding integer mg and millidegrees/second with a host millisecond timestamp. False also means no fresh sample this tick; its output is cleared. Tilt accepts the last filtered state for up to100ms between fresh samples. Core0 serializes IMU and touch operations on the existing400kHz I2C bus; each bus call retains a2ms timeout.

The downloaded older vendor header labels gFS=100 as512dps. Both the local QMI8658A table and the manufacturer's [QMI8658C datasheet](https://qstcorp.com/upload/pdf/202202/QMI8658C%20datasheet%20rev%200.9.pdf) specify gFS=101 for512dps, so the new driver corrects that enum mismatch rather than copying its scale setting. Driver tests assert the selected full-scale register and independently check positive/negative physical-unit conversion.

## Other useful modes to consider

A **horizontal relative swipe** can map finger travel to orbit distance without requiring a border gesture. It tolerates a thumb partly covering the screen and could be easier for one-handed play. An **absolute border point** mode can move toward the touched angle, but needs a speed cap and careful crossing behavior to avoid teleporting the ship. Autofire is implemented and enabled by default to remove the need to hold BOOT while steering. These remain proposals; split hold, border drag, and tilt are implemented.

## Verification

`tests/test_controls.c` exercises hold/release; pause/Settings navigation and adjustments; half-open menu hit boundaries; held-button suppression; sensor-error quarantine; BOOT debounce and context changes; quit; one-shot five-second reboot; clock rollover; clockwise/counterclockwise drag; angular wrap/deadzone/discontinuities; all three mode choices; 3D tilt calibration in upright/flat/inclined poses, all-axis stationary bias and learned gyro projection, live Settings preview, calibration preservation across fresh screen transitions, gyro prediction and gravity correction, proportional steering, saturation, angular wrap, stale-sensor stop; and tilt sensitivity default, gain limits, deadzone, and saturation. `tests/test_sensors.c` uses isolated fake I2C/registers to exercise the actual driver initialization/readback, correct full-scale settings, little-endian signed scaling, fresh-data gating, timeout clearing, fallback address, configuration-command timeouts, ODR updates while locked, and stale-lock cleanup after a burst failure. These tests pass with strict warnings and address/undefined-behavior sanitizers.

The BOOT sampler was separately compiled with ARM compiler flags. Object disassembly places `sample_bootsel` and both register literals in `.time_critical.sample_bootsel`; the callback contains no calls into flash. Final linked placement and physical orientation/button/timing acceptance belong to firmware integration.

## Launcher and Tempest actions

`CONTROLS_HUB` produces one `navigate=-1|1` pulse on an arrow tap or a horizontal
card swipe. The swipe threshold is 28 pixels with at most 40 pixels of vertical
travel; action taps must release within 650 ms and stay within 12 pixels.
`controls_consume_inputs()` quarantines a held touch and BOOT across game
selection, including transitions between identical title contexts, while retaining
fresh tilt calibration.

`CONTROLS_TEMPEST_PLAY` shares all steering modes and center pause. BOOT emits a
debounced rising-edge `special` pulse instead of held fire. The visible ZAP target
[90,150) × [176,202) emits the same pulse on a valid release. Gameplay autofire is
independent of that action. Reboot and all Settings mechanics remain shared.
