#ifndef COMMON_GEOMETRY_CORE_H
#define COMMON_GEOMETRY_CORE_H

#include <SDL2/SDL.h>

#include "common/types.h"

typedef struct {
    float rotate_x;
    float rotate_y;
    float rotate_z;
    float screen_x;
    float screen_y;
} BenchVertex;

typedef struct {
    float rotation;
    float cos_val;
    float sin_val;
} RotationCache;

void bench_update_rotation_cache(RotationCache *cache, float rotation_radians);

void bench_project_vertex(const float *base_coords,
                          const RotationCache *cache,
                          float center_x,
                          float center_y,
                          float size,
                          BenchVertex *out_vertex);

/* Like bench_project_vertex, but rotates the Y-Z plane (roll around the local
 * X/forward axis) instead of X-Z (yaw around the vertical axis) -- for models
 * whose forward axis is X, e.g. ships banking as they fly. base_coords are
 * final local units (not multiplied by a separate size), extra_z is added to
 * the rotated Z before the perspective divide (for manual depth offsets, e.g.
 * parallax sorting that should now also affect scale). */
void bench_project_vertex_roll(const float *base_coords,
                               const RotationCache *cache,
                               float center_x,
                               float center_y,
                               float depth,
                               float extra_z,
                               BenchVertex *out_vertex);

void bench_setup_sdl_vertex(SDL_Vertex *vert,
                            const BenchVertex *bench_vert,
                            const SDL_Color *color);

void bench_render_triangle_batch(SDL_Renderer *renderer,
                                 SDL_Vertex *vertices,
                                 int triangle_count,
                                 BenchMetrics *metrics);

void bench_render_edge_batch(SDL_Renderer *renderer,
                             const BenchVertex *vertices,
                             const int (*edges)[2],
                             int edge_count,
                             const SDL_Color *palette,
                             int palette_size,
                             int mode,
                             BenchMetrics *metrics);

void bench_render_points(SDL_Renderer *renderer,
                         const BenchVertex *vertices,
                         int vertex_count,
                         const SDL_Color *color,
                         BenchMetrics *metrics);

#endif /* COMMON_GEOMETRY_CORE_H */
