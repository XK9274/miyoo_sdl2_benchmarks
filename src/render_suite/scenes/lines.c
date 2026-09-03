#include <math.h>
#include <stdlib.h>

#include "render_suite/scenes/lines.h"
#include "common/geometry/shapes.h"

#define RS_LINES_TWO_PI 6.28318530717958647692f
#define RS_LINES_MAX_GRID_N 12
#define RS_LINES_MAX_COLUMNS (RS_LINES_MAX_GRID_N * RS_LINES_MAX_GRID_N)
#define RS_LINES_MAX_COLUMN_HEIGHT 24
/* Worst case: every cube exposes all 4 sides plus one top face per column,
 * sized for grid_n=12, max column height=20. */
#define RS_LINES_MAX_EXPOSED_FACES 12288
#define RS_LINES_MAX_ANOMALIES RS_LINES_MAX_GRID_N

typedef enum {
    RS_LINES_FACE_TOP = 0,
    RS_LINES_FACE_PX,
    RS_LINES_FACE_NX,
    RS_LINES_FACE_PY,
    RS_LINES_FACE_NY,
    RS_LINES_FACE_AXIS_COUNT
} RSLinesFaceAxis;

typedef struct {
    int corner_idx[4];
    float shade;
    float ox, oy, oz;
} RSLinesFaceDef;

/* Bottom faces are never emitted -- every column in this scene rests on the
 * ground or another cube, so undersides are never visible from any angle. */
static const RSLinesFaceDef g_lines_face_defs[RS_LINES_FACE_AXIS_COUNT] = {
    [RS_LINES_FACE_TOP] = {{4, 5, 6, 7}, 1.15f, 0.5f, 0.5f, 1.0f},
    [RS_LINES_FACE_PX]  = {{1, 2, 6, 5}, 0.85f, 1.0f, 0.5f, 0.5f},
    [RS_LINES_FACE_NX]  = {{0, 3, 7, 4}, 0.55f, 0.0f, 0.5f, 0.5f},
    [RS_LINES_FACE_PY]  = {{3, 2, 6, 7}, 0.85f, 0.5f, 1.0f, 0.5f},
    [RS_LINES_FACE_NY]  = {{0, 1, 5, 4}, 0.55f, 0.5f, 0.0f, 0.5f},
};

