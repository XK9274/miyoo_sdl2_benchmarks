#include "common/geometry/square_pyramid.h"

#include "common/geometry/core.h"

static const float square_pyramid_base[][3] = {
    {-1.0f, -1.0f, -1.0f},
    { 1.0f, -1.0f, -1.0f},
    { 1.0f,  1.0f, -1.0f},
    {-1.0f,  1.0f, -1.0f},
    { 0.0f,  0.0f,  1.0f},
};

static const int square_pyramid_faces[][3] = {
    {0, 1, 2}, {0, 2, 3},
    {0, 1, 4}, {1, 2, 4}, {2, 3, 4}, {3, 0, 4},
};

static const int square_pyramid_edges[][2] = {
    {0, 1}, {0, 2}, {0, 3}, {0, 4},
    {1, 2}, {1, 4},
    {2, 3}, {2, 4},
    {3, 4},
};

static const SDL_Color square_pyramid_edge_palette[] = {
    {255, 140, 110, 255}, {140, 210, 255, 255}, {190, 255, 190, 255},
    {255, 220, 140, 255}, {220, 160, 255, 255},
};

static const SDL_Color square_pyramid_face_palette[] = {
    {255, 140, 110, 255}, {150, 210, 255, 255}, {190, 255, 210, 255},
    {255, 230, 150, 255}, {230, 170, 255, 255},
};

void bench_render_square_pyramid(SDL_Renderer *renderer,
                                 BenchMetrics *metrics,
                                 float rotation_radians,
                                 float center_x,
                                 float center_y,
                                 float size,
                                 int mode)
{
    BenchVertex vertices[5];
    RotationCache rotation_cache = {.rotation = NAN};
    bench_update_rotation_cache(&rotation_cache, rotation_radians);

    for (int i = 0; i < 5; ++i) {
        bench_project_vertex(square_pyramid_base[i], &rotation_cache, center_x, center_y, size, &vertices[i]);
    }

    if (mode == 0) {
        SDL_Vertex triangle_vertices[(int)(sizeof(square_pyramid_faces) / sizeof(square_pyramid_faces[0])) * 3];
        int tri_count = 0;

        const int palette_len = (int)(sizeof(square_pyramid_face_palette) / sizeof(square_pyramid_face_palette[0]));
        const int face_count = (int)(sizeof(square_pyramid_faces) / sizeof(square_pyramid_faces[0]));

        for (int i = 0; i < face_count; ++i) {
            const int *indices = square_pyramid_faces[i];
            const SDL_Color color = square_pyramid_face_palette[i % palette_len];

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
                                square_pyramid_edges,
                                (int)(sizeof(square_pyramid_edges) / sizeof(square_pyramid_edges[0])),
                                square_pyramid_edge_palette,
                                (int)(sizeof(square_pyramid_edge_palette) / sizeof(square_pyramid_edge_palette[0])),
                                mode,
                                metrics);
        return;
    }

    const SDL_Color point_color = {255, 255, 190, 255};
    bench_render_points(renderer, vertices, 5, &point_color, metrics);
}
