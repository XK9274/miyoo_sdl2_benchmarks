#include "common/geometry.h"

#include <math.h>
#include <float.h>

#include <SDL2/SDL.h>

#if defined(__ARM_NEON)
#include <arm_neon.h>
#endif

typedef struct BenchCubeVertex {
    float rotate_x;
    float rotate_y;
    float rotate_z;
    float screen_x;
    float screen_y;
} BenchCubeVertex;

typedef struct BenchOctahedronVertex {
    float rotate_x;
    float rotate_y;
    float rotate_z;
    float screen_x;
    float screen_y;
} BenchOctahedronVertex;

static const float bench_cube_base_x[8] = {
    -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f
};

static const float bench_cube_base_y[8] = {
    -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f
};

static const float bench_cube_base_z[8] = {
    -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f
};

static const int bench_cube_edges[12][2] = {
    {0, 1}, {1, 2}, {2, 3}, {3, 0},
    {4, 5}, {5, 6}, {6, 7}, {7, 4},
    {0, 4}, {1, 5}, {2, 6}, {3, 7}
};

static const SDL_Color bench_cube_edge_palette[6] = {
    {255,  80,  40, 255},
    { 40, 210, 120, 255},
    { 90, 120, 255, 255},
    {255, 210,  90, 255},
    {210,  90, 255, 255},
    {255, 255, 255, 255}
};

#if SDL_VERSION_ATLEAST(2,0,18)
static const int bench_cube_faces[6][4] = {
    {0, 1, 2, 3},
    {4, 5, 6, 7},
    {0, 1, 5, 4},
    {2, 3, 7, 6},
    {1, 2, 6, 5},
    {0, 3, 7, 4}
};

static const SDL_Color bench_cube_face_colors[6] = {
    {255,  80,  40, 255},
    { 40, 210, 120, 255},
    { 90, 120, 255, 255},
    {255, 210,  90, 255},
    {210,  90, 255, 255},
    {255, 255, 255, 255}
};
#endif

static const float bench_octahedron_base_x[6] = {
    1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f
};

static const float bench_octahedron_base_y[6] = {
    0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f
};

static const float bench_octahedron_base_z[6] = {
    0.0f, 0.0f, 0.0f, 0.0f, 1.0f, -1.0f
};

static const int bench_octahedron_edges[12][2] = {
    {0, 2}, {0, 3}, {0, 4}, {0, 5},
    {1, 2}, {1, 3}, {1, 4}, {1, 5},
    {2, 4}, {2, 5}, {3, 4}, {3, 5}
};

static const SDL_Color bench_octahedron_edge_palette[6] = {
    {255, 100,  60, 255},
    { 60, 255, 140, 255},
    {110, 140, 255, 255},
    {255, 230, 110, 255},
    {230, 110, 255, 255},
    {255, 255, 255, 255}
};

#if SDL_VERSION_ATLEAST(2,0,18)
static const int bench_octahedron_faces[8][3] = {
    {0, 2, 4}, {0, 4, 3}, {0, 3, 5}, {0, 5, 2},
    {1, 4, 2}, {1, 3, 4}, {1, 5, 3}, {1, 2, 5}
};

static const SDL_Color bench_octahedron_face_colors[8] = {
    {255, 100,  60, 255},
    { 60, 255, 140, 255},
    {110, 140, 255, 255},
    {255, 230, 110, 255},
    {230, 110, 255, 255},
    {180, 180, 255, 255},
    {255, 180, 180, 255},
    {180, 255, 180, 255}
};
#endif

