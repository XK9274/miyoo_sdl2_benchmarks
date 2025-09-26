#include "render_suite/scenes/geometry.h"

#include <math.h>
#include <stdlib.h>

#define RS_PI 3.14159265358979323846f
#define MAX_TRIANGLES 500
#define MAX_STAR_PARTICLES 200

typedef struct {
    float x, y, z;
} Vector3;

typedef struct {
    Vector3 vertices[3];
    Uint8 r, g, b, a;
} Triangle3D;

typedef struct {
    float x, y;
    float dx, dy;
    Uint8 r, g, b, a;
    float life;
} StarParticle;

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

static void rs_project_vertex(const Vector3 *v3d, float *x2d, float *y2d,
                              float center_x, float center_y, float scale)
{
    const float perspective = 1.0f / (1.0f + v3d->z * 0.001f);
    *x2d = center_x + v3d->x * scale * perspective;
    *y2d = center_y + v3d->y * scale * perspective;
}

static void rs_rotate_vector(Vector3 *v, float rx, float ry, float rz)
{
    float x = v->x, y = v->y, z = v->z;

    // Rotate around X axis
    float cos_rx = cosf(rx), sin_rx = sinf(rx);
    float new_y = y * cos_rx - z * sin_rx;
    float new_z = y * sin_rx + z * cos_rx;
    y = new_y;
    z = new_z;

    // Rotate around Y axis
    float cos_ry = cosf(ry), sin_ry = sinf(ry);
    float new_x = x * cos_ry + z * sin_ry;
    new_z = -x * sin_ry + z * cos_ry;
    x = new_x;
    z = new_z;

    // Rotate around Z axis
    float cos_rz = cosf(rz), sin_rz = sinf(rz);
    new_x = x * cos_rz - y * sin_rz;
    new_y = x * sin_rz + y * cos_rz;

    v->x = new_x;
    v->y = new_y;
    v->z = z;
}

static void rs_create_cube_triangles(Triangle3D *triangles, int *triangle_count,
                                     float size, int subdivisions)
{
    int count = 0;
    const float step = size / (float)subdivisions;

    // Create tessellated cube faces
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

                // Create two triangles per quad
                triangles[count].vertices[0] = v1;
                triangles[count].vertices[1] = v2;
                triangles[count].vertices[2] = v3;
                triangles[count].r = (Uint8)(128 + face * 20);
                triangles[count].g = (Uint8)(100 + (i + j) * 8);
                triangles[count].b = (Uint8)(200 - face * 15);
                triangles[count].a = 255;
                count++;

                triangles[count].vertices[0] = v1;
                triangles[count].vertices[1] = v3;
                triangles[count].vertices[2] = v4;
                triangles[count].r = (Uint8)(128 + face * 20);
                triangles[count].g = (Uint8)(100 + (i + j) * 8);
                triangles[count].b = (Uint8)(200 - face * 15);
                triangles[count].a = 255;
                count++;
            }
        }
    }

    *triangle_count = count;
}

static void rs_update_star_field(StarParticle *particles, int particle_count,
                                 float delta_seconds, float center_x, float center_y)
{
    for (int i = 0; i < particle_count; i++) {
        StarParticle *p = &particles[i];

        p->x += p->dx * delta_seconds * 60.0f;
        p->y += p->dy * delta_seconds * 60.0f;
        p->life -= delta_seconds;

        // Reset particle if it goes off-screen or dies
        if (p->x < 0 || p->x > BENCH_SCREEN_W ||
            p->y < 0 || p->y > BENCH_SCREEN_H || p->life <= 0.0f) {

            p->x = center_x + ((float)rand() / RAND_MAX - 0.5f) * 20.0f;
            p->y = center_y + ((float)rand() / RAND_MAX - 0.5f) * 20.0f;

            float angle = (float)rand() / RAND_MAX * 2.0f * RS_PI;
            float speed = 20.0f + (float)rand() / RAND_MAX * 80.0f;
            p->dx = cosf(angle) * speed;
            p->dy = sinf(angle) * speed;

            p->r = (Uint8)(200 + rand() % 56);
            p->g = (Uint8)(200 + rand() % 56);
            p->b = (Uint8)(100 + rand() % 156);
            p->a = 255;
            p->life = 1.0f + (float)rand() / RAND_MAX * 3.0f;
        }
    }
}

