#include "gyrus/video.h"
#include "gyrus/assets.h"

video_span video_strip_span(unsigned strip, bool full) {
    /* For each 16-row band: min/max x of a circle centered at120,120,
     * radius122. Rounded outward to include the whole physical aperture.
     * 49120 pixels per frame versus57600 with rectangular windows. */
    static const video_span spans[VIDEO_STRIPS] = {
        {58, 125}, {37, 167}, {23, 195}, {13, 215}, {6, 229},  {1, 239},  {0, 240}, {0, 240},
        {0, 240},  {1, 239},  {5, 231},  {12, 217}, {22, 197}, {36, 169}, {57, 127}};
    if (strip >= VIDEO_STRIPS)
        return (video_span){0};
    return full ? (video_span){0, GAME_WIDTH} : spans[strip];
}
size_t video_expand_strip(const uint8_t *frame, unsigned strip, bool full, uint16_t *output,
                          size_t capacity) {
    video_span span = video_strip_span(strip, full);
    size_t count = (size_t)span.width * VIDEO_STRIP_ROWS;
    if (!frame || !output || count == 0 || capacity < count)
        return 0;
    const uint8_t *source = frame + (size_t)strip * VIDEO_STRIP_PIXELS + span.x;
    for (unsigned y = 0; y < VIDEO_STRIP_ROWS; ++y) {
        for (unsigned x = 0; x < span.width; ++x)
            output[(size_t)y * span.width + x] = gyrus_palette[source[x]];
        source += GAME_WIDTH;
    }
    return count;
}
