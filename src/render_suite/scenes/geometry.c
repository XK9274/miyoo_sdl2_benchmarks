#include "render_suite/scenes/geometry.h"
#include "render_suite/render_neon.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define RS_PI 3.14159265358979323846f
#define ICOS_PHI 1.61803398875f
#define MAX_TRIANGLES 320
#define MAX_STAR_PARTICLES 200
#define MAX_VERTICES (MAX_TRIANGLES * 3)
#define ICOS_SUBDIVISION_MAX 2
#define MAX_EMITTER_PARTICLES 100
#define EMITTER_TRAIL_SAMPLES 5
#define EMITTER_TRAIL_STEP 0.06f
#define MAX_PROJECTED (MAX_VERTICES + MAX_EMITTER_PARTICLES * EMITTER_TRAIL_SAMPLES)

typedef struct {
    float x, y, z;
} Vector3;

typedef struct {
    float sx, cx;
    float sy, cy;
    float sz, cz;
} RotationTrig;

/* Vertex positions and per-triangle color, stored SoA so rotation/projection
 * can be batched 4-wide with NEON instead of walking an array of structs.
 * dir_x/y/z is each vertex's parent top-level face's outward centroid
 * direction, applied as the explosion offset before rotation every frame. */
typedef struct {
    SDL_bool valid;
    int subdivisions;
    int triangle_count;
    float base_vx[MAX_VERTICES];
    float base_vy[MAX_VERTICES];
    float base_vz[MAX_VERTICES];
    float dir_x[MAX_VERTICES];
    float dir_y[MAX_VERTICES];
    float dir_z[MAX_VERTICES];
    Uint8 r[MAX_TRIANGLES], g[MAX_TRIANGLES], b[MAX_TRIANGLES], a[MAX_TRIANGLES];
} TriangleCache;

static TriangleCache g_triangle_cache[ICOS_SUBDIVISION_MAX + 1];

static const float g_icos_base[12][3] = {
    {-1.0f,  ICOS_PHI,  0.0f}, { 1.0f,  ICOS_PHI,  0.0f},
    {-1.0f, -ICOS_PHI,  0.0f}, { 1.0f, -ICOS_PHI,  0.0f},
    { 0.0f, -1.0f,  ICOS_PHI}, { 0.0f,  1.0f,  ICOS_PHI},
    { 0.0f, -1.0f, -ICOS_PHI}, { 0.0f,  1.0f, -ICOS_PHI},
    { ICOS_PHI,  0.0f, -1.0f}, { ICOS_PHI,  0.0f,  1.0f},
    {-ICOS_PHI,  0.0f, -1.0f}, {-ICOS_PHI,  0.0f,  1.0f},
};

static const int g_icos_faces[20][3] = {
    {0, 11, 5}, {0, 5, 1}, {0, 1, 7}, {0, 7, 10}, {0, 10, 11},
    {1, 5, 9}, {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8},
    {3, 9, 4}, {3, 4, 2}, {3, 2, 6}, {3, 6, 8}, {3, 8, 9},
    {4, 9, 5}, {2, 4, 11}, {6, 2, 10}, {8, 6, 7}, {9, 8, 1},
};

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

static StarField g_star_field;

/* Particles fired from the origin toward a random point on a mesh triangle.
 * A particle is hidden whenever its target triangle currently faces the
 * camera, since that face's outer surface then sits between the camera and
 * the particle's inner-surface target. */
typedef struct {
    float target_x[MAX_EMITTER_PARTICLES];
    float target_y[MAX_EMITTER_PARTICLES];
    float target_z[MAX_EMITTER_PARTICLES];
    int target_triangle[MAX_EMITTER_PARTICLES];
    float progress[MAX_EMITTER_PARTICLES];
    float speed[MAX_EMITTER_PARTICLES];
    /* Randomized per spawn: progress at which this particle abandons its
     * current target and re-aims elsewhere, instead of always completing
     * the full journey to one fixed point. */
    float retarget_progress[MAX_EMITTER_PARTICLES];
} EmitterParticles;

