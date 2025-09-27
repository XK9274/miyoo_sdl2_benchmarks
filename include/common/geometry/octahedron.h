#ifndef COMMON_GEOMETRY_OCTAHEDRON_H
#define COMMON_GEOMETRY_OCTAHEDRON_H

#include <SDL2/SDL.h>

#include "common/types.h"

void bench_render_octahedron(SDL_Renderer *renderer,
                             BenchMetrics *metrics,
                             float rotation_radians,
                             float center_x,
                             float center_y,
                             float size,
                             int mode);

#endif /* COMMON_GEOMETRY_OCTAHEDRON_H */
