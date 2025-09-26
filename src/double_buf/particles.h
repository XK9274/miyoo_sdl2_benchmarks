#ifndef DOUBLE_BUF_PARTICLES_H
#define DOUBLE_BUF_PARTICLES_H

#include <SDL2/SDL.h>

#include "double_buf/state.h"

void db_particles_update(DoubleBenchState *state, double dt);
void db_particles_draw(DoubleBenchState *state,
                       SDL_Renderer *renderer,
                       BenchMetrics *metrics);

#endif /* DOUBLE_BUF_PARTICLES_H */
