#include "render_suite_gl/overlay.h"

#include <float.h>

#include "common/overlay_grid.h"
#include "render_suite_gl/scenes/effects.h"

void rsgl_overlay_submit(BenchOverlay *overlay,
                         const RsglState *state,
                         const BenchMetrics *metrics)
{
    if (!overlay || !state || !metrics) {
        return;
    }

    const SDL_Color accent = {255, 215, 0, 255};
    const SDL_Color primary = {255, 255, 255, 255};
    const SDL_Color cyan = {0, 200, 255, 255};
    const SDL_Color green = {0, 255, 160, 255};
    const SDL_Color info = {255, 200, 0, 255};

    OverlayGrid grid;
    overlay_grid_init(&grid, 2, 8);
    overlay_grid_set_background(&grid, (SDL_Color){0, 0, 0, 210});

    const int effect_count = state->effect_count;
    const int effect_index = (effect_count > 0) ? state->effect_index % effect_count : 0;
    const char *effect_name = rsgl_effect_name(effect_index);

    overlay_grid_set_cell(&grid, 0, 0, accent, 1, "SDL2 GL Effect Suite");
    overlay_grid_set_cell(&grid, 0, 1, accent, 1, "Controls");

    overlay_grid_set_cell(&grid, 1, 0, primary, 0,
                          "Effect: %s (%d/%d)",
                          effect_name,
                          effect_index + 1,
                          effect_count);
    overlay_grid_set_cell(&grid, 1, 1, primary, 0,
                          "Y - Next Effect");

    overlay_grid_set_cell(&grid, 2, 0, primary, 0,
                          "FPS %.1f (min %.1f / max %.1f / avg %.1f)",
                          metrics->current_fps,
                          (metrics->min_fps == DBL_MAX) ? 0.0 : metrics->min_fps,
                          metrics->max_fps,
                          metrics->avg_fps);
    overlay_grid_set_cell(&grid, 2, 1, primary, 0,
                          "A - Auto Cycle %s",
                          state->auto_cycle ? "ON" : "OFF");

    overlay_grid_set_cell(&grid, 3, 0, cyan, 0,
                          "Frame %.3fms (min %.3f / max %.3f)",
                          metrics->frame_time_ms,
                          (metrics->min_frame_time_ms == DBL_MAX) ? 0.0 : metrics->min_frame_time_ms,
                          metrics->max_frame_time_ms);
    overlay_grid_set_cell(&grid, 3, 1, primary, 0,
                          "X/SELECT - Reset Metrics");

    overlay_grid_set_cell(&grid, 4, 0, green, 0,
                          "Draw Calls %llu | Texture Updates %llu",
                          (unsigned long long)metrics->draw_calls,
                          (unsigned long long)metrics->texture_switches);

    overlay_grid_set_cell(&grid, 5, 0, info, 0,
                          "Effect Timer %.2fs",
                          state->elapsed_time);

    overlay_grid_set_cell(&grid, 6, 0, cyan, 0,
                          "Back - START/ESC");

    overlay_grid_submit_to_overlay(&grid, overlay);
}
