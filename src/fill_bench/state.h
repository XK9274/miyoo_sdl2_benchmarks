#ifndef FILL_BENCH_STATE_H
#define FILL_BENCH_STATE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "bench_common.h"
#include "common/bench_stress.h"

typedef struct {
    int stress_level;
    float top_margin;
    TTF_Font *font;

    BenchSinTable sin_table;

    float fill_phase_units;
} FillBenchState;

void fill_state_init(FillBenchState *state);
void fill_state_update_layout(FillBenchState *state, BenchOverlay *overlay);
void fill_state_destroy(FillBenchState *state, SDL_Renderer *renderer);

float fill_state_stress_factor(const FillBenchState *state);
float fill_state_sin(const FillBenchState *state, float units);

#endif /* FILL_BENCH_STATE_H */
