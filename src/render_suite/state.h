#ifndef RENDER_SUITE_STATE_H
#define RENDER_SUITE_STATE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "bench_common.h"

#define RS_SIN_TABLE_SIZE 512

typedef enum {
    SCENE_FILL = 0,
    SCENE_TEXTURE,
    SCENE_LINES,
    SCENE_MAX
} SceneKind;

typedef struct {
    SceneKind active_scene;
    SDL_bool auto_cycle;
    int stress_level;
    float texture_angle;
    float top_margin;
    TTF_Font *font;
    SDL_Texture *checker_texture;

    float sin_table[RS_SIN_TABLE_SIZE];
    int sin_table_size;
    int sin_table_mask;

    float fill_phase_units;
    float texture_phase_units;
    float lines_cursor_progress;
    int lines_cursor_index;
} RenderSuiteState;

void rs_state_init(RenderSuiteState *state);
void rs_state_update_layout(RenderSuiteState *state, BenchOverlay *overlay);
void rs_state_destroy(RenderSuiteState *state, SDL_Renderer *renderer);

float rs_state_stress_factor(const RenderSuiteState *state);
float rs_state_sin(const RenderSuiteState *state, float units);
float rs_state_cos(const RenderSuiteState *state, float units);

#endif /* RENDER_SUITE_STATE_H */
