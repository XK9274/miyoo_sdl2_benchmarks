#include "texture_bench/state.h"

#include "common/bench_stress.h"

void texture_state_init(TextureBenchState *state)
{
    if (!state) {
        return;
    }
    SDL_memset(state, 0, sizeof(*state));
    state->stress_level = 1;
    state->top_margin = 0.0f;
    state->texture_streaming = SDL_TRUE;
    state->texture_blend_variety = SDL_TRUE;
    state->texture_transform_variety = SDL_TRUE;
}

void texture_state_update_layout(TextureBenchState *state, BenchOverlay *overlay)
{
    if (!state) {
        return;
    }
    state->top_margin = overlay ? (float)bench_overlay_height(overlay) : 0.0f;
}

void texture_state_destroy(TextureBenchState *state, SDL_Renderer *renderer)
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

float texture_state_stress_factor(const TextureBenchState *state)
{
    if (!state) {
        return 1.0f;
    }
    return bench_stress_factor(state->stress_level);
}
