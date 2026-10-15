#ifndef ENGINE_DEBUG_SESSION_H
#define ENGINE_DEBUG_SESSION_H
#include "engine/session.h"
#include "gyrus/debug.h"
typedef struct {
    engine_state *engine;
    uint8_t *framebuffer;
    void (*write)(void *, const void *, size_t);
    void *write_context;
} engine_debug_session;
void engine_debug_session_init(engine_debug_session *, engine_state *, uint8_t *,
                               void (*write)(void *, const void *, size_t), void *);
gyrus_debug_callbacks engine_debug_session_callbacks(engine_debug_session *);
#endif
