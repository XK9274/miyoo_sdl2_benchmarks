#ifndef MEMORY_BENCH_RENDER_H
#define MEMORY_BENCH_RENDER_H

#include "memory_bench/state.h"

typedef enum {
    MEMORY_PATTERN_GRADIENT = 0,
    MEMORY_PATTERN_CHECKERBOARD,
    MEMORY_PATTERN_PLASMA,
    MEMORY_PATTERN_NOISE,
    MEMORY_PATTERN_MAX
} MemoryPatternMode;

typedef enum {
    MEMORY_ALLOC_TEXTURE_ONLY = 0,
    MEMORY_ALLOC_MIXED,
    MEMORY_ALLOC_MALLOC_HEAVY,
    MEMORY_ALLOC_MAX
} MemoryAllocMode;

void memory_render_init(MemoryBenchState *state, SDL_Renderer *renderer);
void memory_render_cleanup(MemoryBenchState *state);

void memory_render_scene(MemoryBenchState *state,
                         SDL_Renderer *renderer,
                         BenchMetrics *metrics,
                         double delta_seconds);

const char *memory_render_pattern_name(int mode);
const char *memory_render_alloc_name(int mode);

#endif /* MEMORY_BENCH_RENDER_H */
