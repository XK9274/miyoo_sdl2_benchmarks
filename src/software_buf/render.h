#ifndef SOFTWARE_BUF_RENDER_H
#define SOFTWARE_BUF_RENDER_H

#include <SDL2/SDL.h>

#include "software_buf/state.h"

void sb_render_scene(SDL_Renderer *renderer,
                     SDL_Texture *backbuffer,
                     SoftwareBenchState *state,
                     BenchMetrics *metrics);

#endif /* SOFTWARE_BUF_RENDER_H */