static void
bench_project_cube_vertices(BenchCubeVertex *out_vertices,
                            float rotation_radians,
                            float center_x,
                            float center_y,
                            float size)
{
    static float cached_rotation = FLT_MAX;
    static float cached_cos = 0.0f;
    static float cached_sin = 0.0f;

    // Cache sin/cos calculations when rotation changes meaningfully
    if (fabsf(rotation_radians - cached_rotation) > 0.000015f) {
        cached_rotation = rotation_radians;
        cached_cos = cosf(rotation_radians);
        cached_sin = sinf(rotation_radians);
    }

    const float cos_r = cached_cos;
    const float sin_r = cached_sin;
    const float depth = size * 5.0f;
#if defined(__ARM_NEON)
    const float32x4_t size_vec = vdupq_n_f32(size);
    const float32x4_t depth_vec = vdupq_n_f32(depth);
    const float32x4_t cos_vec = vdupq_n_f32(cos_r);
    const float32x4_t sin_vec = vdupq_n_f32(sin_r);
    const float32x4_t center_x_vec = vdupq_n_f32(center_x);
    const float32x4_t center_y_vec = vdupq_n_f32(center_y);

    for (int i = 0; i < 8; i += 4) {
        const float32x4_t base_x = vmulq_f32(vld1q_f32(&bench_cube_base_x[i]), size_vec);
        const float32x4_t base_y = vmulq_f32(vld1q_f32(&bench_cube_base_y[i]), size_vec);
        const float32x4_t base_z = vmulq_f32(vld1q_f32(&bench_cube_base_z[i]), size_vec);

        const float32x4_t xr = vsubq_f32(vmulq_f32(base_x, cos_vec), vmulq_f32(base_z, sin_vec));
        const float32x4_t zr = vaddq_f32(vmulq_f32(base_x, sin_vec), vmulq_f32(base_z, cos_vec));
        const float32x4_t denom = vaddq_f32(zr, depth_vec);

        float32x4_t recip = vrecpeq_f32(denom);
        recip = vmulq_f32(vrecpsq_f32(denom, recip), recip);
        recip = vmulq_f32(vrecpsq_f32(denom, recip), recip);
        const float32x4_t scale = vmulq_f32(depth_vec, recip);

        const float32x4_t screen_x = vaddq_f32(center_x_vec, vmulq_f32(xr, scale));
        const float32x4_t screen_y = vaddq_f32(center_y_vec, vmulq_f32(base_y, scale));

        float xr_out[4];
        float yr_out[4];
        float zr_out[4];
        float sx_out[4];
        float sy_out[4];

        vst1q_f32(xr_out, xr);
        vst1q_f32(yr_out, base_y);
        vst1q_f32(zr_out, zr);
        vst1q_f32(sx_out, screen_x);
        vst1q_f32(sy_out, screen_y);

        for (int lane = 0; lane < 4; ++lane) {
            const int dst = i + lane;
            out_vertices[dst].rotate_x = xr_out[lane];
            out_vertices[dst].rotate_y = yr_out[lane];
            out_vertices[dst].rotate_z = zr_out[lane];
            out_vertices[dst].screen_x = sx_out[lane];
            out_vertices[dst].screen_y = sy_out[lane];
        }
    }
#else
    for (int i = 0; i < 8; ++i) {
        const float base_x = bench_cube_base_x[i] * size;
        const float base_y = bench_cube_base_y[i] * size;
        const float base_z = bench_cube_base_z[i] * size;

        const float xr = base_x * cos_r - base_z * sin_r;
        const float zr = base_x * sin_r + base_z * cos_r;
        const float scale = depth / (depth + zr);

        out_vertices[i].rotate_x = xr;
        out_vertices[i].rotate_y = base_y;
        out_vertices[i].rotate_z = zr;
        out_vertices[i].screen_x = center_x + xr * scale;
        out_vertices[i].screen_y = center_y + base_y * scale;
    }
#endif
}

#if SDL_VERSION_ATLEAST(2,0,18)
static void
bench_render_cube_faces(SDL_Renderer *renderer,
                        const BenchCubeVertex *vertices,
                        BenchMetrics *metrics)
{
    SDL_Vertex verts[36];
    int v = 0;

    for (int face = 0; face < 6; ++face) {
        const SDL_Color color = bench_cube_face_colors[face];
        const int tri_indices[6] = {
            bench_cube_faces[face][0],
            bench_cube_faces[face][1],
            bench_cube_faces[face][2],
            bench_cube_faces[face][0],
            bench_cube_faces[face][2],
            bench_cube_faces[face][3]
        };

        for (int tri = 0; tri < 6; ++tri) {
            const BenchCubeVertex *src = &vertices[tri_indices[tri]];
            verts[v].position.x = src->screen_x;
            verts[v].position.y = src->screen_y;
            verts[v].color = color;
            verts[v].tex_coord.x = 0.0f;
            verts[v].tex_coord.y = 0.0f;
            ++v;
        }
    }

    SDL_RenderGeometry(renderer, NULL, verts, v, NULL, 0);

    if (metrics) {
        metrics->draw_calls++;
        metrics->vertices_rendered += v;
        metrics->triangles_rendered += v / 3;
    }
}
#endif

