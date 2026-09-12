#ifndef TEXTURE_BENCH_STATE_H
#define TEXTURE_BENCH_STATE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "bench_common.h"

typedef struct {
    int stress_level;
    float top_margin;
    TTF_Font *font;

    SDL_bool texture_streaming;
    SDL_bool texture_blend_variety;
    SDL_bool texture_transform_variety;
} TextureBenchState;

void texture_state_init(TextureBenchState *state);
void texture_state_update_layout(TextureBenchState *state, BenchOverlay *overlay);
void texture_state_destroy(TextureBenchState *state, SDL_Renderer *renderer);

float texture_state_stress_factor(const TextureBenchState *state);

#endif /* TEXTURE_BENCH_STATE_H */
