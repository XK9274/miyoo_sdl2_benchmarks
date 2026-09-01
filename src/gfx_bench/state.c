#include "gfx_bench/state.h"

#include <math.h>

#define GB_TWO_PI 6.28318530717958647692f

static void gb_state_initialise_tables(GfxBenchState *state)
{
    state->sin_table_size = GB_SIN_TABLE_SIZE;
    state->sin_table_mask = GB_SIN_TABLE_SIZE - 1;
    for (int i = 0; i < state->sin_table_size; ++i) {
        const float angle = (GB_TWO_PI * (float)i) / (float)state->sin_table_size;
        state->sin_table[i] = sinf(angle);
    }
}

void gb_state_init(GfxBenchState *state)
{
    if (!state) {
        return;
    }
    SDL_memset(state, 0, sizeof(*state));
    state->active_scene = GB_SCENE_AA_SHAPES;
    state->auto_cycle = SDL_TRUE;
    state->stress_level = 1;
    state->phase = 0.0f;
    gb_state_initialise_tables(state);
}

float gb_state_stress_factor(const GfxBenchState *state)
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

float gb_state_sin(const GfxBenchState *state, float units)
{
    if (!state || state->sin_table_size <= 0) {
        return 0.0f;
    }
    const int idx = ((int)units) & state->sin_table_mask;
    return state->sin_table[idx];
}

float gb_state_cos(const GfxBenchState *state, float units)
{
    if (!state || state->sin_table_size <= 0) {
        return 0.0f;
    }
    const float quarter_turn = (float)(state->sin_table_size >> 2);
    return gb_state_sin(state, units + quarter_turn);
}

float gb_state_sin_rad(const GfxBenchState *state, float radians)
{
    if (!state || state->sin_table_size <= 0) {
        return 0.0f;
    }
    const float units = radians * (float)state->sin_table_size / GB_TWO_PI;
    return gb_state_sin(state, units);
}

float gb_state_cos_rad(const GfxBenchState *state, float radians)
{
    if (!state || state->sin_table_size <= 0) {
        return 0.0f;
    }
    const float units = radians * (float)state->sin_table_size / GB_TWO_PI;
    return gb_state_cos(state, units);
}
