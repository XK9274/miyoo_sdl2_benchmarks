#ifndef SPACE_BENCH_RENDER_H
#define SPACE_BENCH_RENDER_H

#include "space_bench/state.h"

void space_render_scene(SpaceBenchState *state,
                        SDL_Renderer *renderer,
                        BenchMetrics *metrics);

#endif /* SPACE_BENCH_RENDER_H */
