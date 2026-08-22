#include "render_suite/scenes/geometry.h"
#include "render_suite/render_neon.h"

#include <math.h>
#include <stdlib.h>

#define RS_PI 3.14159265358979323846f
#define MAX_TRIANGLES 500
#define MAX_STAR_PARTICLES 200
#define MAX_VERTICES (MAX_TRIANGLES * 3)

typedef struct {
    float x, y, z;
} Vector3;

typedef struct {
    float sx, cx;
    float sy, cy;
    float sz, cz;
} RotationTrig;

/* Vertex positions and per-triangle color, stored SoA so rotation/projection
 * can be batched 4-wide with NEON instead of walking an array of structs. */
typedef struct {
    SDL_bool valid;
    int subdivisions;
    int triangle_count;
    float vx[MAX_VERTICES];
    float vy[MAX_VERTICES];
    float vz[MAX_VERTICES];
    Uint8 r[MAX_TRIANGLES], g[MAX_TRIANGLES], b[MAX_TRIANGLES], a[MAX_TRIANGLES];
} TriangleCache;

static TriangleCache g_triangle_cache[6];

/* Star field state, SoA: position/velocity/life are batch-updated with NEON,
 * color stays per-particle since respawn assigns it via scalar rand(). */
typedef struct {
    float x[MAX_STAR_PARTICLES];
    float y[MAX_STAR_PARTICLES];
    float dx[MAX_STAR_PARTICLES];
    float dy[MAX_STAR_PARTICLES];
    float life[MAX_STAR_PARTICLES];
    Uint8 r[MAX_STAR_PARTICLES], g[MAX_STAR_PARTICLES], b[MAX_STAR_PARTICLES], a[MAX_STAR_PARTICLES];
} StarField;

