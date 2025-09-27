#include "common/geometry/octahedron.h"

#include <math.h>

#include "common/geometry/core.h"

static const float octahedron_base[][3] = {
    { 1.0f,  0.0f,  0.0f},
    {-1.0f,  0.0f,  0.0f},
    { 0.0f,  1.0f,  0.0f},
    { 0.0f, -1.0f,  0.0f},
    { 0.0f,  0.0f,  1.0f},
    { 0.0f,  0.0f, -1.0f},
};

static const int octahedron_edges[][2] = {
    {0, 2}, {0, 3}, {0, 4}, {0, 5},
    {1, 2}, {1, 3}, {1, 4}, {1, 5},
    {2, 4}, {2, 5}, {3, 4}, {3, 5},
};

static const SDL_Color octahedron_edge_palette[] = {
    {255, 100,  60, 255},
    { 60, 255, 140, 255},
    {110, 140, 255, 255},
    {255, 230, 110, 255},
    {230, 110, 255, 255},
    {255, 255, 255, 255},
};

#if SDL_VERSION_ATLEAST(2,0,18)
static const int octahedron_faces[][3] = {
    {0, 2, 4}, {0, 4, 3}, {0, 3, 5}, {0, 5, 2},
    {1, 4, 2}, {1, 3, 4}, {1, 5, 3}, {1, 2, 5},
};

static const SDL_Color octahedron_face_colors[] = {
    {255, 100,  60, 255},
    { 60, 255, 140, 255},
    {110, 140, 255, 255},
    {255, 230, 110, 255},
    {230, 110, 255, 255},
    {180, 180, 255, 255},
    {255, 180, 180, 255},
    {180, 255, 180, 255},
};

static void bench_render_octahedron_faces(SDL_Renderer *renderer,
                                          const BenchVertex *vertices,
                                          BenchMetrics *metrics)
{
    SDL_Vertex triangle_vertices[8 * 3];
    int triangle_count = 0;

    for (int face = 0; face < 8; ++face) {
        const SDL_Color color = octahedron_face_colors[face];
        const int *indices = octahedron_faces[face];

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
#endif

void bench_render_octahedron(SDL_Renderer *renderer,
                             BenchMetrics *metrics,
                             float rotation_radians,
                             float center_x,
                             float center_y,
                             float size,
                             int mode)
{
    BenchVertex vertices[6];
    RotationCache rotation_cache = {.rotation = NAN};
    bench_update_rotation_cache(&rotation_cache, rotation_radians);

    for (int i = 0; i < 6; ++i) {
        bench_project_vertex(octahedron_base[i], &rotation_cache, center_x, center_y, size, &vertices[i]);
    }

#if SDL_VERSION_ATLEAST(2,0,18)
    if (mode == 0) {
        bench_render_octahedron_faces(renderer, vertices, metrics);
    }
#endif

    if (mode == 0 || mode == 1) {
        bench_render_edge_batch(renderer,
                                vertices,
                                octahedron_edges,
                                (int)(sizeof(octahedron_edges) / sizeof(octahedron_edges[0])),
                                octahedron_edge_palette,
                                (int)(sizeof(octahedron_edge_palette) / sizeof(octahedron_edge_palette[0])),
                                mode,
                                metrics);
        return;
    }

    const SDL_Color point_color = {255, 255, 0, 255};
    bench_render_points(renderer, vertices, 6, &point_color, metrics);
}
