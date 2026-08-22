#include "space_bench/render/internal.h"

#include <math.h>

#include "common/geometry/core.h"

void space_render_wire_pyramid(SDL_Renderer *renderer,
                               BenchMetrics *metrics,
                               float roll_radians,
                               float center_x,
                               float center_y,
                               float apex_x,
                               float base_x,
                               float half_extent,
                               float extra_z,
                               SDL_Color color,
                               BenchVertex out_vertices[5])
{
    if (!renderer) {
        return;
    }

    const float model[5][3] = {
        {apex_x, 0.0f, 0.0f},
        {base_x, -half_extent, -half_extent},
        {base_x,  half_extent, -half_extent},
        {base_x,  half_extent,  half_extent},
        {base_x, -half_extent,  half_extent},
    };
    static const int edges[8][2] = {
        {0, 1}, {0, 2}, {0, 3}, {0, 4},
        {1, 2}, {2, 3}, {3, 4}, {4, 1},
    };

    RotationCache cache = {.rotation = NAN};
    bench_update_rotation_cache(&cache, roll_radians);

    const float depth = half_extent * 5.0f;

    BenchVertex local_vertices[5];
    BenchVertex *vertices = out_vertices ? out_vertices : local_vertices;
    for (int i = 0; i < 5; ++i) {
        bench_project_vertex_roll(model[i], &cache, center_x, center_y, depth, extra_z, &vertices[i]);
    }

    bench_render_edge_batch(renderer, vertices, edges, 8, &color, 1, 0, metrics);
}

void space_render_trail(const SpaceTrail *trail,
                        SDL_Renderer *renderer,
                        BenchMetrics *metrics,
                        SDL_Color primary,
                        SDL_Color secondary)
{
    (void)secondary;
    if (!trail || trail->count < 2) {
        return;
    }

    for (int i = 1; i < trail->count; ++i) {
        const SpaceTrailPoint *prev = &trail->points[i - 1];
        const SpaceTrailPoint *curr = &trail->points[i];
        const float t = (float)(trail->count - i) / (float)trail->count;

        SDL_Color blend = primary;
        blend.r = (Uint8)(primary.r * t);
        blend.g = (Uint8)(primary.g * t);
        blend.b = (Uint8)(primary.b * t);
        blend.a = (Uint8)(200 * t * curr->alpha);

        SDL_SetRenderDrawColor(renderer, blend.r, blend.g, blend.b, blend.a);
        SDL_RenderDrawLineF(renderer, prev->x, prev->y, curr->x, curr->y);
        if (metrics) {
            metrics->draw_calls++;
            metrics->vertices_rendered += 2;
        }
    }
}
