#ifndef SCALING_BENCH_RENDER_H
#define SCALING_BENCH_RENDER_H

#include "scaling_bench/state.h"

void scaling_render_init(ScalingBenchState *state, SDL_Renderer *renderer);
void scaling_render_cleanup(ScalingBenchState *state);

void scaling_render_scene(ScalingBenchState *state,
                          SDL_Renderer *renderer,
                          BenchMetrics *metrics,
                          double delta_seconds);

#endif /* SCALING_BENCH_RENDER_H */
