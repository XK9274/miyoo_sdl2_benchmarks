#include "common/geometry/sphere.h"

#include <math.h>

#include "common/geometry/core.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define SPHERE_LAT_DIVISIONS 8
#define SPHERE_LON_DIVISIONS 12
#define SPHERE_VERTEX_COUNT ((SPHERE_LAT_DIVISIONS - 1) * SPHERE_LON_DIVISIONS + 2)
#define SPHERE_TRIANGLE_COUNT (SPHERE_LAT_DIVISIONS * SPHERE_LON_DIVISIONS * 2)

static const SDL_Color sphere_colors[] = {
    {255, 160, 120, 255}, {120, 255, 200, 255}, {170, 200, 255, 255},
    {255, 255, 150, 255}, {255, 150, 255, 255}, {200, 220, 255, 255},
    {255, 220, 220, 255}, {220, 255, 220, 255},
};

static void bench_generate_sphere_vertices(BenchVertex *out_vertices,
                                           const RotationCache *cache,
                                           float center_x,
                                           float center_y,
                                           float size)
{
    int vertex_index = 0;
    float base[3];

    base[0] = 0.0f;
    base[1] = 1.0f;
    base[2] = 0.0f;
    bench_project_vertex(base, cache, center_x, center_y, size, &out_vertices[vertex_index++]);

    float lat_sin[SPHERE_LAT_DIVISIONS - 1];
    float lat_cos[SPHERE_LAT_DIVISIONS - 1];
    for (int lat = 1; lat < SPHERE_LAT_DIVISIONS; ++lat) {
        const float theta = (float)M_PI * (float)lat / (float)SPHERE_LAT_DIVISIONS;
        lat_sin[lat - 1] = sinf(theta);
        lat_cos[lat - 1] = cosf(theta);
    }

    float lon_sin[SPHERE_LON_DIVISIONS];
    float lon_cos[SPHERE_LON_DIVISIONS];
    for (int lon = 0; lon < SPHERE_LON_DIVISIONS; ++lon) {
        const float phi = 2.0f * (float)M_PI * (float)lon / (float)SPHERE_LON_DIVISIONS;
        lon_sin[lon] = sinf(phi);
        lon_cos[lon] = cosf(phi);
    }

    for (int lat = 1; lat < SPHERE_LAT_DIVISIONS; ++lat) {
        const float sin_theta = lat_sin[lat - 1];
        const float cos_theta = lat_cos[lat - 1];

        for (int lon = 0; lon < SPHERE_LON_DIVISIONS; ++lon) {
            base[0] = sin_theta * lon_cos[lon];
            base[1] = cos_theta;
            base[2] = sin_theta * lon_sin[lon];
            bench_project_vertex(base, cache, center_x, center_y, size, &out_vertices[vertex_index++]);
        }
    }

    base[0] = 0.0f;
    base[1] = -1.0f;
    base[2] = 0.0f;
    bench_project_vertex(base, cache, center_x, center_y, size, &out_vertices[vertex_index]);
}

