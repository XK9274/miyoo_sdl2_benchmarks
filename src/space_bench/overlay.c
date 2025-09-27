#include "space_bench/overlay.h"

#include <float.h>

#include "common/overlay_grid.h"

void space_overlay_submit(BenchOverlay *overlay,
                          const SpaceBenchState *state,
                          const BenchMetrics *metrics)
{
    if (!overlay || !state) {
        return;
    }

    const SDL_Color title = {210, 235, 255, 255};
    const SDL_Color info = {180, 210, 255, 255};
    const SDL_Color accent = {255, 200, 120, 255};
    const SDL_Color cyan = {160, 220, 255, 255};

    OverlayGrid grid;
    overlay_grid_init(&grid, 2, 10);
    overlay_grid_set_background(&grid, (SDL_Color){0, 0, 0, 196});

    overlay_grid_set_cell(&grid, 0, 0, title, 0, "Interactive space bench");
    overlay_grid_set_cell(&grid, 0, 1, info, 0, "START/ESC - Exit");

    overlay_grid_set_cell(&grid, 1, 0, info, 0,
                          "Score %d | Enemies %d | Missed %d",
                          state->score,
                          state->total_enemies_killed,
                          state->player_hits);

    const char *guidance = state->weapon_upgrades.guidance_active ? "ON" : "--";
    const char *thumper = state->weapon_upgrades.thumper_active ? "ON" : "--";
    char shield_status[16] = "--";
    if (state->shield_active && state->shield_strength > 0.0f) {
        SDL_snprintf(shield_status, sizeof(shield_status), "%.0f", state->shield_strength);
    }

    char laser_status[16] = "READY";
    if (state->laser_recharge_timer > 0.0f) {
        SDL_snprintf(laser_status, sizeof(laser_status), "%.0fs", state->laser_recharge_timer);
    } else if (state->laser_hold_timer > 0.0f) {
        SDL_snprintf(laser_status, sizeof(laser_status), "%.1fs", state->laser_hold_timer);
    }

    overlay_grid_set_cell(&grid, 1, 1, info, 0,
                          "HP %.0f/%.0f | Shield %s | Split %d | G %s | Drones %d | Laser %s | Thumper %s",
                          state->player_health,
                          state->player_max_health,
                          shield_status,
                          state->weapon_upgrades.split_level,
                          guidance,
                          state->weapon_upgrades.drone_count,
                          laser_status,
                          thumper);

    if (metrics) {
        overlay_grid_set_cell(&grid, 2, 1, info, 0,
                              "FPS %.1f | Frame %.2f ms",
                              metrics->current_fps,
                              metrics->frame_time_ms);
        overlay_grid_set_cell(&grid, 3, 1, cyan, 0,
                              "Frame min %.2f / max %.2f",
                              (metrics->min_frame_time_ms == DBL_MAX) ? 0.0 : metrics->min_frame_time_ms,
                              metrics->max_frame_time_ms);
    } else {
        overlay_grid_set_cell(&grid, 2, 1, info, 0, "FPS -- | Frame --");
        overlay_grid_set_cell(&grid, 3, 1, cyan, 0, "Frame min -- / max --");
    }

    overlay_grid_set_cell(&grid, 2, 0, accent, 0,
                          "Scroll %.0f u/s | Spawn %.2f s | Beam x%.1f",
                          state->scroll_speed,
                          state->spawn_interval,
                          state->weapon_upgrades.beam_scale);

    overlay_grid_set_cell(&grid, 3, 0, info, 0,
                          "Enemies killed: %d | Total score: %d",
                          state->total_enemies_killed,
                          state->score);

    overlay_grid_set_cell(&grid, 4, 0, info, 0,
                          "Controls: D-PAD Move | A Gun | B Laser");
    overlay_grid_set_cell(&grid, 4, 1, accent, 0,
                          "Lasers pierce harder with beam focus");

    overlay_grid_set_cell(&grid, 5, 0, info, 0,
                          "Scroll target %.0f | Spawn min %.2f",
                          state->target_scroll_speed,
                          state->min_spawn_interval);
    overlay_grid_set_cell(&grid, 5, 1, info, 0,
                          "SELECT - Reset Metrics");

    overlay_grid_submit_to_overlay(&grid, overlay);
}