static void rs_render_star_field(SDL_Renderer *renderer, const StarParticle *particles,
                                 int particle_count, BenchMetrics *metrics)
{
    for (int i = 0; i < particle_count; i++) {
        const StarParticle *p = &particles[i];
        Uint8 alpha = (Uint8)(p->a * rs_clampf(p->life, 0.0f, 1.0f));

        SDL_SetRenderDrawColor(renderer, p->r, p->g, p->b, alpha);
        SDL_RenderDrawPointF(renderer, p->x, p->y);

        if (metrics) {
            metrics->draw_calls++;
            metrics->vertices_rendered++;
        }
    }
}

static void rs_render_triangles(SDL_Renderer *renderer, const Triangle3D *triangles,
                                int triangle_count, float rotation,
                                float center_x, float center_y, float scale,
                                BenchMetrics *metrics)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    for (int i = 0; i < triangle_count; i++) {
        const Triangle3D *tri = &triangles[i];

        // Copy and rotate vertices
        Vector3 rotated[3];
        for (int j = 0; j < 3; j++) {
            rotated[j] = tri->vertices[j];
            rs_rotate_vector(&rotated[j], rotation, rotation * 0.7f, rotation * 0.3f);
        }

        // Project to 2D
        SDL_Vertex vertices[3];
        for (int j = 0; j < 3; j++) {
            rs_project_vertex(&rotated[j], &vertices[j].position.x, &vertices[j].position.y,
                             center_x, center_y, scale);
            vertices[j].color.r = tri->r;
            vertices[j].color.g = tri->g;
            vertices[j].color.b = tri->b;
            vertices[j].color.a = tri->a;
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

void rs_scene_geometry(RenderSuiteState *state,
                       SDL_Renderer *renderer,
                       BenchMetrics *metrics,
                       double delta_seconds)
{
    if (!state || !renderer) {
        return;
    }

    const float factor = rs_state_stress_factor(state);
    const int region_height = SDL_max(1, BENCH_SCREEN_H - (int)state->top_margin);
    const float center_x = BENCH_SCREEN_W * 0.5f;
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
    static Triangle3D triangles[MAX_TRIANGLES];
    int triangle_count = 0;

    rs_create_cube_triangles(triangles, &triangle_count, 50.0f, subdivisions);
    triangle_count = rs_clampi(triangle_count, 0, max_triangles);

    // Create and update star field
    static StarParticle star_particles[MAX_STAR_PARTICLES];
    static SDL_bool particles_initialized = SDL_FALSE;

    if (!particles_initialized) {
        for (int i = 0; i < MAX_STAR_PARTICLES; i++) {
            star_particles[i].life = 0.0f; // Force regeneration
        }
        particles_initialized = SDL_TRUE;
    }

    const int particle_count = rs_clampi((int)(50 + factor * 150), 50, MAX_STAR_PARTICLES);
    rs_update_star_field(star_particles, particle_count, (float)delta_seconds, center_x, center_y);

    // Render star field background
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
    rs_render_star_field(renderer, star_particles, particle_count, metrics);

    // Render rotating cube
    const float scale = 80.0f + 40.0f * rs_state_sin(state, state->geometry_phase * 10.0f);
    rs_render_triangles(renderer, triangles, triangle_count, state->geometry_rotation,
                       center_x, center_y, scale, metrics);

    // Additional geometry complexity - wireframe overlay
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 128);

    for (int i = 0; i < triangle_count; i++) {
        const Triangle3D *tri = &triangles[i];
        Vector3 rotated[3];

        for (int j = 0; j < 3; j++) {
            rotated[j] = tri->vertices[j];
            rs_rotate_vector(&rotated[j], state->geometry_rotation,
                           state->geometry_rotation * 0.7f, state->geometry_rotation * 0.3f);
        }

        float x[3], y[3];
        for (int j = 0; j < 3; j++) {
            rs_project_vertex(&rotated[j], &x[j], &y[j], center_x, center_y, scale);
        }

        // Draw wireframe
        SDL_RenderDrawLineF(renderer, x[0], y[0], x[1], y[1]);
        SDL_RenderDrawLineF(renderer, x[1], y[1], x[2], y[2]);
        SDL_RenderDrawLineF(renderer, x[2], y[2], x[0], y[0]);

        if (metrics) {
            metrics->draw_calls += 3;
            metrics->vertices_rendered += 6;
        }
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}