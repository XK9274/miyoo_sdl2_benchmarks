#ifndef RENDER_SUITE_SCENES_GEOMETRY_H
#define RENDER_SUITE_SCENES_GEOMETRY_H

#include <SDL2/SDL.h>

#include "bench_common.h"
#include "render_suite/state.h"

void rs_scene_geometry(RenderSuiteState *state,
                       SDL_Renderer *renderer,
                       BenchMetrics *metrics,
                       double delta_seconds);

#endif /* RENDER_SUITE_SCENES_GEOMETRY_H */