#include "geometry_bench/state.h"

void geometry_state_init(GeometryBenchState *state)
{
    if (!state) {
        return;
    }
    SDL_memset(state, 0, sizeof(*state));
    state->stress_level = 1;
    state->top_margin = 0.0f;
    bench_sin_table_init(&state->sin_table);
    state->geometry_render_mode = GEOMETRY_RENDER_FILLED;
}

void geometry_state_update_layout(GeometryBenchState *state, BenchOverlay *overlay)
{
    if (!state) {
        return;
    }
    state->top_margin = overlay ? (float)bench_overlay_height(overlay) : 0.0f;
}

void geometry_state_destroy(GeometryBenchState *state, SDL_Renderer *renderer)
{
    if (!state) {
        return;
    }
    if (state->font) {
        TTF_CloseFont(state->font);
        state->font = NULL;
    }
    (void)renderer;
}

float geometry_state_stress_factor(const GeometryBenchState *state)
{
    if (!state) {
        return 1.0f;
    }
    return bench_stress_factor(state->stress_level);
}

float geometry_state_sin(const GeometryBenchState *state, float units)
{
    if (!state) {
        return 0.0f;
    }
    return bench_sin_table_sin(&state->sin_table, units);
}

float geometry_state_sin_rad(const GeometryBenchState *state, float radians)
{
    if (!state) {
        return 0.0f;
    }
    return bench_sin_table_sin_rad(&state->sin_table, radians);
}

float geometry_state_cos_rad(const GeometryBenchState *state, float radians)
{
    if (!state) {
        return 0.0f;
    }
    return bench_sin_table_cos_rad(&state->sin_table, radians);
}
