#ifndef RENDER_SUITE_SCENES_TEXTURE_H
#define RENDER_SUITE_SCENES_TEXTURE_H

#include "render_suite/state.h"

void rs_scene_texture(RenderSuiteState *state,
                      SDL_Renderer *renderer,
                      BenchMetrics *metrics,
                      double delta_seconds);

void rs_scene_texture_init(RenderSuiteState *state, SDL_Renderer *renderer);
void rs_scene_texture_cleanup(RenderSuiteState *state);

int rs_scene_texture_instance_count(void);

#endif /* RENDER_SUITE_SCENES_TEXTURE_H */