static inline float rs_clampf(float value, float min_val, float max_val)
{
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

static inline int rs_clampi(int value, int min_val, int max_val)
{
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

static RotationTrig rs_build_rotation_trig(const RenderSuiteState *state,
                                           float rx,
                                           float ry,
                                           float rz)
{
    RotationTrig trig;
    trig.sx = rs_state_sin_rad(state, rx);
    trig.cx = rs_state_cos_rad(state, rx);
    trig.sy = rs_state_sin_rad(state, ry);
    trig.cy = rs_state_cos_rad(state, ry);
    trig.sz = rs_state_sin_rad(state, rz);
    trig.cz = rs_state_cos_rad(state, rz);
    return trig;
}

#if RS_HAS_NEON
/* Newton-Raphson refined reciprocal: ARMv7 NEON has no vdivq_f32. */
static inline float32x4_t rs_neon_recip_f32(float32x4_t d)
{
    float32x4_t r = vrecpeq_f32(d);
    r = vmulq_f32(vrecpsq_f32(d, r), r);
    r = vmulq_f32(vrecpsq_f32(d, r), r);
    return r;
}
#endif

/* Rotates and perspective-projects `count` vertices in one pass. Combines
 * what was previously rs_rotate_vector() + rs_project_vertex() per-vertex
 * into a single SIMD-batched sweep over SoA position arrays. */
static void rs_rotate_project_batch(const float *vx, const float *vy, const float *vz,
                                    int count, const RotationTrig *rot,
                                    float center_x, float center_y, float scale,
                                    float *out_x, float *out_y)
{
    int i = 0;

#if RS_HAS_NEON
    const float32x4_t cx = vdupq_n_f32(rot->cx);
    const float32x4_t sx = vdupq_n_f32(rot->sx);
    const float32x4_t cy = vdupq_n_f32(rot->cy);
    const float32x4_t sy = vdupq_n_f32(rot->sy);
    const float32x4_t cz = vdupq_n_f32(rot->cz);
    const float32x4_t sz = vdupq_n_f32(rot->sz);
    const float32x4_t vcenter_x = vdupq_n_f32(center_x);
    const float32x4_t vcenter_y = vdupq_n_f32(center_y);
    const float32x4_t vscale = vdupq_n_f32(scale);
    const float32x4_t vone = vdupq_n_f32(1.0f);
    const float32x4_t vpersp = vdupq_n_f32(0.001f);

    for (; i + 4 <= count; i += 4) {
        float32x4_t x0 = vld1q_f32(vx + i);
        float32x4_t y0 = vld1q_f32(vy + i);
        float32x4_t z0 = vld1q_f32(vz + i);

        /* X axis rotation */
        float32x4_t y1 = vsubq_f32(vmulq_f32(y0, cx), vmulq_f32(z0, sx));
        float32x4_t z1 = vaddq_f32(vmulq_f32(y0, sx), vmulq_f32(z0, cx));

        /* Y axis rotation */
        float32x4_t x2 = vaddq_f32(vmulq_f32(x0, cy), vmulq_f32(z1, sy));
        float32x4_t z2 = vaddq_f32(vnegq_f32(vmulq_f32(x0, sy)), vmulq_f32(z1, cy));

        /* Z axis rotation */
        float32x4_t x3 = vsubq_f32(vmulq_f32(x2, cz), vmulq_f32(y1, sz));
        float32x4_t y3 = vaddq_f32(vmulq_f32(x2, sz), vmulq_f32(y1, cz));

        /* Perspective projection */
        float32x4_t denom = vaddq_f32(vone, vmulq_f32(z2, vpersp));
        float32x4_t persp = rs_neon_recip_f32(denom);

        float32x4_t px = vaddq_f32(vcenter_x, vmulq_f32(vmulq_f32(x3, vscale), persp));
        float32x4_t py = vaddq_f32(vcenter_y, vmulq_f32(vmulq_f32(y3, vscale), persp));

        vst1q_f32(out_x + i, px);
        vst1q_f32(out_y + i, py);
    }
#endif

    for (; i < count; i++) {
        float x = vx[i], y = vy[i], z = vz[i];

        float y1 = y * rot->cx - z * rot->sx;
        float z1 = y * rot->sx + z * rot->cx;

        float x2 = x * rot->cy + z1 * rot->sy;
        float z2 = -x * rot->sy + z1 * rot->cy;

        float x3 = x2 * rot->cz - y1 * rot->sz;
        float y3 = x2 * rot->sz + y1 * rot->cz;

        const float perspective = 1.0f / (1.0f + z2 * 0.001f);
        out_x[i] = center_x + x3 * scale * perspective;
        out_y[i] = center_y + y3 * scale * perspective;
    }
}

static void rs_create_cube_triangles(TriangleCache *cache, float size, int subdivisions)
{
    int count = 0;
    const float step = size / (float)subdivisions;

    for (int face = 0; face < 6 && count < MAX_TRIANGLES - 12; face++) {
        for (int i = 0; i < subdivisions && count < MAX_TRIANGLES - 2; i++) {
            for (int j = 0; j < subdivisions && count < MAX_TRIANGLES - 2; j++) {
                float x1 = -size + i * step;
                float y1 = -size + j * step;
                float x2 = x1 + step;
                float y2 = y1 + step;

                Vector3 v1, v2, v3, v4;

                switch (face) {
                    case 0: // Front face
                        v1 = (Vector3){x1, y1, size};
                        v2 = (Vector3){x2, y1, size};
                        v3 = (Vector3){x2, y2, size};
                        v4 = (Vector3){x1, y2, size};
                        break;
                    case 1: // Back face
                        v1 = (Vector3){x2, y1, -size};
                        v2 = (Vector3){x1, y1, -size};
                        v3 = (Vector3){x1, y2, -size};
                        v4 = (Vector3){x2, y2, -size};
                        break;
                    case 2: // Left face
                        v1 = (Vector3){-size, y1, x2};
                        v2 = (Vector3){-size, y1, x1};
                        v3 = (Vector3){-size, y2, x1};
                        v4 = (Vector3){-size, y2, x2};
                        break;
                    case 3: // Right face
                        v1 = (Vector3){size, y1, x1};
                        v2 = (Vector3){size, y1, x2};
                        v3 = (Vector3){size, y2, x2};
                        v4 = (Vector3){size, y2, x1};
                        break;
                    case 4: // Top face
                        v1 = (Vector3){x1, size, y2};
                        v2 = (Vector3){x2, size, y2};
                        v3 = (Vector3){x2, size, y1};
                        v4 = (Vector3){x1, size, y1};
                        break;
                    case 5: // Bottom face
                        v1 = (Vector3){x1, -size, y1};
                        v2 = (Vector3){x2, -size, y1};
                        v3 = (Vector3){x2, -size, y2};
                        v4 = (Vector3){x1, -size, y2};
                        break;
                }

                // Two triangles per quad, written directly into SoA vertex arrays.
                const int base0 = count * 3;
                cache->vx[base0 + 0] = v1.x; cache->vy[base0 + 0] = v1.y; cache->vz[base0 + 0] = v1.z;
                cache->vx[base0 + 1] = v2.x; cache->vy[base0 + 1] = v2.y; cache->vz[base0 + 1] = v2.z;
                cache->vx[base0 + 2] = v3.x; cache->vy[base0 + 2] = v3.y; cache->vz[base0 + 2] = v3.z;
                cache->r[count] = (Uint8)(128 + face * 20);
                cache->g[count] = (Uint8)(100 + (i + j) * 8);
                cache->b[count] = (Uint8)(200 - face * 15);
                cache->a[count] = 255;
                count++;

                const int base1 = count * 3;
                cache->vx[base1 + 0] = v1.x; cache->vy[base1 + 0] = v1.y; cache->vz[base1 + 0] = v1.z;
                cache->vx[base1 + 1] = v3.x; cache->vy[base1 + 1] = v3.y; cache->vz[base1 + 1] = v3.z;
                cache->vx[base1 + 2] = v4.x; cache->vy[base1 + 2] = v4.y; cache->vz[base1 + 2] = v4.z;
                cache->r[count] = (Uint8)(128 + face * 20);
                cache->g[count] = (Uint8)(100 + (i + j) * 8);
                cache->b[count] = (Uint8)(200 - face * 15);
                cache->a[count] = 255;
                count++;
            }
        }
    }

    cache->triangle_count = count;
}

static TriangleCache *rs_get_cached_cube(int subdivisions, int *triangle_count)
{
    int clamped = rs_clampi(subdivisions, 1, 6);
    TriangleCache *cache = &g_triangle_cache[clamped - 1];

    if (!cache->valid || cache->subdivisions != clamped) {
        rs_create_cube_triangles(cache, 50.0f, clamped);
        cache->subdivisions = clamped;
        cache->valid = SDL_TRUE;
    }

    if (triangle_count) {
        *triangle_count = cache->triangle_count;
    }
    return cache;
}

/* Batched position/life update for `count` particles. Respawn is still a
 * per-particle scalar pass below: it's branchy (bounds/life check) and uses
 * rand(), a poor fit for SIMD. */
static void rs_update_star_field(StarField *field, int count,
                                 float delta_seconds, float center_x, float center_y,
                                 const RenderSuiteState *state)
{
    int i = 0;
    const float step = delta_seconds * 60.0f;

#if RS_HAS_NEON
    const float32x4_t vstep = vdupq_n_f32(step);
    const float32x4_t vdt = vdupq_n_f32(delta_seconds);

    for (; i + 4 <= count; i += 4) {
        float32x4_t x = vld1q_f32(field->x + i);
        float32x4_t y = vld1q_f32(field->y + i);
        float32x4_t dx = vld1q_f32(field->dx + i);
        float32x4_t dy = vld1q_f32(field->dy + i);
        float32x4_t life = vld1q_f32(field->life + i);

        x = vaddq_f32(x, vmulq_f32(dx, vstep));
        y = vaddq_f32(y, vmulq_f32(dy, vstep));
        life = vsubq_f32(life, vdt);

        vst1q_f32(field->x + i, x);
        vst1q_f32(field->y + i, y);
        vst1q_f32(field->life + i, life);
    }
#endif

    for (; i < count; i++) {
        field->x[i] += field->dx[i] * step;
        field->y[i] += field->dy[i] * step;
        field->life[i] -= delta_seconds;
    }

    for (i = 0; i < count; i++) {
        if (field->x[i] < 0 || field->x[i] > bench_logical_w() ||
            field->y[i] < 0 || field->y[i] > bench_logical_h() || field->life[i] <= 0.0f) {

            field->x[i] = center_x + ((float)rand() / RAND_MAX - 0.5f) * 20.0f;
            field->y[i] = center_y + ((float)rand() / RAND_MAX - 0.5f) * 20.0f;

            float angle = (float)rand() / RAND_MAX * 2.0f * RS_PI;
            float speed = 20.0f + (float)rand() / RAND_MAX * 80.0f;
            field->dx[i] = rs_state_cos_rad(state, angle) * speed;
            field->dy[i] = rs_state_sin_rad(state, angle) * speed;

            field->r[i] = (Uint8)(200 + rand() % 56);
            field->g[i] = (Uint8)(200 + rand() % 56);
            field->b[i] = (Uint8)(100 + rand() % 156);
            field->a[i] = 255;
            field->life[i] = 1.0f + (float)rand() / RAND_MAX * 3.0f;
        }
    }
}

static void rs_render_star_field(SDL_Renderer *renderer, const StarField *field,
                                 int count, BenchMetrics *metrics)
{
    for (int i = 0; i < count; i++) {
        Uint8 alpha = (Uint8)(field->a[i] * rs_clampf(field->life[i], 0.0f, 1.0f));

        SDL_SetRenderDrawColor(renderer, field->r[i], field->g[i], field->b[i], alpha);
        SDL_RenderDrawPointF(renderer, field->x[i], field->y[i]);

        if (metrics) {
            metrics->draw_calls++;
            metrics->vertices_rendered++;
        }
    }
}

static void rs_render_triangles_cpu(SDL_Renderer *renderer,
                                    const TriangleCache *cache,
                                    const float *proj_x, const float *proj_y,
                                    int triangle_count,
                                    BenchMetrics *metrics)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    for (int i = 0; i < triangle_count; i++) {
        SDL_Vertex vertices[3];
        for (int j = 0; j < 3; j++) {
            const int idx = i * 3 + j;
            vertices[j].position.x = proj_x[idx];
            vertices[j].position.y = proj_y[idx];
            vertices[j].color.r = cache->r[i];
            vertices[j].color.g = cache->g[i];
            vertices[j].color.b = cache->b[i];
            vertices[j].color.a = cache->a[i];
            vertices[j].tex_coord.x = 0.0f;
            vertices[j].tex_coord.y = 0.0f;
        }

        SDL_RenderGeometry(renderer, NULL, vertices, 3, NULL, 0);

        if (metrics) {
            metrics->draw_calls++;
            metrics->vertices_rendered += 3;
            metrics->triangles_rendered++;
            metrics->geometry_batches++;
        }
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

static void rs_render_triangle_points(SDL_Renderer *renderer,
                                      const TriangleCache *cache,
                                      const float *proj_x, const float *proj_y,
                                      int triangle_count,
                                      BenchMetrics *metrics)
{
    if (!renderer || !cache || triangle_count <= 0) {
        return;
    }

    for (int i = 0; i < triangle_count; ++i) {
        SDL_SetRenderDrawColor(renderer, cache->r[i], cache->g[i], cache->b[i], cache->a[i]);

        for (int j = 0; j < 3; ++j) {
            const int idx = i * 3 + j;
            SDL_RenderDrawPointF(renderer, proj_x[idx], proj_y[idx]);

            if (metrics) {
                metrics->draw_calls++;
                metrics->vertices_rendered++;
            }
        }
    }
}

void rs_scene_geometry(RenderSuiteState *state,
                       SDL_Renderer *renderer,
                       BenchMetrics *metrics,
                       double delta_seconds)
{
    if (!state || !renderer) {
        return;
    }

    const float factor = rs_state_stress_factor(state);
    const int region_height = SDL_max(1, bench_logical_h() - (int)state->top_margin);
    const float center_x = bench_logical_w() * 0.5f;
    const float center_y = (float)state->top_margin + (float)region_height * 0.5f;

    // Update rotation
    state->geometry_rotation += (float)(delta_seconds * (0.5f + factor * 0.5f));
    state->geometry_phase += (float)(delta_seconds * 2.0f);

    // Calculate triangle count based on stress level
    const int base_triangles = 24; // Start with basic cube
    const int max_triangles = rs_clampi((int)(base_triangles + factor * 100), base_triangles, MAX_TRIANGLES);
    state->geometry_triangle_count = max_triangles;

    // Create cube with tessellation based on triangle count
    const int subdivisions = rs_clampi((int)sqrtf((float)max_triangles / 12.0f), 1, 6);
    int cached_triangle_count = 0;
    TriangleCache *cache = rs_get_cached_cube(subdivisions, &cached_triangle_count);
    const int triangle_count = rs_clampi(cached_triangle_count, 0, max_triangles);

    // Create and update star field
    static StarField star_field;
    static SDL_bool particles_initialized = SDL_FALSE;

    if (!particles_initialized) {
        for (int i = 0; i < MAX_STAR_PARTICLES; i++) {
            star_field.life[i] = 0.0f; // Force regeneration
        }
        particles_initialized = SDL_TRUE;
    }

    const int particle_count = rs_clampi((int)(50 + factor * 150), 50, MAX_STAR_PARTICLES);
    rs_update_star_field(&star_field,
                         particle_count,
                         (float)delta_seconds,
                         center_x,
                         center_y,
                         state);

    // Render star field background
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
    rs_render_star_field(renderer, &star_field, particle_count, metrics);

    const float scale = 80.0f + 40.0f * rs_state_sin(state, state->geometry_phase * 10.0f);
    RotationTrig rotation_trig = rs_build_rotation_trig(state,
                                                       state->geometry_rotation,
                                                       state->geometry_rotation * 0.7f,
                                                       state->geometry_rotation * 0.3f);

    static float proj_x[MAX_VERTICES];
    static float proj_y[MAX_VERTICES];
    const int vertex_count = triangle_count * 3;
    rs_rotate_project_batch(cache->vx, cache->vy, cache->vz, vertex_count, &rotation_trig,
                            center_x, center_y, scale, proj_x, proj_y);

    const RSGeometryRenderMode render_mode =
        (RSGeometryRenderMode)(state->geometry_render_mode % RS_GEOMETRY_RENDER_MODE_MAX);

    if (render_mode == RS_GEOMETRY_RENDER_FILLED) {
        rs_render_triangles_cpu(renderer, cache, proj_x, proj_y, triangle_count, metrics);
    } else if (render_mode == RS_GEOMETRY_RENDER_POINTS) {
        rs_render_triangle_points(renderer, cache, proj_x, proj_y, triangle_count, metrics);
    } else { // wireframe as default fallback
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 128);

        for (int i = 0; i < triangle_count; i++) {
            const int idx = i * 3;

            SDL_RenderDrawLineF(renderer, proj_x[idx + 0], proj_y[idx + 0], proj_x[idx + 1], proj_y[idx + 1]);
            SDL_RenderDrawLineF(renderer, proj_x[idx + 1], proj_y[idx + 1], proj_x[idx + 2], proj_y[idx + 2]);
            SDL_RenderDrawLineF(renderer, proj_x[idx + 2], proj_y[idx + 2], proj_x[idx + 0], proj_y[idx + 0]);

            if (metrics) {
                metrics->draw_calls += 3;
                metrics->vertices_rendered += 6;
            }
        }

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }
}
