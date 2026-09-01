#include "common/overlay_debug_stats.h"

#include <SDL2/SDL_mmiyoo_stats.h>

void overlay_debug_stats_enable_hints(void)
{
    SDL_SetHint("SDL_MMIYOO_FRAME_TIMING", "1");
    SDL_SetHint("SDL_MMIYOO_GEOMETRY_STATS", "1");
}

void overlay_debug_stats_poll(SDL_Renderer *renderer, OverlayDebugStats *out)
{
    if (!out) {
        return;
    }
    SDL_zerop(out);
    if (!renderer) {
        return;
    }

    SDL_MMIYOO_FrameTimingStats timing;
    if (SDL_MMIYOO_GetFrameTimingStats(renderer, &timing)) {
        out->have_timing = SDL_TRUE;
        out->fps = timing.fps;
        out->cmdqueue_ms = timing.cmdqueue_ms_per_frame;
        out->present_ms = timing.present_ms_per_frame;
        out->blits = timing.blits_per_frame;
        out->fill_ms = timing.fill_ms_per_frame;
        out->copy_ms = timing.copy_ms_per_frame;
        out->geometry_ms = timing.geometry_ms_per_frame;
        out->lines_ms = timing.lines_ms_per_frame;
        out->misc_ms = timing.misc_ms_per_frame;
    }

    SDL_MMIYOO_GeometryStats geometry;
    if (SDL_MMIYOO_GetGeometryStats(renderer, &geometry)) {
        out->have_geometry = SDL_TRUE;
        out->triangles = geometry.triangles;
        out->spans = geometry.spans;
        out->span_pixels = geometry.span_pixels;
        out->max_span_width = geometry.max_span_width;
        out->max_span_height = geometry.max_span_height;
    }
}
