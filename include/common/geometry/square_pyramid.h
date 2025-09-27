#ifndef COMMON_GEOMETRY_SQUARE_PYRAMID_H
#define COMMON_GEOMETRY_SQUARE_PYRAMID_H

#include <SDL2/SDL.h>

#include "common/types.h"

void bench_render_square_pyramid(SDL_Renderer *renderer,
                                 BenchMetrics *metrics,
                                 float rotation_radians,
                                 float center_x,
                                 float center_y,
                                 float size,
                                 int mode);

#endif /* COMMON_GEOMETRY_SQUARE_PYRAMID_H */
