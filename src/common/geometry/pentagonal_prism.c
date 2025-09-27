#include "common/geometry/pentagonal_prism.h"

#include <math.h>

#include "common/geometry/core.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static const float pentagonal_prism_base[][3] = {
    { 1.00000000f,  0.00000000f,  1.0f},
    { 0.30901699f,  0.95105652f,  1.0f},
    {-0.80901699f,  0.58778525f,  1.0f},
    {-0.80901699f, -0.58778525f,  1.0f},
    { 0.30901699f, -0.95105652f,  1.0f},
    { 1.00000000f,  0.00000000f, -1.0f},
    { 0.30901699f,  0.95105652f, -1.0f},
    {-0.80901699f,  0.58778525f, -1.0f},
    {-0.80901699f, -0.58778525f, -1.0f},
    { 0.30901699f, -0.95105652f, -1.0f},
};

static const int pentagonal_prism_faces[][3] = {
    {0, 1, 2}, {0, 2, 3}, {0, 3, 4},
    {5, 7, 6}, {5, 8, 7}, {5, 9, 8},
    {0, 1, 6}, {0, 6, 5},
    {1, 2, 7}, {1, 7, 6},
    {2, 3, 8}, {2, 8, 7},
    {3, 4, 9}, {3, 9, 8},
    {4, 0, 5}, {4, 5, 9},
};

static const int pentagonal_prism_edges[][2] = {
    {0, 1}, {0, 2}, {0, 3}, {0, 4}, {0, 5}, {0, 6},
    {1, 2}, {1, 6}, {1, 7},
    {2, 3}, {2, 7}, {2, 8},
    {3, 4}, {3, 8}, {3, 9},
    {4, 5}, {4, 9},
    {5, 6}, {5, 7}, {5, 8}, {5, 9},
    {6, 7}, {7, 8}, {8, 9},
};

static const SDL_Color pentagonal_prism_edge_palette[] = {
    {255, 160, 120, 255}, {140, 220, 255, 255}, {180, 255, 170, 255},
    {255, 225, 140, 255}, {210, 150, 255, 255}, {255, 255, 255, 255},
};

static const SDL_Color pentagonal_prism_face_palette[] = {
    {255, 160, 120, 255}, {150, 210, 255, 255}, {180, 255, 200, 255},
    {255, 235, 150, 255}, {230, 160, 255, 255}, {255, 255, 210, 255},
};

void bench_render_pentagonal_prism(SDL_Renderer *renderer,
                                   BenchMetrics *metrics,
                                   float rotation_radians,
                                   float center_x,
                                   float center_y,
                                   float size,
                                   int mode)
{
    BenchVertex vertices[10];
    RotationCache rotation_cache = {.rotation = NAN};
    bench_update_rotation_cache(&rotation_cache, rotation_radians);

    for (int i = 0; i < 10; ++i) {
        bench_project_vertex(pentagonal_prism_base[i], &rotation_cache, center_x, center_y, size, &vertices[i]);
    }

    if (mode == 0) {
        SDL_Vertex triangle_vertices[(int)(sizeof(pentagonal_prism_faces) / sizeof(pentagonal_prism_faces[0])) * 3];
        int tri_count = 0;

        const int palette_len = (int)(sizeof(pentagonal_prism_face_palette) / sizeof(pentagonal_prism_face_palette[0]));
        const int face_count = (int)(sizeof(pentagonal_prism_faces) / sizeof(pentagonal_prism_faces[0]));

        for (int i = 0; i < face_count; ++i) {
            const int *indices = pentagonal_prism_faces[i];
            const SDL_Color color = pentagonal_prism_face_palette[i % palette_len];

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
                                pentagonal_prism_edges,
                                (int)(sizeof(pentagonal_prism_edges) / sizeof(pentagonal_prism_edges[0])),
                                pentagonal_prism_edge_palette,
                                (int)(sizeof(pentagonal_prism_edge_palette) / sizeof(pentagonal_prism_edge_palette[0])),
                                mode,
                                metrics);
        return;
    }

    const SDL_Color point_color = {255, 255, 200, 255};
    bench_render_points(renderer, vertices, 10, &point_color, metrics);
}
