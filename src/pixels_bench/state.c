#include "pixels_bench/state.h"

void pixels_state_init(PixelsBenchState *state)
{
    if (!state) {
        return;
    }
    SDL_memset(state, 0, sizeof(*state));
    state->stress_level = 1;
    state->top_margin = 0.0f;
    state->forced_mode = -1;
    state->neon_copy_enabled = SDL_TRUE;
    state->blend_mode = PIXELS_BLEND_ALPHA;
    state->upload_path = PIXELS_UPLOAD_LOCK;
    bench_sin_table_init(&state->sin_table);
}

void pixels_state_update_layout(PixelsBenchState *state, BenchOverlay *overlay)
{
    if (!state) {
        return;
    }
    state->top_margin = overlay ? (float)bench_overlay_height(overlay) : 0.0f;
}

void pixels_state_destroy(PixelsBenchState *state, SDL_Renderer *renderer)
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

float pixels_state_stress_factor(const PixelsBenchState *state)
{
    if (!state) {
        return 1.0f;
    }
    return bench_stress_factor(state->stress_level);
}

float pixels_state_sin(const PixelsBenchState *state, float units)
{
    return bench_sin_table_sin(&state->sin_table, units);
}

float pixels_state_cos(const PixelsBenchState *state, float units)
{
    return bench_sin_table_cos(&state->sin_table, units);
}

float pixels_state_sin_rad(const PixelsBenchState *state, float radians)
{
    return bench_sin_table_sin_rad(&state->sin_table, radians);
}

float pixels_state_cos_rad(const PixelsBenchState *state, float radians)
{
    return bench_sin_table_cos_rad(&state->sin_table, radians);
}
