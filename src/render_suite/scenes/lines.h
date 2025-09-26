#ifndef RENDER_SUITE_SCENES_LINES_H
#define RENDER_SUITE_SCENES_LINES_H

#include "render_suite/state.h"

void rs_scene_lines(RenderSuiteState *state,
                    SDL_Renderer *renderer,
                    BenchMetrics *metrics,
                    double time_seconds);

#endif /* RENDER_SUITE_SCENES_LINES_H */