static EmitterParticles g_emitter;
static int g_emitter_cache_subdivisions = -1;

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

static Vector3 rs_icos_normalize(Vector3 v)
{
    const float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len <= 0.0f) {
        return v;
    }
    return (Vector3){v.x / len, v.y / len, v.z / len};
}

/* Edge midpoints are not re-normalized onto a sphere, keeping every face a
 * flat facet. Every emitted sub-triangle shares its parent face's explosion
 * direction, so subdividing doesn't change how the face's island moves. */
static void rs_subdivide_triangle(TriangleCache *cache, int *count,
                                  Vector3 v0, Vector3 v1, Vector3 v2,
                                  Vector3 face_dir,
                                  Uint8 r, Uint8 g, Uint8 b, Uint8 a,
                                  int level)
{
    if (*count >= MAX_TRIANGLES) {
        return;
    }

    if (level <= 0) {
        const int idx = *count;
        const int base = idx * 3;

        cache->base_vx[base + 0] = v0.x; cache->base_vy[base + 0] = v0.y; cache->base_vz[base + 0] = v0.z;
        cache->base_vx[base + 1] = v1.x; cache->base_vy[base + 1] = v1.y; cache->base_vz[base + 1] = v1.z;
        cache->base_vx[base + 2] = v2.x; cache->base_vy[base + 2] = v2.y; cache->base_vz[base + 2] = v2.z;

        for (int k = 0; k < 3; k++) {
            cache->dir_x[base + k] = face_dir.x;
            cache->dir_y[base + k] = face_dir.y;
            cache->dir_z[base + k] = face_dir.z;
        }

        cache->r[idx] = r; cache->g[idx] = g; cache->b[idx] = b; cache->a[idx] = a;
        (*count)++;
        return;
    }

    const Vector3 m01 = {(v0.x + v1.x) * 0.5f, (v0.y + v1.y) * 0.5f, (v0.z + v1.z) * 0.5f};
    const Vector3 m12 = {(v1.x + v2.x) * 0.5f, (v1.y + v2.y) * 0.5f, (v1.z + v2.z) * 0.5f};
    const Vector3 m20 = {(v2.x + v0.x) * 0.5f, (v2.y + v0.y) * 0.5f, (v2.z + v0.z) * 0.5f};

    rs_subdivide_triangle(cache, count, v0, m01, m20, face_dir, r, g, b, a, level - 1);
    rs_subdivide_triangle(cache, count, m01, v1, m12, face_dir, r, g, b, a, level - 1);
    rs_subdivide_triangle(cache, count, m20, m12, v2, face_dir, r, g, b, a, level - 1);
    rs_subdivide_triangle(cache, count, m01, m12, m20, face_dir, r, g, b, a, level - 1);
}

static void rs_build_exploded_icosahedron(TriangleCache *cache, int subdivisions)
{
    int count = 0;

    for (int f = 0; f < 20 && count < MAX_TRIANGLES; f++) {
        const float *b0 = g_icos_base[g_icos_faces[f][0]];
        const float *b1 = g_icos_base[g_icos_faces[f][1]];
        const float *b2 = g_icos_base[g_icos_faces[f][2]];

        const Vector3 v0 = {b0[0], b0[1], b0[2]};
        const Vector3 v1 = {b1[0], b1[1], b1[2]};
        const Vector3 v2 = {b2[0], b2[1], b2[2]};

        /* The icosahedron is centered on the origin, so a face's centroid
         * direction from the origin already is its outward normal. */
        const Vector3 centroid = {
            (v0.x + v1.x + v2.x) / 3.0f,
            (v0.y + v1.y + v2.y) / 3.0f,
            (v0.z + v1.z + v2.z) / 3.0f,
        };
        const Vector3 face_dir = rs_icos_normalize(centroid);

        const Uint8 face_r = (Uint8)(100 + (f * 37) % 156);
        const Uint8 face_g = (Uint8)(120 + (f * 61) % 136);
        const Uint8 face_b = (Uint8)(180 - (f * 7) % 120);

        rs_subdivide_triangle(cache, &count, v0, v1, v2, face_dir,
                              face_r, face_g, face_b, 255, subdivisions);
    }

    cache->triangle_count = count;
}