static void
bench_render_cube_edges(SDL_Renderer *renderer,
                        const BenchCubeVertex *vertices,
                        int mode,
                        BenchMetrics *metrics)
{
    // Batch all lines by color to reduce state changes
    SDL_FPoint lines[24]; // 12 edges * 2 points per line
    SDL_FPoint points[24]; // Up to 24 endpoints
    int line_count = 0;
    int point_count = 0;

    for (int color_index = 0; color_index < 6; ++color_index) {
        const SDL_Color edge_color = bench_cube_edge_palette[color_index];
        const Uint8 alpha = (mode == 1) ? 255 : edge_color.a;

        // Collect lines for this color
        const int first_edge = color_index * 2;
        const int color_line_start = line_count;
        const int color_point_start = point_count;

        for (int local = 0; local < 2; ++local) {
            const int edge_id = first_edge + local;
            const int a = bench_cube_edges[edge_id][0];
            const int b = bench_cube_edges[edge_id][1];
            const BenchCubeVertex *va = &vertices[a];
            const BenchCubeVertex *vb = &vertices[b];

            // Add line points
            lines[line_count].x = va->screen_x;
            lines[line_count].y = va->screen_y;
            line_count++;
            lines[line_count].x = vb->screen_x;
            lines[line_count].y = vb->screen_y;
            line_count++;

            // Add endpoints
            points[point_count].x = va->screen_x;
            points[point_count].y = va->screen_y;
            point_count++;
            points[point_count].x = vb->screen_x;
            points[point_count].y = vb->screen_y;
            point_count++;
        }

        // Draw this color's batch
        if (line_count > color_line_start) {
            SDL_SetRenderDrawColor(renderer, edge_color.r, edge_color.g, edge_color.b, alpha);
            SDL_RenderDrawLinesF(renderer, &lines[color_line_start], line_count - color_line_start);
            SDL_RenderDrawPointsF(renderer, &points[color_point_start], point_count - color_point_start);

            if (metrics) {
                metrics->draw_calls += 2; // One for lines, one for points
                metrics->vertices_rendered += (line_count - color_line_start) + (point_count - color_point_start);
            }
        }
    }
}

static void
bench_render_cube_points(SDL_Renderer *renderer,
                         const BenchCubeVertex *vertices,
                         BenchMetrics *metrics)
{
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
    for (int i = 0; i < 8; ++i) {
        SDL_RenderDrawPointF(renderer, vertices[i].screen_x, vertices[i].screen_y);
        if (metrics) {
            metrics->draw_calls++;
            metrics->vertices_rendered++;
        }
    }
}

void bench_render_cube(SDL_Renderer *renderer,
                       BenchMetrics *metrics,
                       float rotation_radians,
                       float center_x,
                       float center_y,
                       float size,
                       int mode)
{
    BenchCubeVertex vertices[8];

    bench_project_cube_vertices(vertices,
                                rotation_radians,
                                center_x,
                                center_y,
                                size);

#if SDL_VERSION_ATLEAST(2,0,18)
    if (mode == 0) {
        bench_render_cube_faces(renderer, vertices, metrics);
    }
#endif

    if (mode == 0 || mode == 1) {
        bench_render_cube_edges(renderer, vertices, mode, metrics);
        return;
    }

    bench_render_cube_points(renderer, vertices, metrics);
}

static void
bench_project_octahedron_vertices(BenchOctahedronVertex *out_vertices,
                                   float rotation_radians,
                                   float center_x,
                                   float center_y,
                                   float size)
{
    static float cached_rotation = FLT_MAX;
    static float cached_cos = 0.0f;
    static float cached_sin = 0.0f;

    if (fabsf(rotation_radians - cached_rotation) > 0.000015f) {
        cached_rotation = rotation_radians;
        cached_cos = cosf(rotation_radians);
        cached_sin = sinf(rotation_radians);
    }

    const float cos_r = cached_cos;
    const float sin_r = cached_sin;
    const float depth = size * 5.0f;

    for (int i = 0; i < 6; ++i) {
        const float base_x = bench_octahedron_base_x[i] * size;
        const float base_y = bench_octahedron_base_y[i] * size;
        const float base_z = bench_octahedron_base_z[i] * size;

        const float xr = base_x * cos_r - base_z * sin_r;
        const float zr = base_x * sin_r + base_z * cos_r;
        const float scale = depth / (depth + zr);

        out_vertices[i].rotate_x = xr;
        out_vertices[i].rotate_y = base_y;
        out_vertices[i].rotate_z = zr;
        out_vertices[i].screen_x = center_x + xr * scale;
        out_vertices[i].screen_y = center_y + base_y * scale;
    }
}

