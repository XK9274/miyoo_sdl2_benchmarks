#include "memory_bench/state.h"

#include "common/bench_stress.h"

void memory_state_init(MemoryBenchState *state)
{
    if (!state) {
        return;
    }
    SDL_memset(state, 0, sizeof(*state));
    state->stress_level = 1;
    state->top_margin = 0.0f;
}

void memory_state_update_layout(MemoryBenchState *state, BenchOverlay *overlay)
{
    if (!state) {
        return;
    }
    state->top_margin = overlay ? (float)bench_overlay_height(overlay) : 0.0f;
}

void memory_state_destroy(MemoryBenchState *state, SDL_Renderer *renderer)
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

float memory_state_stress_factor(const MemoryBenchState *state)
{
    if (!state) {
        return 1.0f;
    }
    return bench_stress_factor(state->stress_level);
}
