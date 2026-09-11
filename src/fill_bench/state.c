#include "fill_bench/state.h"

void fill_state_init(FillBenchState *state)
{
    if (!state) {
        return;
    }
    SDL_memset(state, 0, sizeof(*state));
    state->stress_level = 1;
    state->top_margin = 0.0f;
    bench_sin_table_init(&state->sin_table);
    state->fill_phase_units = 0.0f;
}

void fill_state_update_layout(FillBenchState *state, BenchOverlay *overlay)
{
    if (!state) {
        return;
    }
    state->top_margin = overlay ? (float)bench_overlay_height(overlay) : 0.0f;
}

void fill_state_destroy(FillBenchState *state, SDL_Renderer *renderer)
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

float fill_state_stress_factor(const FillBenchState *state)
{
    if (!state) {
        return 1.0f;
    }
    return bench_stress_factor(state->stress_level);
}

float fill_state_sin(const FillBenchState *state, float units)
{
    if (!state) {
        return 0.0f;
    }
    return bench_sin_table_sin(&state->sin_table, units);
}
