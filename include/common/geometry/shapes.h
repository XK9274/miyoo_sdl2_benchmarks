#ifndef COMMON_GEOMETRY_SHAPES_H
#define COMMON_GEOMETRY_SHAPES_H

#include <SDL2/SDL.h>

#include "common/types.h"

#include "common/geometry/cube.h"
#include "common/geometry/octahedron.h"
#include "common/geometry/tetrahedron.h"
#include "common/geometry/sphere.h"

typedef enum {
    SHAPE_CUBE = 0,
    SHAPE_OCTAHEDRON = 1,
    SHAPE_TETRAHEDRON = 2,
    SHAPE_SPHERE = 3,
    SHAPE_COUNT = 4
} BenchShapeType;

typedef void (*ShapeRenderFunc)(SDL_Renderer*, BenchMetrics*,
                                float rotation, float cx, float cy, float size,
                                int mode);

const char *bench_get_shape_name(BenchShapeType shape);

void bench_render_shape(BenchShapeType shape,
                        SDL_Renderer *renderer,
                        BenchMetrics *metrics,
                        float rotation,
                        float cx,
                        float cy,
                        float size,
                        int mode);

#endif /* COMMON_GEOMETRY_SHAPES_H */
