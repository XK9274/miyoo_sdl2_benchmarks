#ifndef RENDER_SUITE_GL_SCENES_EFFECTS_H
#define RENDER_SUITE_GL_SCENES_EFFECTS_H

#include <SDL2/SDL.h>

#include "bench_common.h"
#include "render_suite_gl/state.h"

SDL_bool rsgl_effects_init(RsglState *state, SDL_Renderer *renderer);
void rsgl_effects_render(RsglState *state,
                         SDL_Renderer *renderer,
                         BenchMetrics *metrics,
                         double delta_seconds);
void rsgl_effects_warmup(RsglState *state);
SDL_bool rsgl_effects_apply_fbo_size(RsglState *state, SDL_Renderer *renderer);
void rsgl_effects_cleanup(RsglState *state);
int rsgl_effect_count(void);
const char *rsgl_effect_name(int index);

#endif /* RENDER_SUITE_GL_SCENES_EFFECTS_H */
