# Physical tilt-axis mapping and tap-to-recenter

The user specified lowering the left/right edge like banking a tray. Two held
poses were captured directly over USB with the screen roughly face-up:

| Pose | Raw accelerometer X,Y,Z (mg) |
| --- | --- |
| Right edge lower | 140, 623, −795 |
| Left edge lower | −23, −510, −806 |

This confirms that **sensor Y is screen-left/right**. The previous firmware
passed sensor-frame values into controls that assumed screen X, producing the
reported wrong-axis response. Firmware now applies the same proper rotation
to acceleration and angular velocity before calling the portable controls:
`display = [sensorY, −sensorX, sensorZ]`. Raw STATE telemetry remains unchanged;
the desktop virtual IMU already supplies display-frame vectors.

On the title/game-over menu, tapping the small moving ship arrow resets the old
reference in TILT mode. Hold still in the desired comfortable pose for half a
second to learn a new neutral. The caption changes from TAP SHIP TO CENTRE to
HOLD STILL TO CENTRE during calibration. The lower strip is a generous touch
target, restricted to the round display. Contacts act on valid short release;
dragging away or a touch read error cancels them. No game is started by this tap.

All 21 sanitizer-enabled host suites pass. The new sensor-to-controls regression
uses the captured physical poses and checks both steering signs, rejects
front/back pitch as steering for a face-up neutral, and checks gyro rotation.
The input regression resets a nonzero bank through a real press/release,
calibrates that new comfortable pose to zero, and checks touch-error cancellation.
The ARM firmware builds successfully and picotool verified the written flash
before starting it. ELF text is 138,820 bytes; BSS is 141,648 bytes.

Flashed UF2 SHA-256:
`47ae8fe66c07534747e9e3046d1dd5129010d5490f97733108aab4cf797d43e4`.

Raw held-pose snapshots, flash output, and live interaction telemetry are in
`artifacts/tilt-recenter/`. Physical user acceptance is recorded in the results
file when received.
