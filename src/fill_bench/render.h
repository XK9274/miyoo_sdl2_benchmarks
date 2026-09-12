#ifndef FILL_BENCH_RENDER_H
#define FILL_BENCH_RENDER_H

#include "fill_bench/state.h"

typedef enum {
    FILL_PATTERN_COLUMNS = 0,
    FILL_PATTERN_ROWS,
    FILL_PATTERN_CHECKER_BLOCKS,
    FILL_PATTERN_RINGS,
    FILL_PATTERN_MAX
} FillPatternMode;

typedef enum {
    FILL_DRAW_PER_RECT = 0,
    FILL_DRAW_BATCHED,
    FILL_DRAW_MAX
} FillDrawMode;

void fill_render_scene(FillBenchState *state,
                       SDL_Renderer *renderer,
                       BenchMetrics *metrics,
                       double delta_seconds);

const char *fill_render_pattern_name(int mode);
const char *fill_render_draw_name(int mode);

#endif /* FILL_BENCH_RENDER_H */
