#ifndef MEMORY_BENCH_RENDER_H
#define MEMORY_BENCH_RENDER_H

#include "memory_bench/state.h"

void memory_render_init(MemoryBenchState *state, SDL_Renderer *renderer);
void memory_render_cleanup(MemoryBenchState *state);

void memory_render_scene(MemoryBenchState *state,
                         SDL_Renderer *renderer,
                         BenchMetrics *metrics,
                         double delta_seconds);

#endif /* MEMORY_BENCH_RENDER_H */
