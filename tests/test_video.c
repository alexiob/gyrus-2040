#include "gyrus/assets.h"
#include "gyrus/video.h"
#include <assert.h>
#include <string.h>
static uint8_t frame[GAME_WIDTH * GAME_HEIGHT];
static uint16_t storage[VIDEO_STRIP_PIXELS + 2];
int main(void) {
    for (unsigned i = 0; i < sizeof frame; ++i)
        frame[i] = (uint8_t)(i % GYRUS_COLOR_COUNT);
    unsigned optimized_pixels = 0;
    for (unsigned mode = 0; mode < 2; ++mode) {
        bool full = mode != 0;
        for (unsigned strip = 0; strip < VIDEO_STRIPS; ++strip) {
            video_span span = video_strip_span(strip, full);
            assert(span.width > 0 && span.x + span.width <= GAME_WIDTH);
            if (full)
                assert(span.x == 0 && span.width == GAME_WIDTH);
            else
                optimized_pixels += (unsigned)span.width * VIDEO_STRIP_ROWS;
            memset(storage, 0x5a, sizeof storage);
            size_t count = video_expand_strip(frame, strip, full, storage + 1, VIDEO_STRIP_PIXELS);
            assert(count == (size_t)span.width * VIDEO_STRIP_ROWS);
            assert(storage[0] == 0x5a5a && storage[count + 1] == 0x5a5a);
            for (unsigned y = 0; y < VIDEO_STRIP_ROWS; ++y) {
                for (unsigned x = 0; x < GAME_WIDTH; ++x) {
                    int dx = (int)x - 120, dy = (int)(strip * VIDEO_STRIP_ROWS + y) - 120;
                    if (dx * dx + dy * dy <= 122 * 122)
                        assert(x >= span.x && x < span.x + span.width);
                }
                for (unsigned x = 0; x < span.width; ++x)
                    assert(storage[1 + y * span.width + x] ==
                           gyrus_palette[frame[(strip * VIDEO_STRIP_ROWS + y) * GAME_WIDTH +
                                               span.x + x]]);
            }
            assert(video_expand_strip(frame, strip, full, storage, 1) == 0);
        }
    }
    assert(optimized_pixels < 50000);
    assert(video_strip_span(VIDEO_STRIPS, false).width == 0);
    assert(video_expand_strip(frame, VIDEO_STRIPS, false, storage, VIDEO_STRIP_PIXELS) == 0);
    return 0;
}
