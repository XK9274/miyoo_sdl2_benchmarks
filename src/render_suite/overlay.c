#include "render_suite/overlay.h"

#include <float.h>

#include "common/overlay_grid.h"

void rs_overlay_submit(BenchOverlay *overlay,
                       const RenderSuiteState *state,
                       const BenchMetrics *metrics)
{
    if (!overlay || !state || !metrics) {
        return;
    }

    static const char *scene_names[SCENE_MAX] = {
        "Fill Rate",
        "Texture",
        "Lines/Geometry"
    };

    const SDL_Color accent = {255, 215, 0, 255};    // Gold for headers
    const SDL_Color primary = {255, 255, 255, 255}; // White for main content
    const SDL_Color cyan = {0, 200, 255, 255};      // Cyan for technical specs
    const SDL_Color green = {0, 255, 160, 255};     // Green for status info
    const SDL_Color amber = {255, 180, 120, 255};   // Amber for state info
    const SDL_Color info = {255, 200, 0, 255};      // Info yellow for controls

    OverlayGrid grid;
    overlay_grid_init(&grid, 2, 10);  // 2 columns, 10 rows (standardized)
    overlay_grid_set_background(&grid, (SDL_Color){0, 0, 0, 210});

    // Row 0 - Headers (centered)
    overlay_grid_set_cell(&grid, 0, 0, accent, 1, "SDL2 Render Suite");
    overlay_grid_set_cell(&grid, 0, 1, accent, 1, "Control Scheme");

    // Row 1 - Scene info left, first control right
    overlay_grid_set_cell(&grid, 1, 0, primary, 0,
                        "Scene: %s | Auto %s | Stress L%d x%.1f",
                        scene_names[state->active_scene],
                        state->auto_cycle ? "ON" : "OFF",
                        state->stress_level,
                        rs_state_stress_factor(state));
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

    // Row 4 - Draw call stats left, control right
    overlay_grid_set_cell(&grid, 4, 0, green, 0,
                        "Draw Calls %llu | Vertices %llu | Triangles %llu",
                        (unsigned long long)metrics->draw_calls,
                        (unsigned long long)metrics->vertices_rendered,
                        (unsigned long long)metrics->triangles_rendered);
    overlay_grid_set_cell(&grid, 4, 1, primary, 0, "X - Reset Metrics");

    // Row 5 - Render type left, exit control right
    overlay_grid_set_cell(&grid, 5, 0, amber, 0, "Single-threaded Hardware Rendering");
    overlay_grid_set_cell(&grid, 5, 1, info, 0, "START/ESC - Exit");

    // Rows 6-9 remain empty for future expansion

    overlay_grid_submit_to_overlay(&grid, overlay);
}
