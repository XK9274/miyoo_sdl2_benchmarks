#include "lines_bench/state.h"

#include "common/bench_stress.h"

void lines_state_init(LinesBenchState *state)
{
    if (!state) {
        return;
    }
    SDL_memset(state, 0, sizeof(*state));
    state->stress_level = 1;
    state->top_margin = 0.0f;
    state->lines_grid_n = -1;
    state->lines_anomalies_visible = SDL_TRUE;
    state->lines_wireframe = SDL_FALSE;
    state->lines_backface_cull = SDL_TRUE;
}

void lines_state_update_layout(LinesBenchState *state, BenchOverlay *overlay)
{
    if (!state) {
        return;
    }
    state->top_margin = overlay ? (float)bench_overlay_height(overlay) : 0.0f;
}

void lines_state_destroy(LinesBenchState *state, SDL_Renderer *renderer)
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

float lines_state_stress_factor(const LinesBenchState *state)
{
    if (!state) {
        return 1.0f;
    }
    return bench_stress_factor(state->stress_level);
}
