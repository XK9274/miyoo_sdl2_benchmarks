#ifndef COMMON_GEOMETRY_SPHERE_H
#define COMMON_GEOMETRY_SPHERE_H

#include <SDL2/SDL.h>

#include "common/types.h"

void bench_render_sphere(SDL_Renderer *renderer,
                         BenchMetrics *metrics,
                         float rotation_radians,
                         float center_x,
                         float center_y,
                         float size,
                         int mode);

#endif /* COMMON_GEOMETRY_SPHERE_H */
