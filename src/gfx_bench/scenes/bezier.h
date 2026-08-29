#ifndef GFX_BENCH_SCENES_BEZIER_H
#define GFX_BENCH_SCENES_BEZIER_H

#include "gfx_bench/state.h"

void gb_scene_bezier(GfxBenchState *state,
                     SDL_Renderer *renderer,
                     BenchMetrics *metrics,
                     double delta_seconds);

#endif /* GFX_BENCH_SCENES_BEZIER_H */
