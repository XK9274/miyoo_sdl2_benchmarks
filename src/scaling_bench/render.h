#ifndef SCALING_BENCH_RENDER_H
#define SCALING_BENCH_RENDER_H

#include "scaling_bench/state.h"

typedef enum {
    SCALING_MODE_LOGICAL = 0,
    SCALING_MODE_VIEWPORT,
    SCALING_MODE_TEXTURE_TARGET,
    SCALING_MODE_MAX
} ScalingMode;

typedef enum {
    SCALING_CONTENT_GRADIENT = 0,
    SCALING_CONTENT_CHECKERBOARD,
    SCALING_CONTENT_NOISE,
    SCALING_CONTENT_MAX
} ScalingContentMode;

void scaling_render_init(ScalingBenchState *state, SDL_Renderer *renderer);
void scaling_render_cleanup(ScalingBenchState *state);

void scaling_render_scene(ScalingBenchState *state,
                          SDL_Renderer *renderer,
                          BenchMetrics *metrics,
                          double delta_seconds);

const char *scaling_render_mode_name(int mode);
const char *scaling_render_content_name(int mode);

#endif /* SCALING_BENCH_RENDER_H */