static const float g_lines_cube_corners[8][3] = {
    {0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
    {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1},
};

typedef struct {
    float wx, wy, gz;
    float cx, cy, cz;
    RSLinesFaceAxis axis;
    Uint8 r, g, b;
    float depth;
} RSLinesFace;

typedef struct {
    float wx, wy;
    float base_z;
    float float_amp;
    float float_speed;
    float phase;
    float drift_r;
    int shape_variant;
    float depth;
    float screen_ax, screen_ay;
    float screen_x, screen_y;
} RSLinesAnomaly;

typedef struct {
    float depth;
    SDL_bool is_anomaly;
    int index;
} RSLinesDrawItem;

static int g_lines_heights[RS_LINES_MAX_COLUMNS];
static Uint8 g_lines_colors_r[RS_LINES_MAX_COLUMNS][RS_LINES_MAX_COLUMN_HEIGHT];
static Uint8 g_lines_colors_g[RS_LINES_MAX_COLUMNS][RS_LINES_MAX_COLUMN_HEIGHT];
static Uint8 g_lines_colors_b[RS_LINES_MAX_COLUMNS][RS_LINES_MAX_COLUMN_HEIGHT];

static RSLinesFace g_lines_faces[RS_LINES_MAX_EXPOSED_FACES];
static int g_lines_face_count;

static RSLinesAnomaly g_lines_anomalies[RS_LINES_MAX_ANOMALIES];
static int g_lines_anomaly_count;

static RSLinesDrawItem g_lines_draw_items[RS_LINES_MAX_EXPOSED_FACES + RS_LINES_MAX_ANOMALIES];
static int g_lines_current_grid_n;

static const BenchShapeType g_lines_anomaly_shapes[3] = {
    SHAPE_TETRAHEDRON, SHAPE_OCTAHEDRON, SHAPE_PENTAGONAL_PRISM,
};

static inline int rs_lines_clampi(int value, int min_val, int max_val)
{
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

static inline float rs_lines_clampf(float value, float min_val, float max_val)
{
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

static Uint8 rs_lines_hsl_channel(float l, float a, float n, float h)
{
    const float k = fmodf(n + h / 30.0f, 12.0f);
    float t = fminf(k - 3.0f, 9.0f - k);
    t = fminf(t, 1.0f);
    t = fmaxf(t, -1.0f);
    const float channel = rs_lines_clampf(l - a * t, 0.0f, 1.0f);
    return (Uint8)(channel * 255.0f + 0.5f);
}

static void rs_lines_hsl_to_rgb(float h, float s_pct, float l_pct, Uint8 *out_r, Uint8 *out_g, Uint8 *out_b)
{
    const float s = s_pct / 100.0f;
    const float l = l_pct / 100.0f;
    const float a = s * fminf(l, 1.0f - l);
    *out_r = rs_lines_hsl_channel(l, a, 0.0f, h);
    *out_g = rs_lines_hsl_channel(l, a, 8.0f, h);
    *out_b = rs_lines_hsl_channel(l, a, 4.0f, h);
}

static inline int rs_lines_height_at(const int *heights, int grid_n, int gx, int gy)
{
    if (gx < 0 || gx >= grid_n || gy < 0 || gy >= grid_n) {
        return 0;
    }
    return heights[gx * grid_n + gy];
}

static void rs_lines_add_face(float wx, float wy, int gz, RSLinesFaceAxis axis, Uint8 r, Uint8 g, Uint8 b)
{
    if (g_lines_face_count >= RS_LINES_MAX_EXPOSED_FACES) {
        return;
    }
    const RSLinesFaceDef *def = &g_lines_face_defs[axis];
    RSLinesFace *face = &g_lines_faces[g_lines_face_count++];
    face->wx = wx;
    face->wy = wy;
    face->gz = (float)gz;
    face->axis = axis;
    face->r = r;
    face->g = g;
    face->b = b;
    face->cx = wx + def->ox;
    face->cy = wy + def->oy;
    face->cz = (float)gz + def->oz;
}

static float rs_lines_randf(void)
{
    return (float)rand() / (float)RAND_MAX;
}

static void rs_lines_build_grid(int grid_n, float avg_height)
{
    g_lines_current_grid_n = grid_n;
    for (int i = 0; i < RS_LINES_MAX_COLUMNS; i++) {
        g_lines_heights[i] = 0;
    }

    for (int gx = 0; gx < grid_n; gx++) {
        for (int gy = 0; gy < grid_n; gy++) {
            const int col = gx * grid_n + gy;
            const float jitter = (rs_lines_randf() - 0.5f) * avg_height * 1.3f;
            const int height = rs_lines_clampi((int)(avg_height + jitter + 0.5f), 1, RS_LINES_MAX_COLUMN_HEIGHT);
            g_lines_heights[col] = height;
            for (int level = 0; level < height; level++) {
                Uint8 r, g, b;
                rs_lines_hsl_to_rgb(rs_lines_randf() * 360.0f,
                                    55.0f + rs_lines_randf() * 20.0f,
                                    52.0f + rs_lines_randf() * 14.0f,
                                    &r, &g, &b);
                g_lines_colors_r[col][level] = r;
                g_lines_colors_g[col][level] = g;
                g_lines_colors_b[col][level] = b;
            }
        }
    }

    const float offset = (float)(grid_n - 1) * 0.5f;
    g_lines_face_count = 0;

    for (int gx = 0; gx < grid_n; gx++) {
        for (int gy = 0; gy < grid_n; gy++) {
            const int col = gx * grid_n + gy;
            const int height = g_lines_heights[col];
            const float wx = (float)gx - offset;
            const float wy = (float)gy - offset;

            for (int level = 0; level < height; level++) {
                const Uint8 r = g_lines_colors_r[col][level];
                const Uint8 g = g_lines_colors_g[col][level];
                const Uint8 b = g_lines_colors_b[col][level];

                if (level + 1 >= height) {
                    rs_lines_add_face(wx, wy, level, RS_LINES_FACE_TOP, r, g, b);
                }
                if (rs_lines_height_at(g_lines_heights, grid_n, gx + 1, gy) <= level) {
                    rs_lines_add_face(wx, wy, level, RS_LINES_FACE_PX, r, g, b);
                }
                if (rs_lines_height_at(g_lines_heights, grid_n, gx - 1, gy) <= level) {
                    rs_lines_add_face(wx, wy, level, RS_LINES_FACE_NX, r, g, b);
                }
                if (rs_lines_height_at(g_lines_heights, grid_n, gx, gy + 1) <= level) {
                    rs_lines_add_face(wx, wy, level, RS_LINES_FACE_PY, r, g, b);
                }
                if (rs_lines_height_at(g_lines_heights, grid_n, gx, gy - 1) <= level) {
                    rs_lines_add_face(wx, wy, level, RS_LINES_FACE_NY, r, g, b);
                }
            }
        }
    }
}

static void rs_lines_build_anomalies(int grid_n)
{
    g_lines_anomaly_count = 0;

    for (int gy = 0; gy < grid_n && g_lines_anomaly_count < RS_LINES_MAX_ANOMALIES; gy++) {
        const int gx = rand() % grid_n;
        const int col = gx * grid_n + gy;
        const int height = g_lines_heights[col];
        const float offset = (float)(grid_n - 1) * 0.5f;

        RSLinesAnomaly *anomaly = &g_lines_anomalies[g_lines_anomaly_count++];
        anomaly->wx = (float)gx - offset;
        anomaly->wy = (float)gy - offset;
        anomaly->base_z = (float)height;
        anomaly->float_amp = 1.1f + rs_lines_randf() * 0.9f;
        anomaly->float_speed = 0.4f + rs_lines_randf() * 0.4f;
        anomaly->phase = rs_lines_randf() * RS_LINES_TWO_PI;
        anomaly->drift_r = 0.12f + rs_lines_randf() * 0.14f;
        anomaly->shape_variant = gy % 3;
    }
}

static inline void rs_lines_iso_project(float gx, float gy, float gz, float sin_r, float cos_r,
                                        float *out_x, float *out_y)
{
    const float rx = gx * cos_r - gy * sin_r;
    const float ry = gx * sin_r + gy * cos_r;
    *out_x = rx - ry;
    *out_y = (rx + ry) * 0.5f - gz;
}

static inline float rs_lines_depth(float gx, float gy, float gz, float sin_r, float cos_r)
{
    const float rx = gx * cos_r - gy * sin_r;
    const float ry = gx * sin_r + gy * cos_r;
    return rx + ry + gz;
}

static inline SDL_bool rs_lines_face_is_backface(RSLinesFaceAxis axis, float a, float b)
{
    switch (axis) {
        case RS_LINES_FACE_PX: return a <= 0.0f;
        case RS_LINES_FACE_NX: return a >= 0.0f;
        case RS_LINES_FACE_PY: return b <= 0.0f;
        case RS_LINES_FACE_NY: return b >= 0.0f;
        default: return SDL_FALSE;
    }
}

static int rs_lines_draw_item_cmp(const void *a, const void *b)
{
    const RSLinesDrawItem *ia = (const RSLinesDrawItem *)a;
    const RSLinesDrawItem *ib = (const RSLinesDrawItem *)b;
    if (ia->depth < ib->depth) return -1;
    if (ia->depth > ib->depth) return 1;
    return 0;
}

static void rs_lines_draw_face(SDL_Renderer *renderer, const RSLinesFace *face,
                               float origin_x, float origin_y, float px_size,
                               float sin_r, float cos_r, SDL_bool wireframe, BenchMetrics *metrics)
{
    const RSLinesFaceDef *def = &g_lines_face_defs[face->axis];
    float screen_x[4], screen_y[4];

    for (int k = 0; k < 4; k++) {
        const float *corner = g_lines_cube_corners[def->corner_idx[k]];
        float px, py;
        rs_lines_iso_project(face->wx + corner[0], face->wy + corner[1], face->gz + corner[2],
                             sin_r, cos_r, &px, &py);
        screen_x[k] = origin_x + px * px_size;
        screen_y[k] = origin_y + py * px_size;
    }

    const Uint8 r = (Uint8)rs_lines_clampf((float)face->r * def->shade, 0.0f, 255.0f);
    const Uint8 g = (Uint8)rs_lines_clampf((float)face->g * def->shade, 0.0f, 255.0f);
    const Uint8 b = (Uint8)rs_lines_clampf((float)face->b * def->shade, 0.0f, 255.0f);

    if (wireframe) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, r, g, b, 180);
        for (int k = 0; k < 4; k++) {
            const int next = (k + 1) % 4;
            SDL_RenderDrawLineF(renderer, screen_x[k], screen_y[k], screen_x[next], screen_y[next]);
        }
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        if (metrics) {
            metrics->draw_calls += 4;
            metrics->vertices_rendered += 8;
        }
        return;
    }

    SDL_Vertex vertices[4];
    for (int k = 0; k < 4; k++) {
        vertices[k].position.x = screen_x[k];
        vertices[k].position.y = screen_y[k];
        vertices[k].color.r = r;
        vertices[k].color.g = g;
        vertices[k].color.b = b;
        vertices[k].color.a = 255;
        vertices[k].tex_coord.x = 0.0f;
        vertices[k].tex_coord.y = 0.0f;
    }

    static const int indices[6] = {0, 1, 2, 0, 2, 3};
    SDL_RenderGeometry(renderer, NULL, vertices, 4, indices, 6);

    if (metrics) {
        metrics->draw_calls++;
        metrics->vertices_rendered += 4;
        metrics->triangles_rendered += 2;
        metrics->geometry_batches++;
    }
}

static void rs_lines_draw_anomaly(SDL_Renderer *renderer, const RSLinesAnomaly *anomaly,
                                  SDL_Color color, float bob, BenchMetrics *metrics)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 140);
    SDL_RenderDrawLineF(renderer, anomaly->screen_ax, anomaly->screen_ay,
                        anomaly->screen_x, anomaly->screen_y);
    if (metrics) {
        metrics->draw_calls++;
        metrics->vertices_rendered += 2;
    }

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 230);
    SDL_RenderDrawPointF(renderer, anomaly->screen_ax, anomaly->screen_ay);
    if (metrics) {
        metrics->draw_calls++;
        metrics->vertices_rendered++;
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    const float size = 8.0f + bob * 6.0f;
    const float rotation = anomaly->phase;
    bench_render_shape(g_lines_anomaly_shapes[anomaly->shape_variant], renderer, metrics,
                       rotation, anomaly->screen_x, anomaly->screen_y, size, 0);
}