static TriangleCache *rs_get_cached_icosahedron(int subdivisions, int *triangle_count)
{
    const int clamped = rs_clampi(subdivisions, 0, ICOS_SUBDIVISION_MAX);
    TriangleCache *cache = &g_triangle_cache[clamped];

    if (!cache->valid || cache->subdivisions != clamped) {
        rs_build_exploded_icosahedron(cache, clamped);
        cache->subdivisions = clamped;
        cache->valid = SDL_TRUE;
    }

    if (triangle_count) {
        *triangle_count = cache->triangle_count;
    }
    return cache;
}

/* Samples a uniform random point inside a random mesh triangle (via
 * rejection-free barycentric reflection) and aims a particle at it, in the
 * same unexploded local space as the triangle cache's base_v* arrays. */
static void rs_emitter_spawn_particle(int index, const TriangleCache *cache, int triangle_count)
{
    if (triangle_count <= 0) {
        return;
    }

    const int tri = rand() % triangle_count;
    float u = (float)rand() / RAND_MAX;
    float v = (float)rand() / RAND_MAX;
    if (u + v > 1.0f) {
        u = 1.0f - u;
        v = 1.0f - v;
    }
    const float w = 1.0f - u - v;

    const int base = tri * 3;
    g_emitter.target_x[index] = u * cache->base_vx[base + 0] + v * cache->base_vx[base + 1] + w * cache->base_vx[base + 2];
    g_emitter.target_y[index] = u * cache->base_vy[base + 0] + v * cache->base_vy[base + 1] + w * cache->base_vy[base + 2];
    g_emitter.target_z[index] = u * cache->base_vz[base + 0] + v * cache->base_vz[base + 1] + w * cache->base_vz[base + 2];
    g_emitter.target_triangle[index] = tri;
    g_emitter.progress[index] = 0.0f;
    g_emitter.speed[index] = 0.6f + (float)rand() / RAND_MAX * 0.8f;
    g_emitter.retarget_progress[index] = 0.35f + (float)rand() / RAND_MAX * 0.65f;
}

/* Batched position/life update for `count` particles. Respawn stays a
 * per-particle scalar pass: it's branchy (bounds/life check) and uses
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

/* Screen-space signed-area winding test: negative area means front-facing. */
static inline SDL_bool rs_triangle_is_front_facing(float x0, float y0, float x1, float y1, float x2, float y2)
{
    const float area = (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0);
    return area < 0.0f;
}

/* One front-facing test per triangle, shared by all 3 render modes and the
 * emitter's occlusion check instead of each recomputing it independently. */
static void rs_compute_front_facing(const float *proj_x, const float *proj_y,
                                    int triangle_count, SDL_bool *out_front_facing)
{
    for (int i = 0; i < triangle_count; i++) {
        const int base = i * 3;
        out_front_facing[i] = rs_triangle_is_front_facing(proj_x[base], proj_y[base],
                                                           proj_x[base + 1], proj_y[base + 1],
                                                           proj_x[base + 2], proj_y[base + 2]);
    }
}