static void bench_render_sphere_faces(SDL_Renderer *renderer,
                                      const BenchVertex *vertices,
                                      BenchMetrics *metrics)
{
    SDL_Vertex triangle_vertices[SPHERE_TRIANGLE_COUNT * 3];
    int triangle_count = 0;

    for (int lon = 0; lon < SPHERE_LON_DIVISIONS; ++lon) {
        const SDL_Color color = sphere_colors[lon % 8];
        const int next_lon = (lon + 1) % SPHERE_LON_DIVISIONS;

        const BenchVertex *v0 = &vertices[0];
        const BenchVertex *v1 = &vertices[1 + lon];
        const BenchVertex *v2 = &vertices[1 + next_lon];

        const int base = triangle_count * 3;
        bench_setup_sdl_vertex(&triangle_vertices[base + 0], v0, &color);
        bench_setup_sdl_vertex(&triangle_vertices[base + 1], v1, &color);
        bench_setup_sdl_vertex(&triangle_vertices[base + 2], v2, &color);
        triangle_count++;
    }

    for (int lat = 0; lat < SPHERE_LAT_DIVISIONS - 2; ++lat) {
        const int curr_ring_start = 1 + lat * SPHERE_LON_DIVISIONS;
        const int next_ring_start = 1 + (lat + 1) * SPHERE_LON_DIVISIONS;

        for (int lon = 0; lon < SPHERE_LON_DIVISIONS; ++lon) {
            const int next_lon = (lon + 1) % SPHERE_LON_DIVISIONS;
            const SDL_Color color = sphere_colors[(lat + lon) % 8];

            const BenchVertex *v0 = &vertices[curr_ring_start + lon];
            const BenchVertex *v1 = &vertices[next_ring_start + lon];
            const BenchVertex *v2 = &vertices[curr_ring_start + next_lon];

            const int base = triangle_count * 3;
            bench_setup_sdl_vertex(&triangle_vertices[base + 0], v0, &color);
            bench_setup_sdl_vertex(&triangle_vertices[base + 1], v1, &color);
            bench_setup_sdl_vertex(&triangle_vertices[base + 2], v2, &color);
            triangle_count++;

            const BenchVertex *v3 = &vertices[curr_ring_start + next_lon];
            const BenchVertex *v4 = &vertices[next_ring_start + lon];
            const BenchVertex *v5 = &vertices[next_ring_start + next_lon];

            const int base2 = triangle_count * 3;
            bench_setup_sdl_vertex(&triangle_vertices[base2 + 0], v3, &color);
            bench_setup_sdl_vertex(&triangle_vertices[base2 + 1], v4, &color);
            bench_setup_sdl_vertex(&triangle_vertices[base2 + 2], v5, &color);
            triangle_count++;
        }
    }

    const int south_pole_index = SPHERE_VERTEX_COUNT - 1;
    const int last_ring_start = south_pole_index - SPHERE_LON_DIVISIONS;

    for (int lon = 0; lon < SPHERE_LON_DIVISIONS; ++lon) {
        const SDL_Color color = sphere_colors[(lon + 3) % 8];
        const int next_lon = (lon + 1) % SPHERE_LON_DIVISIONS;

        const BenchVertex *v0 = &vertices[last_ring_start + lon];
        const BenchVertex *v1 = &vertices[last_ring_start + next_lon];
        const BenchVertex *v2 = &vertices[south_pole_index];

        const int base = triangle_count * 3;
        bench_setup_sdl_vertex(&triangle_vertices[base + 0], v0, &color);
        bench_setup_sdl_vertex(&triangle_vertices[base + 1], v1, &color);
        bench_setup_sdl_vertex(&triangle_vertices[base + 2], v2, &color);
        triangle_count++;
    }

    bench_render_triangle_batch(renderer, triangle_vertices, triangle_count, metrics);
}
static void bench_render_sphere_wireframe(SDL_Renderer *renderer,
                                          const BenchVertex *vertices,
                                          BenchMetrics *metrics)
{
    SDL_SetRenderDrawColor(renderer, 100, 200, 255, 255);
    for (int lat = 1; lat < SPHERE_LAT_DIVISIONS; ++lat) {
        const int ring_start = 1 + (lat - 1) * SPHERE_LON_DIVISIONS;
        for (int lon = 0; lon < SPHERE_LON_DIVISIONS; ++lon) {
            const int next_lon = (lon + 1) % SPHERE_LON_DIVISIONS;
            SDL_RenderDrawLineF(renderer,
                                vertices[ring_start + lon].screen_x,
                                vertices[ring_start + lon].screen_y,
                                vertices[ring_start + next_lon].screen_x,
                                vertices[ring_start + next_lon].screen_y);
        }
    }

    SDL_SetRenderDrawColor(renderer, 255, 150, 100, 255);
    for (int lon = 0; lon < SPHERE_LON_DIVISIONS; ++lon) {
        SDL_RenderDrawLineF(renderer,
                            vertices[0].screen_x,
                            vertices[0].screen_y,
                            vertices[1 + lon].screen_x,
                            vertices[1 + lon].screen_y);

        for (int lat = 0; lat < SPHERE_LAT_DIVISIONS - 2; ++lat) {
            const int curr_ring = 1 + lat * SPHERE_LON_DIVISIONS;
            const int next_ring = 1 + (lat + 1) * SPHERE_LON_DIVISIONS;
            SDL_RenderDrawLineF(renderer,
                                vertices[curr_ring + lon].screen_x,
                                vertices[curr_ring + lon].screen_y,
                                vertices[next_ring + lon].screen_x,
                                vertices[next_ring + lon].screen_y);
        }

        const int last_ring = 1 + (SPHERE_LAT_DIVISIONS - 2) * SPHERE_LON_DIVISIONS;
        SDL_RenderDrawLineF(renderer,
                            vertices[last_ring + lon].screen_x,
                            vertices[last_ring + lon].screen_y,
                            vertices[SPHERE_VERTEX_COUNT - 1].screen_x,
                            vertices[SPHERE_VERTEX_COUNT - 1].screen_y);
    }

    if (metrics) {
        const Uint64 lat_calls = (Uint64)(SPHERE_LAT_DIVISIONS - 1) * (Uint64)SPHERE_LON_DIVISIONS;
        const Uint64 lon_calls = (Uint64)SPHERE_LON_DIVISIONS * (Uint64)SPHERE_LAT_DIVISIONS;
        metrics->draw_calls += lat_calls + lon_calls;
        metrics->vertices_rendered += 2ULL * (lat_calls + lon_calls);
    }
}

void bench_render_sphere(SDL_Renderer *renderer,
                         BenchMetrics *metrics,
                         float rotation_radians,
                         float center_x,
                         float center_y,
                         float size,
                         int mode)
{
    BenchVertex vertices[SPHERE_VERTEX_COUNT];
    RotationCache rotation_cache = {.rotation = NAN};
    bench_update_rotation_cache(&rotation_cache, rotation_radians);

    bench_generate_sphere_vertices(vertices, &rotation_cache, center_x, center_y, size);

    if (mode == 0) {
        bench_render_sphere_faces(renderer, vertices, metrics);
        return;
    }

    if (mode == 1) {
        bench_render_sphere_wireframe(renderer, vertices, metrics);
        return;
    }

    const SDL_Color point_color = {255, 255, 0, 255};
    bench_render_points(renderer, vertices, SPHERE_VERTEX_COUNT, &point_color, metrics);
}
