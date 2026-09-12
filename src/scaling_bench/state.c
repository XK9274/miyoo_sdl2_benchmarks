#include "scaling_bench/state.h"

void scaling_state_init(ScalingBenchState *state)
{
    if (!state) {
        return;
    }
    SDL_memset(state, 0, sizeof(*state));
    state->stress_level = 1;
    state->top_margin = 0.0f;
    state->forced_content_mode = -1;
    state->forced_scaling_mode = -1;
    state->neon_enabled = SDL_TRUE;
    bench_sin_table_init(&state->sin_table);
}

void scaling_state_update_layout(ScalingBenchState *state, BenchOverlay *overlay)
{
    if (!state) {
        return;
    }
    state->top_margin = overlay ? (float)bench_overlay_height(overlay) : 0.0f;
}

void scaling_state_destroy(ScalingBenchState *state, SDL_Renderer *renderer)
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

float scaling_state_stress_factor(const ScalingBenchState *state)
{
    if (!state) {
        return 1.0f;
    }
    return bench_stress_factor(state->stress_level);
}

float scaling_state_sin_rad(const ScalingBenchState *state, float radians)
{
    if (!state) {
        return 0.0f;
    }
    return bench_sin_table_sin_rad(&state->sin_table, radians);
}

float scaling_state_cos_rad(const ScalingBenchState *state, float radians)
{
    if (!state) {
        return 0.0f;
    }
    return bench_sin_table_cos_rad(&state->sin_table, radians);
}
