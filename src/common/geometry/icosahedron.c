#include "common/geometry/icosahedron.h"

#include <math.h>

#include "common/geometry/core.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define ICOS_PHI 1.61803398875f

static const float icosahedron_base[][3] = {
    {-1.0f,  ICOS_PHI,  0.0f},
    { 1.0f,  ICOS_PHI,  0.0f},
    {-1.0f, -ICOS_PHI,  0.0f},
    { 1.0f, -ICOS_PHI,  0.0f},
    { 0.0f, -1.0f,  ICOS_PHI},
    { 0.0f,  1.0f,  ICOS_PHI},
    { 0.0f, -1.0f, -ICOS_PHI},
    { 0.0f,  1.0f, -ICOS_PHI},
    { ICOS_PHI,  0.0f, -1.0f},
    { ICOS_PHI,  0.0f,  1.0f},
    {-ICOS_PHI,  0.0f, -1.0f},
    {-ICOS_PHI,  0.0f,  1.0f},
};

static const int icosahedron_faces[][3] = {
    {0, 11, 5}, {0, 5, 1}, {0, 1, 7}, {0, 7, 10}, {0, 10, 11},
    {1, 5, 9}, {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8},
    {3, 9, 4}, {3, 4, 2}, {3, 2, 6}, {3, 6, 8}, {3, 8, 9},
    {4, 9, 5}, {2, 4, 11}, {6, 2, 10}, {8, 6, 7}, {9, 8, 1},
};

static const int icosahedron_edges[][2] = {
    {0, 1}, {0, 5}, {0, 7}, {0, 10}, {0, 11}, {1, 5}, {1, 7}, {1, 8},
    {1, 9}, {2, 3}, {2, 4}, {2, 6}, {2, 10}, {2, 11}, {3, 4}, {3, 6},
    {3, 8}, {3, 9}, {4, 5}, {4, 9}, {4, 11}, {5, 9}, {5, 11}, {6, 7},
    {6, 8}, {6, 10}, {7, 8}, {7, 10}, {8, 9}, {10, 11},
};

static const SDL_Color icosahedron_edge_palette[] = {
    {255, 120, 90, 255}, {130, 200, 255, 255}, {120, 255, 180, 255},
    {255, 210, 110, 255}, {210, 140, 255, 255}, {255, 255, 255, 255},
};

static const SDL_Color icosahedron_face_palette[] = {
    {255, 120, 90, 255}, {110, 200, 255, 255}, {120, 255, 180, 255},
    {255, 210, 110, 255}, {200, 140, 255, 255}, {255, 255, 180, 255},
};

void bench_render_icosahedron(SDL_Renderer *renderer,
                              BenchMetrics *metrics,
                              float rotation_radians,
                              float center_x,
                              float center_y,
                              float size,
                              int mode)
{
    BenchVertex vertices[12];
    RotationCache rotation_cache = {.rotation = NAN};
    bench_update_rotation_cache(&rotation_cache, rotation_radians);

    for (int i = 0; i < 12; ++i) {
        bench_project_vertex(icosahedron_base[i], &rotation_cache, center_x, center_y, size, &vertices[i]);
    }

    if (mode == 0) {
        SDL_Vertex triangle_vertices[20 * 3];
        int tri_count = 0;

        for (int i = 0; i < 20; ++i) {
            const int *indices = icosahedron_faces[i];
            const SDL_Color color = icosahedron_face_palette[i % (int)(sizeof(icosahedron_face_palette) / sizeof(icosahedron_face_palette[0]))];

            const BenchVertex *v0 = &vertices[indices[0]];
            const BenchVertex *v1 = &vertices[indices[1]];
            const BenchVertex *v2 = &vertices[indices[2]];

            const int base = tri_count * 3;
            bench_setup_sdl_vertex(&triangle_vertices[base + 0], v0, &color);
            bench_setup_sdl_vertex(&triangle_vertices[base + 1], v1, &color);
            bench_setup_sdl_vertex(&triangle_vertices[base + 2], v2, &color);
            tri_count++;
        }

        bench_render_triangle_batch(renderer, triangle_vertices, tri_count, metrics);
    }

    if (mode == 0 || mode == 1) {
        bench_render_edge_batch(renderer,
                                vertices,
                                icosahedron_edges,
                                (int)(sizeof(icosahedron_edges) / sizeof(icosahedron_edges[0])),
                                icosahedron_edge_palette,
                                (int)(sizeof(icosahedron_edge_palette) / sizeof(icosahedron_edge_palette[0])),
                                mode,
                                metrics);
        return;
    }

    const SDL_Color point_color = {255, 255, 180, 255};
    bench_render_points(renderer, vertices, 12, &point_color, metrics);
}
