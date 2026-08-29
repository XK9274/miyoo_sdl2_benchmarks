#ifndef GFX_BENCH_STATE_H
#define GFX_BENCH_STATE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "bench_common.h"

#define GB_SIN_TABLE_SIZE 512

typedef enum {
    GB_SCENE_AA_SHAPES = 0,
    GB_SCENE_ROUNDED_RECTS,
    GB_SCENE_POLYGONS,
    GB_SCENE_BEZIER,
    GB_SCENE_THICK_LINES,
    GB_SCENE_MAX
} GfxBenchSceneKind;

typedef struct {
    GfxBenchSceneKind active_scene;
    SDL_bool auto_cycle;
    int stress_level; /* 1-10 */
    float top_margin;

    float sin_table[GB_SIN_TABLE_SIZE];
    int sin_table_size;
    int sin_table_mask;

    float phase; /* shared animation clock, advances by delta_seconds each frame */
} GfxBenchState;

void gb_state_init(GfxBenchState *state);
void gb_state_update_layout(GfxBenchState *state, BenchOverlay *overlay);

/* Maps stress_level 1-10 to a workload multiplier, same curve as render_suite's. */
float gb_state_stress_factor(const GfxBenchState *state);

float gb_state_sin(const GfxBenchState *state, float units);
float gb_state_cos(const GfxBenchState *state, float units);
float gb_state_sin_rad(const GfxBenchState *state, float radians);
float gb_state_cos_rad(const GfxBenchState *state, float radians);

/* SDL2_gfx primitive color format is packed 0xRRGGBBAA, unlike SDL_Color. */
static inline Uint32 gb_pack_color(Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    return ((Uint32)r << 24) | ((Uint32)g << 16) | ((Uint32)b << 8) | (Uint32)a;
}

#endif /* GFX_BENCH_STATE_H */
