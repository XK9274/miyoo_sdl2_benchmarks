#include "render_suite/state.h"

#include <math.h>

#define RS_TWO_PI 6.28318530717958647692f

static void rs_state_initialise_tables(RenderSuiteState *state)
{
    state->sin_table_size = RS_SIN_TABLE_SIZE;
    state->sin_table_mask = RS_SIN_TABLE_SIZE - 1;
    for (int i = 0; i < state->sin_table_size; ++i) {
        const float angle = (RS_TWO_PI * (float)i) / (float)state->sin_table_size;
        state->sin_table[i] = sinf(angle);
    }
}

void rs_state_init(RenderSuiteState *state)
{
    if (!state) {
        return;
    }
    SDL_memset(state, 0, sizeof(*state));
    state->active_scene = SCENE_FILL;
    state->auto_cycle = SDL_TRUE;
    state->stress_level = 1;
    state->texture_angle = 0.0f;
    state->top_margin = 0.0f;
    rs_state_initialise_tables(state);
    state->fill_phase_units = 0.0f;
    state->texture_phase_units = 0.0f;
    state->lines_cursor_progress = 0.0f;
    state->lines_cursor_index = 0;
}

void rs_state_update_layout(RenderSuiteState *state, BenchOverlay *overlay)
{
    if (!state) {
        return;
    }
    state->top_margin = overlay ? (float)bench_overlay_height(overlay) : 0.0f;
}

void rs_state_destroy(RenderSuiteState *state, SDL_Renderer *renderer)
{
    if (!state) {
        return;
    }

    if (state->checker_texture) {
        SDL_DestroyTexture(state->checker_texture);
        state->checker_texture = NULL;
    }
    if (state->font) {
        TTF_CloseFont(state->font);
        state->font = NULL;
    }
    (void)renderer;
}

float rs_state_stress_factor(const RenderSuiteState *state)
{
    if (!state) {
        return 1.0f;
    }
    int level = state->stress_level;
    if (level < 1) {
        level = 1;
    } else if (level > 10) {
        level = 10;
    }
    const float min_factor = 0.5f;
    const float max_factor = 7.0f;
    if (level <= 1) {
        return min_factor;
    }
    const float t = (float)(level - 1) / 9.0f;
    return min_factor + (max_factor - min_factor) * t;
}

float rs_state_sin(const RenderSuiteState *state, float units)
{
    if (!state || state->sin_table_size <= 0) {
        return 0.0f;
    }
    const int idx = ((int)units) & state->sin_table_mask;
    return state->sin_table[idx];
}

float rs_state_cos(const RenderSuiteState *state, float units)
{
    if (!state || state->sin_table_size <= 0) {
        return 0.0f;
    }
    const float quarter_turn = (float)(state->sin_table_size >> 2);
    return rs_state_sin(state, units + quarter_turn);
}
