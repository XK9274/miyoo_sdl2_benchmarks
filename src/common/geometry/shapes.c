#include "common/geometry/shapes.h"

static const ShapeRenderFunc shape_renderers[SHAPE_COUNT] = {
    [SHAPE_CUBE] = bench_render_cube,
    [SHAPE_OCTAHEDRON] = bench_render_octahedron,
    [SHAPE_TETRAHEDRON] = bench_render_tetrahedron,
    [SHAPE_SPHERE] = bench_render_sphere,
    [SHAPE_ICOSAHEDRON] = bench_render_icosahedron,
    [SHAPE_PENTAGONAL_PRISM] = bench_render_pentagonal_prism,
    [SHAPE_SQUARE_PYRAMID] = bench_render_square_pyramid,
};

const char *bench_get_shape_name(BenchShapeType shape)
{
    switch (shape) {
        case SHAPE_CUBE: return "CUBE";
        case SHAPE_OCTAHEDRON: return "OCTAHEDRON";
        case SHAPE_TETRAHEDRON: return "TETRAHEDRON";
        case SHAPE_SPHERE: return "SPHERE";
        case SHAPE_ICOSAHEDRON: return "ICOSAHEDRON";
        case SHAPE_PENTAGONAL_PRISM: return "PENTAGONAL PRISM";
        case SHAPE_SQUARE_PYRAMID: return "SQUARE PYRAMID";
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
