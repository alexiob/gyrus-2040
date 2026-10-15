# Demo videos and screenshots

## Menu, Gyrus, and Tempest

[round-arcade-demo.mp4](round-arcade-demo.mp4) shows the current multi-game
launcher, Settings, Gyrus combat and a warp, then Tempest combat, Superzapper,
the level exit, and entry into the next web.

| Property | Repository version |
| --- | --- |
| Duration | 61.6 seconds; 1,848 frames |
| Dimensions | 720 × 720; nearest-neighbor 3× scaling of the native 240 × 240 frame |
| Video | H.264 High, level 3.1; YUV 4:2:0; 30 frames/s |
| Size | 3,253,045 bytes (3.1 MiB) |
| Audio | No audio stream |
| Startup | MP4 metadata precedes video data (`faststart`) |
| Source | Native headless renderer, using the board's C engine and assets |

This is a software recording, not a video of the physical LCD. The simulation
advances at its normal 60 Hz; capturing every second tick yields real-speed
30 fps playback. Inputs use normal steering, autofire, and two Superzapper
presses. Both games naturally reach stage 2 with three lives remaining;
invincibility, injected scores, enemy clearing, and level skipping are unused.
The physical board is not accessed by the capture script.

The [capture manifest](round-arcade-demo.json) records the engine revision,
seed, scene timestamps, end states, encoding, and SHA-256. The main README uses
the identical file's
[GitHub attachment](https://github.com/user-attachments/assets/c9fabc5a-87a6-4457-b8e2-3b7ec1dce93b)
for inline playback; the MP4 stays in Git for offline viewing.

### Record the demo again

Install FFmpeg, build the host target, and run from the repository root:

```sh
cmake --preset host
cmake --build --preset host
python3 tools/record_demo.py
```

The script uses only Python's standard library and the existing G1 client.
It starts its own headless process, CRC-checks each captured frame, and pipes PNG
frames directly to FFmpeg, so it needs no window capture, serial port, or Python
imaging packages. It writes the video to `docs/media/round-arcade-demo.mp4` and
lossless screenshots plus a detailed state log to `artifacts/demo/`.
`--output`, `--evidence`, `--headless`, and `--seed` override these defaults.

To validate the encoded file:

```sh
ffprobe -v error -show_streams -show_format docs/media/round-arcade-demo.mp4
ffmpeg -v error -i docs/media/round-arcade-demo.mp4 -f null -
```

Re-recording changes the local MP4 only. Upload the new file as a GitHub video
attachment and update the README URL and manifest when publishing a replacement.
An ordinary repository MP4 link does not create GitHub's inline video player.

## Earlier on-device Gyrus demo

[gyrus-2040-demo.mp4](gyrus-2040-demo.mp4) is the optimized, silent copy of the
user-supplied `gyrus-2040.mov`. The full clip is retained.

| Property | Repository version |
| --- | --- |
| Duration | 34.57 seconds |
| Dimensions | 720 × 1280, upright portrait |
| Video | H.264 High, level 3.1; YUV 4:2:0; 30 frames/s |
| Size | 4,180,112 bytes (4.0 MiB), approximately 93% smaller than the source |
| Audio | No audio stream |
| Startup | MP4 metadata precedes video data (`faststart`) |

Camera/location metadata and auxiliary data tracks are removed. The original
MOV is not committed.

The main README's collapsed on-device section uses this identical file's
[GitHub video attachment](https://github.com/user-attachments/assets/ca1884a3-ee16-48ce-9918-4bd2ffbf2fec)
for its inline player. Keeping a copy here makes the demo available with a clone.
GitHub's player syntax is documented in
[Attaching files with GitHub CLI](https://docs.github.com/en/github-cli/github-cli/attaching-files-with-github-cli#embedding-a-video).

### Reproduce the conversion

With FFmpeg installed, replace `SOURCE.mov` with the original recording:

```sh
ffmpeg -i SOURCE.mov \
  -map 0:v:0 -an -sn -dn \
  -vf 'scale=720:-2:flags=lanczos,fps=30,setsar=1' \
  -c:v libx264 -preset slow -crf 25 -maxrate 1800k -bufsize 3600k \
  -profile:v high -level:v 3.1 -pix_fmt yuv420p -g 60 \
  -map_metadata -1 -map_chapters -1 -movflags +faststart \
  gyrus-2040-demo.mp4
```

FFmpeg applies the source's rotation before scaling, so playback does not rely
on camera orientation metadata. The encoding was checked by decoding the full
file and probing the resulting streams and MP4 atom order.

## Shared arcade screenshots

`menu-gyrus.png`, `menu-tempest.png`, `arcade-settings.png`, `gyrus-combat.png`,
`tempest-combat.png`, and `tempest-dive.png` are fresh lossless 240 × 240 PNGs
from the native renderer, captured through the same CRC-checked G1 client.
They show the current menu, settings, and both games without video compression.
The capture script reproduces them in `artifacts/demo/` as `menu-gyrus.png`,
`menu-tempest-neon.png`, `settings.png`, `gyrus-gameplay.png`,
`tempest-combat.png`, and `tempest-gameplay.png`, respectively.

`arcade-launcher.png`, `tempest-gameplay.png`, `tempest-entry.png`, and
`tempest-flight.png` are native-resolution 240×240
indexed frames expanded to PNG by the CRC-checked USB G1 capture client. They
show independently authored launcher/vector graphics from the shared firmware.
They contain no camera data or downloaded arcade artwork.

`gyrus-neptune-formation.png` and `gyrus-saturn-formation.png` show the corrected
paired-arc and single-file formation families, captured from the final board
firmware with the same USB client.
