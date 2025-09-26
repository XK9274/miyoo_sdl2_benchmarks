#include "double_buf/overlay.h"

#include <float.h>

#include "common/overlay_grid.h"

void db_overlay_submit(BenchOverlay *overlay,
                       const DoubleBenchState *state,
                       const BenchMetrics *metrics)
{
    if (!overlay || !state || !metrics) {
        return;
    }

    const SDL_Color accent = {255, 215, 0, 255};    // Gold for headers
    const SDL_Color primary = {255, 255, 255, 255}; // White for main content
    const SDL_Color cyan = {0, 200, 255, 255};      // Cyan for technical specs
    const SDL_Color green = {0, 255, 160, 255};     // Green for status info
    const SDL_Color amber = {255, 180, 120, 255};   // Amber for state info
    const SDL_Color info = {255, 200, 0, 255};      // Info yellow for controls

    OverlayGrid grid;
    overlay_grid_init(&grid, 2, 10);
    overlay_grid_set_background(&grid, (SDL_Color){0, 0, 0, 210});

    // Row 0 - Headers
    overlay_grid_set_cell(&grid, 0, 0, accent, 1, "SDL2 Hardware Double Buffer");
    overlay_grid_set_cell(&grid, 0, 1, accent, 1, "Control Scheme");

    // Row 1 - Particle status left, first control right
    overlay_grid_set_cell(&grid, 1, 0, primary, 0,
                        "Particles %d/%d | Cube %s | Grid %s",
                        state->particle_count, DB_MAX_PARTICLES,
                        state->show_cube ? "ON" : "OFF",
                        state->backdrop_grid ? "ON" : "OFF");
    overlay_grid_set_cell(&grid, 1, 1, primary, 0, "A/B - +/-150 Particles");

    // Row 2 - FPS metrics left, control right
    overlay_grid_set_cell(&grid, 2, 0, primary, 0,
                        "FPS %.1f (min %.1f / max %.1f / avg %.1f)",
                        metrics->current_fps,
                        (metrics->min_fps == DBL_MAX) ? 0.0 : metrics->min_fps,
                        metrics->max_fps,
                        metrics->avg_fps);
    overlay_grid_set_cell(&grid, 2, 1, primary, 0, "L1 - Toggle Backdrop");

    // Row 3 - Frame timing left, control right
    overlay_grid_set_cell(&grid, 3, 0, cyan, 0,
                        "Frame %.3fms (min %.3f / max %.3f)",
                        metrics->frame_time_ms,
                        (metrics->min_frame_time_ms == DBL_MAX) ? 0.0 : metrics->min_frame_time_ms,
                        metrics->max_frame_time_ms);
    overlay_grid_set_cell(&grid, 3, 1, primary, 0, "R1 - Toggle Cube");

    // Row 4 - Draw call stats left, control right
    overlay_grid_set_cell(&grid, 4, 0, green, 0,
                        "Draw Calls %llu | Vertices %llu | Triangles %llu",
                        (unsigned long long)metrics->draw_calls,
                        (unsigned long long)metrics->vertices_rendered,
                        (unsigned long long)metrics->triangles_rendered);
    overlay_grid_set_cell(&grid, 4, 1, primary, 0, "L2/R2 - Particle Speed");

    // Row 5 - Particle state left, control right
    overlay_grid_set_cell(&grid, 5, 0, amber, 0,
                        "Particle Speed %.0f | Render Mode %d",
                        state->particle_speed,
                        state->render_mode);
    overlay_grid_set_cell(&grid, 5, 1, primary, 0, "X - Change Render Mode");

    // Row 6 - Render type left, control right
    overlay_grid_set_cell(&grid, 6, 0, primary, 0, "Single-threaded Hardware Rendering");
    overlay_grid_set_cell(&grid, 6, 1, primary, 0, "Y - Toggle Particles");

    // Row 7 - Empty left, control right
    overlay_grid_set_cell(&grid, 7, 0, primary, 0, "");
    overlay_grid_set_cell(&grid, 7, 1, info, 0, "SELECT - Reset Metrics");

    // Row 8 - Empty left, exit control right
    overlay_grid_set_cell(&grid, 8, 0, primary, 0, "");
    overlay_grid_set_cell(&grid, 8, 1, info, 0, "START/ESC - Exit");

    // Row 9 remains empty for future expansion

    overlay_grid_submit_to_overlay(&grid, overlay);
}
