#ifndef GYRUS_VIDEO_H
#define GYRUS_VIDEO_H
#include "engine/video.h"
#include "gyrus/game.h"
/* Compatibility expansion using the original Gyrus palette. */
size_t video_expand_strip(const uint8_t *, unsigned, bool, uint16_t *, size_t);
#endif