void rs_scene_lines_init(RenderSuiteState *state, SDL_Renderer *renderer)
{
    (void)renderer;
    g_lines_face_count = 0;
    g_lines_anomaly_count = 0;
    g_lines_current_grid_n = 0;
    if (state) {
        state->lines_grid_n = -1;
    }
}

void rs_scene_lines_cleanup(RenderSuiteState *state)
{
    (void)state;
    g_lines_face_count = 0;
    g_lines_anomaly_count = 0;
    g_lines_current_grid_n = 0;
}

int rs_scene_lines_cube_count(void)
{
    int total = 0;
    const int column_count = g_lines_current_grid_n * g_lines_current_grid_n;
    for (int i = 0; i < column_count; i++) {
        total += g_lines_heights[i];
    }
    return total;
}

int rs_scene_lines_anomaly_count(void)
{
    return g_lines_anomaly_count;
}

void rs_scene_lines(RenderSuiteState *state,
                    SDL_Renderer *renderer,
                    BenchMetrics *metrics,
                    double delta_seconds)
{
    if (!state || !renderer) {
        return;
    }

    const float factor = rs_state_stress_factor(state);
    const int region_height = SDL_max(1, bench_logical_h() - (int)state->top_margin);
    const float origin_x = bench_logical_w() * 0.5f;
    const float origin_y = state->top_margin + (float)region_height * 0.72f;

    const int grid_n = rs_lines_clampi((int)lroundf(2.0f + (float)state->stress_level), 3, RS_LINES_MAX_GRID_N);
    if (grid_n != state->lines_grid_n) {
        const float avg_height = 1.0f + (float)state->stress_level * 1.1f;
        rs_lines_build_grid(grid_n, avg_height);
        rs_lines_build_anomalies(grid_n);
        state->lines_grid_n = grid_n;
    }

    state->lines_rotation += (float)(delta_seconds * (0.1f + factor * 0.05f));
    state->lines_phase += (float)delta_seconds;

    const Uint64 perf_freq = SDL_GetPerformanceFrequency();
    const Uint64 transform_start = SDL_GetPerformanceCounter();

    const float sin_r = sinf(state->lines_rotation);
    const float cos_r = cosf(state->lines_rotation);
    const float px_size = fminf((float)bench_logical_w(), (float)region_height) * (0.27f / (float)grid_n);

    const float cull_a = cos_r + sin_r;
    const float cull_b = cos_r - sin_r;
    const SDL_bool apply_backface_cull = state->lines_backface_cull && !state->lines_wireframe;

    int item_count = 0;
    for (int i = 0; i < g_lines_face_count; i++) {
        RSLinesFace *face = &g_lines_faces[i];
        if (apply_backface_cull && rs_lines_face_is_backface(face->axis, cull_a, cull_b)) {
            continue;
        }
        face->depth = rs_lines_depth(face->cx, face->cy, face->cz, sin_r, cos_r);
        g_lines_draw_items[item_count].depth = face->depth;
        g_lines_draw_items[item_count].is_anomaly = SDL_FALSE;
        g_lines_draw_items[item_count].index = i;
        item_count++;
    }

    if (state->lines_anomalies_visible) {
        for (int i = 0; i < g_lines_anomaly_count; i++) {
            RSLinesAnomaly *anomaly = &g_lines_anomalies[i];
            const float bob = 0.5f + 0.5f * sinf(state->lines_phase * anomaly->float_speed + anomaly->phase);
            const float float_z = anomaly->base_z + 1.0f + anomaly->float_amp * bob;
            const float drift_wx = anomaly->wx + cosf(state->lines_phase * 0.3f + anomaly->phase) * anomaly->drift_r;
            const float drift_wy = anomaly->wy + sinf(state->lines_phase * 0.3f + anomaly->phase) * anomaly->drift_r;

            float anchor_x, anchor_y, pos_x, pos_y;
            rs_lines_iso_project(anomaly->wx + 0.5f, anomaly->wy + 0.5f, anomaly->base_z, sin_r, cos_r,
                                 &anchor_x, &anchor_y);
            rs_lines_iso_project(drift_wx + 0.5f, drift_wy + 0.5f, float_z, sin_r, cos_r, &pos_x, &pos_y);

            anomaly->screen_ax = origin_x + anchor_x * px_size;
            anomaly->screen_ay = origin_y + anchor_y * px_size;
            anomaly->screen_x = origin_x + pos_x * px_size;
            anomaly->screen_y = origin_y + pos_y * px_size;

            anomaly->depth = rs_lines_depth(drift_wx + 0.5f, drift_wy + 0.5f, float_z, sin_r, cos_r);

            g_lines_draw_items[item_count].depth = anomaly->depth;
            g_lines_draw_items[item_count].is_anomaly = SDL_TRUE;
            g_lines_draw_items[item_count].index = i;
            item_count++;
        }
    }

    const Uint64 transform_end = SDL_GetPerformanceCounter();
    metrics->stage_transform_ms = (double)(transform_end - transform_start) * 1000.0 / (double)perf_freq;

    qsort(g_lines_draw_items, (size_t)item_count, sizeof(RSLinesDrawItem), rs_lines_draw_item_cmp);

    const Uint64 sort_end = SDL_GetPerformanceCounter();
    metrics->stage_sort_ms = (double)(sort_end - transform_end) * 1000.0 / (double)perf_freq;

    static const SDL_Color anomaly_colors[3] = {
        {51, 200, 255, 255}, {51, 255, 160, 255}, {240, 194, 94, 255},
    };

    for (int i = 0; i < item_count; i++) {
        const RSLinesDrawItem *item = &g_lines_draw_items[i];
        if (item->is_anomaly) {
            const RSLinesAnomaly *anomaly = &g_lines_anomalies[item->index];
            const float bob = 0.5f + 0.5f * sinf(state->lines_phase * anomaly->float_speed + anomaly->phase);
            rs_lines_draw_anomaly(renderer, anomaly, anomaly_colors[anomaly->shape_variant], bob, metrics);
        } else {
            rs_lines_draw_face(renderer, &g_lines_faces[item->index], origin_x, origin_y, px_size,
                               sin_r, cos_r, state->lines_wireframe, metrics);
        }
    }

    metrics->stage_draw_ms = (double)(SDL_GetPerformanceCounter() - sort_end) * 1000.0 / (double)perf_freq;
}
