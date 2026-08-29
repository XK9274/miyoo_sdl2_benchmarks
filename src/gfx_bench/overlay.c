#include "gfx_bench/overlay.h"

#include <float.h>

#include "common/overlay_grid.h"

void gb_overlay_submit(BenchOverlay *overlay,
                       const GfxBenchState *state,
                       const BenchMetrics *metrics)
{
    if (!overlay || !state || !metrics) {
        return;
    }

    static const char *scene_names[GB_SCENE_MAX] = {
        "AA Shapes",
        "Rounded Rects",
        "Polygons",
        "Bezier Curves",
        "Thick Lines",
    };

    const SDL_Color accent = {255, 215, 0, 255};    // Gold for headers
    const SDL_Color primary = {255, 255, 255, 255}; // White for main content
    const SDL_Color cyan = {0, 200, 255, 255};      // Cyan for technical specs
    const SDL_Color green = {0, 255, 160, 255};     // Green for status info
    const SDL_Color info = {255, 200, 0, 255};      // Info yellow for controls

    OverlayGrid grid;
    overlay_grid_init(&grid, 2, 10);
    overlay_grid_set_background(&grid, (SDL_Color){0, 0, 0, 210});

    // Row 0 - Headers
    overlay_grid_set_cell(&grid, 0, 0, accent, 1, "SDL2_gfx Bench");
    overlay_grid_set_cell(&grid, 0, 1, accent, 1, "Control Scheme");

    // Row 1 - Scene info left, first control right
    overlay_grid_set_cell(&grid, 1, 0, primary, 0,
                        "Scene: %s | Auto %s | Stress L%d x%.1f",
                        scene_names[state->active_scene],
                        state->auto_cycle ? "ON" : "OFF",
                        state->stress_level,
                        gb_state_stress_factor(state));
    overlay_grid_set_cell(&grid, 1, 1, primary, 0, "L2/R2 - Switch Scene");

    // Row 2 - FPS metrics left, control right
    overlay_grid_set_cell(&grid, 2, 0, primary, 0,
                        "FPS %.1f (min %.1f / max %.1f / avg %.1f)",
                        metrics->current_fps,
                        (metrics->min_fps == DBL_MAX) ? 0.0 : metrics->min_fps,
                        metrics->max_fps,
                        metrics->avg_fps);
    overlay_grid_set_cell(&grid, 2, 1, primary, 0, "A - Toggle Auto Cycle");

    // Row 3 - Frame timing left, control right
    overlay_grid_set_cell(&grid, 3, 0, cyan, 0,
                        "Frame %.3fms (min %.3f / max %.3f)",
                        metrics->frame_time_ms,
                        (metrics->min_frame_time_ms == DBL_MAX) ? 0.0 : metrics->min_frame_time_ms,
                        metrics->max_frame_time_ms);
    overlay_grid_set_cell(&grid, 3, 1, primary, 0, "B - Adjust Stress Level");

    // Row 4 - Primitive call stats left, control right
    overlay_grid_set_cell(&grid, 4, 0, green, 0,
                        "Primitive Calls %llu",
                        (unsigned long long)metrics->draw_calls);
    overlay_grid_set_cell(&grid, 4, 1, primary, 0, "SELECT - Reset Metrics");

    overlay_grid_submit_to_overlay(&grid, overlay);
}
