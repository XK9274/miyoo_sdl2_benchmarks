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
    SDL_Texture **resource_textures;
    int resource_texture_count;
    int resource_allocation_index;
} MemoryBenchState;

void memory_state_init(MemoryBenchState *state);
void memory_state_update_layout(MemoryBenchState *state, BenchOverlay *overlay);
void memory_state_destroy(MemoryBenchState *state, SDL_Renderer *renderer);

float memory_state_stress_factor(const MemoryBenchState *state);

#endif /* MEMORY_BENCH_STATE_H */
