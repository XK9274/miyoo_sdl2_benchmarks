#include "common/geometry/cube.h"

#include <float.h>
#include <math.h>

#include "common/geometry/core.h"

static const float cube_base[][3] = {
    {-1.0f, -1.0f, -1.0f},
    { 1.0f, -1.0f, -1.0f},
    { 1.0f,  1.0f, -1.0f},
    {-1.0f,  1.0f, -1.0f},
    {-1.0f, -1.0f,  1.0f},
    { 1.0f, -1.0f,  1.0f},
    { 1.0f,  1.0f,  1.0f},
    {-1.0f,  1.0f,  1.0f},
};

static const int cube_edges[][2] = {
    {0, 1}, {1, 2}, {2, 3}, {3, 0},
    {4, 5}, {5, 6}, {6, 7}, {7, 4},
    {0, 4}, {1, 5}, {2, 6}, {3, 7},
};

static const SDL_Color cube_edge_palette[] = {
    {255,  80,  40, 255},
    { 40, 210, 120, 255},
    { 90, 120, 255, 255},
    {255, 210,  90, 255},
    {210,  90, 255, 255},
    {255, 255, 255, 255},
};

static const int cube_faces[][4] = {
    {0, 1, 2, 3},
    {4, 5, 6, 7},
    {0, 1, 5, 4},
    {2, 3, 7, 6},
    {1, 2, 6, 5},
    {0, 3, 7, 4},
};

static const SDL_Color cube_face_colors[] = {
    {255,  80,  40, 255},
    { 40, 210, 120, 255},
    { 90, 120, 255, 255},
    {255, 210,  90, 255},
    {210,  90, 255, 255},
    {255, 255, 255, 255},
};

static void bench_render_cube_faces(SDL_Renderer *renderer,
                                    const BenchVertex *vertices,
                                    BenchMetrics *metrics)
{
    SDL_Vertex triangle_vertices[12 * 3];
    int triangle_count = 0;

    for (int face = 0; face < 6; ++face) {
        const SDL_Color color = cube_face_colors[face];
        const int *indices = cube_faces[face];

        const int tri_indices[6] = {
            indices[0], indices[1], indices[2],
            indices[0], indices[2], indices[3]
        };

        for (int tri = 0; tri < 2; ++tri) {
            const BenchVertex *v0 = &vertices[tri_indices[tri * 3 + 0]];
            const BenchVertex *v1 = &vertices[tri_indices[tri * 3 + 1]];
            const BenchVertex *v2 = &vertices[tri_indices[tri * 3 + 2]];

            const int base = triangle_count * 3;
            bench_setup_sdl_vertex(&triangle_vertices[base + 0], v0, &color);
            bench_setup_sdl_vertex(&triangle_vertices[base + 1], v1, &color);
            bench_setup_sdl_vertex(&triangle_vertices[base + 2], v2, &color);
            triangle_count++;
        }
    }

    bench_render_triangle_batch(renderer, triangle_vertices, triangle_count, metrics);
}
void bench_render_cube(SDL_Renderer *renderer,
                       BenchMetrics *metrics,
                       float rotation_radians,
                       float center_x,
                       float center_y,
                       float size,
                       int mode)
{
    BenchVertex vertices[8];
    RotationCache rotation_cache = {.rotation = NAN};
    bench_update_rotation_cache(&rotation_cache, rotation_radians);

    for (int i = 0; i < 8; ++i) {
        bench_project_vertex(cube_base[i], &rotation_cache, center_x, center_y, size, &vertices[i]);
    }

    if (mode == 0) {
        bench_render_cube_faces(renderer, vertices, metrics);
    }

    if (mode == 0 || mode == 1) {
        bench_render_edge_batch(renderer,
                                vertices,
                                cube_edges,
                                (int)(sizeof(cube_edges) / sizeof(cube_edges[0])),
                                cube_edge_palette,
                                (int)(sizeof(cube_edge_palette) / sizeof(cube_edge_palette[0])),
                                mode,
                                metrics);
        return;
    }

    const SDL_Color point_color = {255, 255, 0, 255};
    bench_render_points(renderer, vertices, 8, &point_color, metrics);
}
