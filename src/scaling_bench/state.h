#ifndef SCALING_BENCH_STATE_H
#define SCALING_BENCH_STATE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "bench_common.h"
#include "common/bench_stress.h"

typedef struct {
    int stress_level;
    float top_margin;
    TTF_Font *font;

    BenchSinTable sin_table;

    int scaling_current_width;
    int scaling_current_height;
    float scaling_phase;
    SDL_Texture **scaling_targets;
    int scaling_target_count;
} ScalingBenchState;

void scaling_state_init(ScalingBenchState *state);
void scaling_state_update_layout(ScalingBenchState *state, BenchOverlay *overlay);
void scaling_state_destroy(ScalingBenchState *state, SDL_Renderer *renderer);

float scaling_state_stress_factor(const ScalingBenchState *state);
float scaling_state_sin_rad(const ScalingBenchState *state, float radians);
float scaling_state_cos_rad(const ScalingBenchState *state, float radians);

#endif /* SCALING_BENCH_STATE_H */
