#include "common/geometry/tetrahedron.h"

#include <math.h>

#include "common/geometry/core.h"

static const float tetrahedron_base[][3] = {
    { 1.0f,  1.0f,  1.0f},
    {-1.0f,  1.0f, -1.0f},
    {-1.0f, -1.0f,  1.0f},
    { 1.0f, -1.0f, -1.0f},
};

static const int tetrahedron_edges[][2] = {
    {0, 1}, {0, 2}, {0, 3},
    {1, 2}, {1, 3}, {2, 3},
};

static const SDL_Color tetrahedron_edge_palette[] = {
    {255, 120,  80, 255},
    { 80, 255, 160, 255},
    {130, 160, 255, 255},
};

static const int tetrahedron_faces[][3] = {
    {0, 1, 2},
    {0, 1, 3},
    {0, 2, 3},
    {1, 2, 3},
};

static const SDL_Color tetrahedron_face_colors[] = {
    {255, 120,  80, 255},
    { 80, 255, 160, 255},
    {130, 160, 255, 255},
    {200, 200, 100, 255},
};

static void bench_render_tetrahedron_faces(SDL_Renderer *renderer,
                                           const BenchVertex *vertices,
                                           BenchMetrics *metrics)
{
    SDL_Vertex triangle_vertices[4 * 3];
    int triangle_count = 0;

    for (int face = 0; face < 4; ++face) {
        const SDL_Color color = tetrahedron_face_colors[face];
        const int *indices = tetrahedron_faces[face];

        const BenchVertex *v0 = &vertices[indices[0]];
        const BenchVertex *v1 = &vertices[indices[1]];
        const BenchVertex *v2 = &vertices[indices[2]];

        const int base = triangle_count * 3;
        bench_setup_sdl_vertex(&triangle_vertices[base + 0], v0, &color);
        bench_setup_sdl_vertex(&triangle_vertices[base + 1], v1, &color);
        bench_setup_sdl_vertex(&triangle_vertices[base + 2], v2, &color);
        triangle_count++;
    }

    bench_render_triangle_batch(renderer, triangle_vertices, triangle_count, metrics);
}
void bench_render_tetrahedron(SDL_Renderer *renderer,
                              BenchMetrics *metrics,
                              float rotation_radians,
                              float center_x,
                              float center_y,
                              float size,
                              int mode)
{
    BenchVertex vertices[4];
    RotationCache rotation_cache = {.rotation = NAN};
    bench_update_rotation_cache(&rotation_cache, rotation_radians);

    for (int i = 0; i < 4; ++i) {
        bench_project_vertex(tetrahedron_base[i], &rotation_cache, center_x, center_y, size, &vertices[i]);
    }

    if (mode == 0) {
        bench_render_tetrahedron_faces(renderer, vertices, metrics);
    }

    if (mode == 0 || mode == 1) {
        bench_render_edge_batch(renderer,
                                vertices,
                                tetrahedron_edges,
                                (int)(sizeof(tetrahedron_edges) / sizeof(tetrahedron_edges[0])),
                                tetrahedron_edge_palette,
                                (int)(sizeof(tetrahedron_edge_palette) / sizeof(tetrahedron_edge_palette[0])),
                                mode,
                                metrics);
        return;
    }

    const SDL_Color point_color = {255, 255, 0, 255};
    bench_render_points(renderer, vertices, 4, &point_color, metrics);
}
