#ifndef FILL_BENCH_RENDER_H
#define FILL_BENCH_RENDER_H

#include "fill_bench/state.h"

void fill_render_scene(FillBenchState *state,
                       SDL_Renderer *renderer,
                       BenchMetrics *metrics,
                       double delta_seconds);

#endif /* FILL_BENCH_RENDER_H */
