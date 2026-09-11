#ifndef LINES_BENCH_STATE_H
#define LINES_BENCH_STATE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "bench_common.h"

typedef struct {
    int stress_level;
    float top_margin;
    TTF_Font *font;

    float lines_rotation;
    float lines_phase;
    int lines_grid_n;
    SDL_bool lines_anomalies_visible;
    SDL_bool lines_wireframe;
    SDL_bool lines_backface_cull;
} LinesBenchState;

void lines_state_init(LinesBenchState *state);
void lines_state_update_layout(LinesBenchState *state, BenchOverlay *overlay);
void lines_state_destroy(LinesBenchState *state, SDL_Renderer *renderer);

float lines_state_stress_factor(const LinesBenchState *state);

#endif /* LINES_BENCH_STATE_H */