static void rs_render_triangles_cpu(SDL_Renderer *renderer,
                                    const TriangleCache *cache,
                                    const float *proj_x, const float *proj_y,
                                    const SDL_bool *front_facing,
                                    int triangle_count,
                                    BenchMetrics *metrics)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    for (int i = 0; i < triangle_count; i++) {
        const int base = i * 3;
        if (!front_facing[i]) {
            continue;
        }

        SDL_Vertex vertices[3];
        for (int j = 0; j < 3; j++) {
            const int idx = base + j;
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
                                      const SDL_bool *front_facing,
                                      int triangle_count,
                                      BenchMetrics *metrics)
{
    if (!renderer || !cache || triangle_count <= 0) {
        return;
    }

    for (int i = 0; i < triangle_count; ++i) {
        const int base = i * 3;
        if (!front_facing[i]) {
            continue;
        }

        SDL_SetRenderDrawColor(renderer, cache->r[i], cache->g[i], cache->b[i], cache->a[i]);

        for (int j = 0; j < 3; ++j) {
            const int idx = base + j;
            SDL_RenderDrawPointF(renderer, proj_x[idx], proj_y[idx]);

            if (metrics) {
                metrics->draw_calls++;
                metrics->vertices_rendered++;
            }
        }
    }
}

void rs_scene_geometry_init(RenderSuiteState *state, SDL_Renderer *renderer)
{
    (void)state;
    (void)renderer;

    memset(&g_star_field, 0, sizeof(g_star_field));
    for (int i = 0; i < MAX_STAR_PARTICLES; i++) {
        g_star_field.life[i] = 0.0f; /* force respawn on first update */
    }

    memset(&g_emitter, 0, sizeof(g_emitter));
    for (int i = 0; i < MAX_EMITTER_PARTICLES; i++) {
        g_emitter.progress[i] = 1.0f; /* force a real target on first update */
    }
    g_emitter_cache_subdivisions = -1;
}

void rs_scene_geometry_cleanup(RenderSuiteState *state)
{
    (void)state;

    memset(&g_star_field, 0, sizeof(g_star_field));
    memset(&g_emitter, 0, sizeof(g_emitter));
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

    // Calculate triangle count based on stress level (subdivides each of the
    // icosahedron's 20 faces; level N gives 4^N sub-triangles/face)
    const int base_triangles = 20;
    const int max_triangles = rs_clampi((int)(base_triangles + factor * 60), base_triangles, MAX_TRIANGLES);
    state->geometry_triangle_count = max_triangles;

    const int subdivisions = rs_clampi(
        (int)(logf((float)max_triangles / 20.0f) / logf(4.0f) + 0.5f), 0, ICOS_SUBDIVISION_MAX);
    int cached_triangle_count = 0;
    TriangleCache *cache = rs_get_cached_icosahedron(subdivisions, &cached_triangle_count);
    const int triangle_count = rs_clampi(cached_triangle_count, 0, max_triangles);

    // Update star field
    const int particle_count = rs_clampi((int)(50 + factor * 150), 50, MAX_STAR_PARTICLES);
    rs_update_star_field(&g_star_field,
                         particle_count,
                         (float)delta_seconds,
                         center_x,
                         center_y,
                         state);

    // Render star field background
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
    rs_render_star_field(renderer, &g_star_field, particle_count, metrics);

    const float scale = 80.0f + 40.0f * rs_state_sin(state, state->geometry_phase * 10.0f);

    // Explosion distance pulses but never reaches 0, so faces never re-touch.
    const float explosion_base = 0.7f;
    const float explosion_amplitude = 0.35f;
    const float explosion_distance = explosion_base +
        explosion_amplitude * rs_state_sin(state, state->geometry_phase * 6.0f);

    RotationTrig rotation_trig = rs_build_rotation_trig(state,
                                                       state->geometry_rotation,
                                                       state->geometry_rotation * 0.7f,
                                                       state->geometry_rotation * 0.3f);

    static float exploded_x[MAX_PROJECTED];
    static float exploded_y[MAX_PROJECTED];
    static float exploded_z[MAX_PROJECTED];
    static float proj_x[MAX_PROJECTED];
    static float proj_y[MAX_PROJECTED];
    const int vertex_count = triangle_count * 3;

    for (int i = 0; i < vertex_count; i++) {
        exploded_x[i] = cache->base_vx[i] + cache->dir_x[i] * explosion_distance;
        exploded_y[i] = cache->base_vy[i] + cache->dir_y[i] * explosion_distance;
        exploded_z[i] = cache->base_vz[i] + cache->dir_z[i] * explosion_distance;
    }

    // A subdivision-level change rebuilds the mesh cache, so any stored
    // target-triangle index would now point at a different triangle --
    // respawn every emitter particle against the fresh cache.
    if (subdivisions != g_emitter_cache_subdivisions) {
        for (int i = 0; i < MAX_EMITTER_PARTICLES; i++) {
            rs_emitter_spawn_particle(i, cache, triangle_count);
        }
        g_emitter_cache_subdivisions = subdivisions;
    }

    const int emitter_particle_count = rs_clampi((int)(20 + factor * 40), 20, MAX_EMITTER_PARTICLES);

    // Motion is a straight lerp from the origin to each particle's exploded
    // target, so a trail is just several more samples behind the current
    // progress along that same line -- no per-frame position history needed.
    for (int i = 0; i < emitter_particle_count; i++) {
        g_emitter.progress[i] += g_emitter.speed[i] * (float)delta_seconds;
        if (g_emitter.progress[i] >= g_emitter.retarget_progress[i] ||
            g_emitter.target_triangle[i] >= triangle_count) {
            rs_emitter_spawn_particle(i, cache, triangle_count);
        }

        const int base = g_emitter.target_triangle[i] * 3;
        const float ex = g_emitter.target_x[i] + cache->dir_x[base] * explosion_distance;
        const float ey = g_emitter.target_y[i] + cache->dir_y[base] * explosion_distance;
        const float ez = g_emitter.target_z[i] + cache->dir_z[base] * explosion_distance;

        for (int k = 0; k < EMITTER_TRAIL_SAMPLES; k++) {
            const float sample_progress = rs_clampf(g_emitter.progress[i] - (float)k * EMITTER_TRAIL_STEP, 0.0f, 1.0f);
            const int slot = vertex_count + i * EMITTER_TRAIL_SAMPLES + k;
            exploded_x[slot] = ex * sample_progress;
            exploded_y[slot] = ey * sample_progress;
            exploded_z[slot] = ez * sample_progress;
        }
    }

    const int trail_point_count = emitter_particle_count * EMITTER_TRAIL_SAMPLES;
    rs_rotate_project_batch(exploded_x, exploded_y, exploded_z, vertex_count + trail_point_count,
                            &rotation_trig, center_x, center_y, scale, proj_x, proj_y);

    static SDL_bool front_facing[MAX_TRIANGLES];
    rs_compute_front_facing(proj_x, proj_y, triangle_count, front_facing);

    const RSGeometryRenderMode render_mode =
        (RSGeometryRenderMode)(state->geometry_render_mode % RS_GEOMETRY_RENDER_MODE_MAX);

    if (render_mode == RS_GEOMETRY_RENDER_FILLED) {
        rs_render_triangles_cpu(renderer, cache, proj_x, proj_y, front_facing, triangle_count, metrics);
    } else if (render_mode == RS_GEOMETRY_RENDER_POINTS) {
        rs_render_triangle_points(renderer, cache, proj_x, proj_y, front_facing, triangle_count, metrics);
    } else { // wireframe as default fallback
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 128);

        for (int i = 0; i < triangle_count; i++) {
            if (!front_facing[i]) {
                continue;
            }
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

    // Independent layer over any mesh render mode; trail samples fade from
    // full alpha at the head (k=0) to near-transparent at the tail.
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
    for (int i = 0; i < emitter_particle_count; i++) {
        if (front_facing[g_emitter.target_triangle[i]]) {
            continue;
        }

        for (int k = 0; k < EMITTER_TRAIL_SAMPLES; k++) {
            const Uint8 alpha = (Uint8)(255.0f * (1.0f - (float)k / (float)EMITTER_TRAIL_SAMPLES));
            SDL_SetRenderDrawColor(renderer, 180, 220, 255, alpha);

            const int slot = vertex_count + i * EMITTER_TRAIL_SAMPLES + k;
            SDL_RenderDrawPointF(renderer, proj_x[slot], proj_y[slot]);

            if (metrics) {
                metrics->draw_calls++;
                metrics->vertices_rendered++;
            }
        }
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}
