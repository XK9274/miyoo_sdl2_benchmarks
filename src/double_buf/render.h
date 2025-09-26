#ifndef DOUBLE_BUF_RENDER_H
#define DOUBLE_BUF_RENDER_H

#include <SDL2/SDL.h>

#include "double_buf/state.h"

void db_render_backdrop(DoubleBenchState *state,
                        SDL_Renderer *renderer,
                        BenchMetrics *metrics);

void db_render_cube_and_particles(DoubleBenchState *state,
                                  SDL_Renderer *renderer,
                                  BenchMetrics *metrics);

#endif /* DOUBLE_BUF_RENDER_H */
