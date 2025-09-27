#include "common/geometry/shapes.h"

static const ShapeRenderFunc shape_renderers[SHAPE_COUNT] = {
    [SHAPE_CUBE] = bench_render_cube,
    [SHAPE_OCTAHEDRON] = bench_render_octahedron,
    [SHAPE_TETRAHEDRON] = bench_render_tetrahedron,
    [SHAPE_SPHERE] = bench_render_sphere,
};

const char *bench_get_shape_name(BenchShapeType shape)
{
    switch (shape) {
        case SHAPE_CUBE: return "CUBE";
        case SHAPE_OCTAHEDRON: return "OCTAHEDRON";
        case SHAPE_TETRAHEDRON: return "TETRAHEDRON";
        case SHAPE_SPHERE: return "SPHERE";
        default: return "UNKNOWN";
    }
}

void bench_render_shape(BenchShapeType shape,
                        SDL_Renderer *renderer,
                        BenchMetrics *metrics,
                        float rotation,
                        float cx,
                        float cy,
                        float size,
                        int mode)
{
    if (shape < 0 || shape >= SHAPE_COUNT) {
        return;
    }

    ShapeRenderFunc func = shape_renderers[shape];
    if (!func) {
        return;
    }

    func(renderer, metrics, rotation, cx, cy, size, mode);
}
