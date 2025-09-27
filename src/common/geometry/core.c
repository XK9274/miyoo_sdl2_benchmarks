#include "common/geometry/core.h"

#include <float.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void bench_update_rotation_cache(RotationCache *cache, float rotation_radians)
{
    if (!cache) {
        return;
    }

    if (!isfinite(cache->rotation) || fabsf(rotation_radians - cache->rotation) > 0.000015f) {
        cache->rotation = rotation_radians;
        cache->cos_val = cosf(rotation_radians);
        cache->sin_val = sinf(rotation_radians);
    }
}

void bench_project_vertex(const float *base_coords,
                          const RotationCache *cache,
                          float center_x,
                          float center_y,
                          float size,
                          BenchVertex *out_vertex)
{
    if (!base_coords || !cache || !out_vertex) {
        return;
    }

    const float depth = size * 5.0f;
    const float base_x = base_coords[0] * size;
    const float base_y = base_coords[1] * size;
    const float base_z = base_coords[2] * size;

    const float xr = base_x * cache->cos_val - base_z * cache->sin_val;
    const float zr = base_x * cache->sin_val + base_z * cache->cos_val;
    const float denom = depth + zr;
    const float scale = (fabsf(denom) > 1e-6f) ? (depth / denom) : 1.0f;

    out_vertex->rotate_x = xr;
    out_vertex->rotate_y = base_y;
    out_vertex->rotate_z = zr;
    out_vertex->screen_x = center_x + xr * scale;
    out_vertex->screen_y = center_y + base_y * scale;
}

void bench_setup_sdl_vertex(SDL_Vertex *vert,
                            const BenchVertex *bench_vert,
                            const SDL_Color *color)
{
    if (!vert || !bench_vert) {
        return;
    }

    vert->position.x = bench_vert->screen_x;
    vert->position.y = bench_vert->screen_y;
    vert->tex_coord.x = 0.0f;

    if (color) {
        vert->color = *color;
    } else {
        vert->color.r = 255;
        vert->color.g = 255;
        vert->color.b = 255;
        vert->color.a = 255;
    }
}

void bench_render_triangle_batch(SDL_Renderer *renderer,
                                 SDL_Vertex *vertices,
                                 int triangle_count,
                                 BenchMetrics *metrics)
{
    if (!renderer || !vertices || triangle_count <= 0) {
        return;
    }

    SDL_RenderGeometry(renderer, NULL, vertices, triangle_count * 3, NULL, 0);

    if (metrics) {
        metrics->draw_calls++;
        metrics->vertices_rendered += (Uint64)triangle_count * 3ULL;
        metrics->triangles_rendered += (Uint64)triangle_count;
    }
}

void bench_render_edge_batch(SDL_Renderer *renderer,
                             const BenchVertex *vertices,
                             const int (*edges)[2],
                             int edge_count,
                             const SDL_Color *palette,
                             int palette_size,
                             int mode,
                             BenchMetrics *metrics)
{
    if (!renderer || !vertices || !edges || edge_count <= 0 || palette_size <= 0 || !palette) {
        return;
    }

    SDL_FPoint line_points[edge_count * 2];
    SDL_FPoint point_points[edge_count * 2];
    int line_count = 0;
    int point_count = 0;

    const int edges_per_color = edge_count / palette_size;
    const int remainder = edge_count % palette_size;
    int current_edge = 0;

    for (int color_index = 0; color_index < palette_size; ++color_index) {
        int count = edges_per_color;
        if (color_index < remainder) {
            count += 1;
        }

        if (current_edge >= edge_count) {
            break;
        }

        const int color_line_start = line_count;
        const int color_point_start = point_count;
        const int edge_limit = current_edge + count;
        const SDL_Color edge_color = palette[color_index];
        const Uint8 alpha = (mode == 1) ? 255 : edge_color.a;

        for (; current_edge < edge_limit && current_edge < edge_count; ++current_edge) {
            const int a = edges[current_edge][0];
            const int b = edges[current_edge][1];
            const BenchVertex *va = &vertices[a];
            const BenchVertex *vb = &vertices[b];

            line_points[line_count].x = va->screen_x;
            line_points[line_count].y = va->screen_y;
            line_count++;
            line_points[line_count].x = vb->screen_x;
            line_points[line_count].y = vb->screen_y;
            line_count++;

            point_points[point_count].x = va->screen_x;
            point_points[point_count].y = va->screen_y;
            point_count++;
            point_points[point_count].x = vb->screen_x;
            point_points[point_count].y = vb->screen_y;
            point_count++;
        }

        if (line_count > color_line_start) {
            SDL_SetRenderDrawColor(renderer, edge_color.r, edge_color.g, edge_color.b, alpha);
            SDL_RenderDrawLinesF(renderer, &line_points[color_line_start], line_count - color_line_start);
            SDL_RenderDrawPointsF(renderer, &point_points[color_point_start], point_count - color_point_start);

            if (metrics) {
                metrics->draw_calls += 2;
                metrics->vertices_rendered += (Uint64)(line_count - color_line_start);
                metrics->vertices_rendered += (Uint64)(point_count - color_point_start);
            }
        }
    }
}

void bench_render_points(SDL_Renderer *renderer,
                         const BenchVertex *vertices,
                         int vertex_count,
                         const SDL_Color *color,
                         BenchMetrics *metrics)
{
    if (!renderer || !vertices || vertex_count <= 0) {
        return;
    }

    const SDL_Color draw_color = color ? *color : (SDL_Color){255, 255, 255, 255};
    SDL_SetRenderDrawColor(renderer, draw_color.r, draw_color.g, draw_color.b, draw_color.a);

    for (int i = 0; i < vertex_count; ++i) {
        SDL_RenderDrawPointF(renderer, vertices[i].screen_x, vertices[i].screen_y);
        if (metrics) {
            metrics->draw_calls++;
            metrics->vertices_rendered++;
        }
    }
}
