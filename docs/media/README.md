# On-device demo

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

The main README uses this identical file's
[GitHub video attachment](https://github.com/user-attachments/assets/ca1884a3-ee16-48ce-9918-4bd2ffbf2fec)
for its inline player. Keeping a copy here makes the demo available with a clone.
GitHub's player syntax is documented in
[Attaching files with GitHub CLI](https://docs.github.com/en/github-cli/github-cli/attaching-files-with-github-cli#embedding-a-video).

## Reproduce the conversion

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

`arcade-launcher.png`, `tempest-gameplay.png`, `tempest-entry.png`, and
`tempest-flight.png` are native-resolution 240×240
indexed frames expanded to PNG by the CRC-checked USB G1 capture client. They
show independently authored launcher/vector graphics from the shared firmware.
They contain no camera data or downloaded arcade artwork.

`gyrus-neptune-formation.png` and `gyrus-saturn-formation.png` show the corrected
paired-arc and single-file formation families, captured from the final board
firmware with the same USB client.
