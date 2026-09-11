#ifndef GEOMETRY_BENCH_STATE_H
#define GEOMETRY_BENCH_STATE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "bench_common.h"
#include "common/bench_stress.h"

typedef enum {
    GEOMETRY_RENDER_FILLED = 0,
    GEOMETRY_RENDER_WIREFRAME,
    GEOMETRY_RENDER_POINTS,
    GEOMETRY_RENDER_MODE_MAX
} GeometryRenderMode;

typedef struct {
    int stress_level;
    float top_margin;
    TTF_Font *font;

    BenchSinTable sin_table;

    float geometry_rotation;
    int geometry_triangle_count;
    float geometry_phase;
    int geometry_render_mode;
} GeometryBenchState;

void geometry_state_init(GeometryBenchState *state);
void geometry_state_update_layout(GeometryBenchState *state, BenchOverlay *overlay);
void geometry_state_destroy(GeometryBenchState *state, SDL_Renderer *renderer);

float geometry_state_stress_factor(const GeometryBenchState *state);
float geometry_state_sin(const GeometryBenchState *state, float units);
float geometry_state_sin_rad(const GeometryBenchState *state, float radians);
float geometry_state_cos_rad(const GeometryBenchState *state, float radians);

#endif /* GEOMETRY_BENCH_STATE_H */
