#ifndef COMMON_OVERLAY_DEBUG_STATS_H
#define COMMON_OVERLAY_DEBUG_STATS_H

#include <SDL2/SDL.h>

typedef struct {
    double fps;
    double cmdqueue_ms;
    double present_ms;
    double blits;
    double fill_ms;
    double copy_ms;
    double geometry_ms;
    double lines_ms;
    double misc_ms;

    Uint64 triangles;
    Uint64 spans;
    Uint64 span_pixels;
    Uint32 max_span_width;
    Uint32 max_span_height;

    SDL_bool have_timing;
    SDL_bool have_geometry;
} OverlayDebugStats;

/* Sets SDL_MMIYOO_FRAME_TIMING/SDL_MMIYOO_GEOMETRY_STATS hints -- call once
 * before SDL_CreateRenderer. Only compiled into DEBUG=1 builds. */
void overlay_debug_stats_enable_hints(void);

/* Fills out from the sdl2_miyoo stats API for the given renderer. */
void overlay_debug_stats_poll(SDL_Renderer *renderer, OverlayDebugStats *out);

#endif /* COMMON_OVERLAY_DEBUG_STATS_H */