#if SDL_VERSION_ATLEAST(2,0,18)
static void
bench_render_octahedron_faces(SDL_Renderer *renderer,
                               const BenchOctahedronVertex *vertices,
                               BenchMetrics *metrics)
{
    SDL_Vertex verts[24];
    int v = 0;

    for (int face = 0; face < 8; ++face) {
        const SDL_Color color = bench_octahedron_face_colors[face];
        const int tri_indices[3] = {
            bench_octahedron_faces[face][0],
            bench_octahedron_faces[face][1],
            bench_octahedron_faces[face][2]
        };

        for (int tri = 0; tri < 3; ++tri) {
            const BenchOctahedronVertex *src = &vertices[tri_indices[tri]];
            verts[v].position.x = src->screen_x;
            verts[v].position.y = src->screen_y;
            verts[v].color = color;
            verts[v].tex_coord.x = 0.0f;
            verts[v].tex_coord.y = 0.0f;
            ++v;
        }
    }

    SDL_RenderGeometry(renderer, NULL, verts, v, NULL, 0);

    if (metrics) {
        metrics->draw_calls++;
        metrics->vertices_rendered += v;
        metrics->triangles_rendered += v / 3;
    }
}
#endif

static void
bench_render_octahedron_edges(SDL_Renderer *renderer,
                               const BenchOctahedronVertex *vertices,
                               int mode,
                               BenchMetrics *metrics)
{
    SDL_FPoint lines[24];
    SDL_FPoint points[24];
    int line_count = 0;
    int point_count = 0;

    for (int color_index = 0; color_index < 6; ++color_index) {
        const SDL_Color edge_color = bench_octahedron_edge_palette[color_index];
        const Uint8 alpha = (mode == 1) ? 255 : edge_color.a;

        const int first_edge = color_index * 2;
        const int color_line_start = line_count;
        const int color_point_start = point_count;

        for (int local = 0; local < 2; ++local) {
            const int edge_id = first_edge + local;
            if (edge_id >= 12) break;

            const int a = bench_octahedron_edges[edge_id][0];
            const int b = bench_octahedron_edges[edge_id][1];
            const BenchOctahedronVertex *va = &vertices[a];
            const BenchOctahedronVertex *vb = &vertices[b];

            lines[line_count].x = va->screen_x;
            lines[line_count].y = va->screen_y;
            line_count++;
            lines[line_count].x = vb->screen_x;
            lines[line_count].y = vb->screen_y;
            line_count++;

            points[point_count].x = va->screen_x;
            points[point_count].y = va->screen_y;
            point_count++;
            points[point_count].x = vb->screen_x;
            points[point_count].y = vb->screen_y;
            point_count++;
        }

        if (line_count > color_line_start) {
            SDL_SetRenderDrawColor(renderer, edge_color.r, edge_color.g, edge_color.b, alpha);
            SDL_RenderDrawLinesF(renderer, &lines[color_line_start], line_count - color_line_start);
            SDL_RenderDrawPointsF(renderer, &points[color_point_start], point_count - color_point_start);

            if (metrics) {
                metrics->draw_calls += 2;
                metrics->vertices_rendered += (line_count - color_line_start) + (point_count - color_point_start);
            }
        }
    }
}

static void
bench_render_octahedron_points(SDL_Renderer *renderer,
                                const BenchOctahedronVertex *vertices,
                                BenchMetrics *metrics)
{
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
    for (int i = 0; i < 6; ++i) {
        SDL_RenderDrawPointF(renderer, vertices[i].screen_x, vertices[i].screen_y);
        if (metrics) {
            metrics->draw_calls++;
            metrics->vertices_rendered++;
        }
    }
}

void bench_render_octahedron(SDL_Renderer *renderer,
                             BenchMetrics *metrics,
                             float rotation_radians,
                             float center_x,
                             float center_y,
                             float size,
                             int mode)
{
    BenchOctahedronVertex vertices[6];

    bench_project_octahedron_vertices(vertices,
                                      rotation_radians,
                                      center_x,
                                      center_y,
                                      size);

#if SDL_VERSION_ATLEAST(2,0,18)
    if (mode == 0) {
        bench_render_octahedron_faces(renderer, vertices, metrics);
    }
#endif

    if (mode == 0 || mode == 1) {
        bench_render_octahedron_edges(renderer, vertices, mode, metrics);
        return;
    }

    bench_render_octahedron_points(renderer, vertices, metrics);
}
