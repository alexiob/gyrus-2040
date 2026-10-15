#include "gyrus/video.h"
#include "gyrus/assets.h"
size_t video_expand_strip(const uint8_t *frame, unsigned strip, bool full, uint16_t *output,
                          size_t capacity) {
    return video_expand_strip_palette(frame, gyrus_palette, strip, full, output, capacity);
}
