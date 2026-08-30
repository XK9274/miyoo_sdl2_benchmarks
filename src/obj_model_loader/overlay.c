#include "obj_model_loader/overlay.h"

#include <float.h>

#include "common/overlay_grid.h"

void obj_overlay_submit(BenchOverlay *overlay, const ObjModelLoaderState *state, const BenchMetrics *metrics)
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
    overlay_grid_init(&grid, 2, 10);
    overlay_grid_set_background(&grid, (SDL_Color){0, 0, 0, 210});

    overlay_grid_set_cell(&grid, 0, 0, accent, 1, "Obj Model Loader");
    overlay_grid_set_cell(&grid, 0, 1, accent, 1, "Control Scheme");

    overlay_grid_set_cell(&grid, 1, 0, primary, 0, "Model: %s (%d/%d)", state->model_label,
                        state->available_model_count > 0 ? state->current_model_index + 1 : 0,
                        state->available_model_count);
    overlay_grid_set_cell(&grid, 1, 1, primary, 0, "D-Pad - Orbit Camera");

    overlay_grid_set_cell(&grid, 2, 0, primary, 0,
                        "FPS %.1f (min %.1f / max %.1f / avg %.1f)",
                        metrics->current_fps,
                        (metrics->min_fps == DBL_MAX) ? 0.0 : metrics->min_fps,
                        metrics->max_fps,
                        metrics->avg_fps);
    overlay_grid_set_cell(&grid, 2, 1, primary, 0, "L1/R1 - Zoom In/Out");

    overlay_grid_set_cell(&grid, 3, 0, cyan, 0,
                        "Frame %.3fms (min %.3f / max %.3f)",
                        metrics->frame_time_ms,
                        (metrics->min_frame_time_ms == DBL_MAX) ? 0.0 : metrics->min_frame_time_ms,
                        metrics->max_frame_time_ms);
    overlay_grid_set_cell(&grid, 3, 1, primary, 0, "A - Toggle Auto-Rotate");

    overlay_grid_set_cell(&grid, 4, 0, green, 0,
                        "Draw Calls %llu | Tris %llu | Verts %llu",
                        (unsigned long long)metrics->draw_calls,
                        (unsigned long long)metrics->triangles_rendered,
                        (unsigned long long)metrics->vertices_rendered);
    overlay_grid_set_cell(&grid, 4, 1, primary, 0, "B - Toggle Wireframe");

    overlay_grid_set_cell(&grid, 5, 0, green, 0,
                        "Texture Switches %llu | Auto-Rotate %s | Wireframe %s",
                        (unsigned long long)metrics->texture_switches,
                        state->auto_rotate ? "ON" : "OFF",
                        state->wireframe ? "ON" : "OFF");
    overlay_grid_set_cell(&grid, 5, 1, primary, 0, "SELECT - Reset Metrics");

    overlay_grid_set_cell(&grid, 6, 0, info, 0, "L2/R2 - Previous/Next Model");
    overlay_grid_set_cell(&grid, 6, 1, info, 0, "START - Input Mode  V - Vsync");

    overlay_grid_submit_to_overlay(&grid, overlay);
}
