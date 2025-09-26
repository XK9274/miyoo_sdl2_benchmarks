#ifndef SOFTWARE_BUF_PARTICLES_H
#define SOFTWARE_BUF_PARTICLES_H

#include <SDL2/SDL.h>

#include "software_buf/state.h"

void sb_particles_update(SoftwareBenchState *state, double dt);
void sb_particles_draw(SoftwareBenchState *state,
                       SDL_Renderer *renderer,
                       BenchMetrics *metrics);

#endif /* SOFTWARE_BUF_PARTICLES_H */
