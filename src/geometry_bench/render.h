#ifndef GEOMETRY_BENCH_RENDER_H
#define GEOMETRY_BENCH_RENDER_H

#include "geometry_bench/state.h"

void geometry_render_init(GeometryBenchState *state, SDL_Renderer *renderer);
void geometry_render_cleanup(GeometryBenchState *state);

void geometry_render_scene(GeometryBenchState *state,
                           SDL_Renderer *renderer,
                           BenchMetrics *metrics,
                           double delta_seconds);

#endif /* GEOMETRY_BENCH_RENDER_H */
