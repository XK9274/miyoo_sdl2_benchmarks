#ifndef RENDER_SUITE_SCENES_LINES_H
#define RENDER_SUITE_SCENES_LINES_H

#include "render_suite/state.h"

void rs_scene_lines(RenderSuiteState *state,
                    SDL_Renderer *renderer,
                    BenchMetrics *metrics,
                    double delta_seconds);

void rs_scene_lines_init(RenderSuiteState *state, SDL_Renderer *renderer);
void rs_scene_lines_cleanup(RenderSuiteState *state);

int rs_scene_lines_cube_count(void);
int rs_scene_lines_anomaly_count(void);

#endif /* RENDER_SUITE_SCENES_LINES_H */
