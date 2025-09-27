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
