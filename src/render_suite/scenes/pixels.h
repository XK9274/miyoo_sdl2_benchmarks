#ifndef RENDER_SUITE_SCENES_PIXELS_H
#define RENDER_SUITE_SCENES_PIXELS_H

#include <SDL2/SDL.h>

#include "bench_common.h"
#include "render_suite/state.h"

void rs_scene_pixels(RenderSuiteState *state,
                     SDL_Renderer *renderer,
                     BenchMetrics *metrics,
                     double delta_seconds);

void rs_scene_pixels_init(RenderSuiteState *state, SDL_Renderer *renderer);
void rs_scene_pixels_cleanup(RenderSuiteState *state);

#endif /* RENDER_SUITE_SCENES_PIXELS_H */