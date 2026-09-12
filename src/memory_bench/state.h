#ifndef MEMORY_BENCH_STATE_H
#define MEMORY_BENCH_STATE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "bench_common.h"

typedef struct {
    int stress_level;
    float top_margin;
    TTF_Font *font;

    float resources_phase;
    float mode_phase_seconds;

    int forced_pattern_mode;  /* -1 = auto-cycle, else locked mode index */
    int current_pattern_mode;
    int forced_alloc_mode;    /* -1 = auto-cycle, else locked mode index */
    int current_alloc_mode;
    SDL_bool neon_enabled;
} MemoryBenchState;

void memory_state_init(MemoryBenchState *state);
void memory_state_update_layout(MemoryBenchState *state, BenchOverlay *overlay);
void memory_state_destroy(MemoryBenchState *state, SDL_Renderer *renderer);

float memory_state_stress_factor(const MemoryBenchState *state);

#endif /* MEMORY_BENCH_STATE_H */
