#ifndef PIXELS_BENCH_RENDER_H
#define PIXELS_BENCH_RENDER_H

#include "pixels_bench/state.h"

void pixels_render_init(PixelsBenchState *state, SDL_Renderer *renderer);
void pixels_render_cleanup(PixelsBenchState *state);

void pixels_render_scene(PixelsBenchState *state,
                         SDL_Renderer *renderer,
                         BenchMetrics *metrics,
                         double delta_seconds);

#endif /* PIXELS_BENCH_RENDER_H */
