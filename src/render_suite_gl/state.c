#include "render_suite_gl/state.h"

const RsglFboPreset rsgl_fbo_presets[RSGL_FBO_PRESET_COUNT] = {
    {80, 60, "80x60"},
    {160, 120, "160x120"},
    {240, 180, "240x180"},
    {320, 240, "320x240"}
};

static int rsgl_state_normalize_index(int index)
{
    if (index < 0 || index >= RSGL_FBO_PRESET_COUNT) {
        return RSGL_FBO_DEFAULT_INDEX;
    }
    return index;
}

static void rsgl_state_apply_fbo_preset(RsglState *state, int index)
{
    if (!state) {
        return;
    }
    const int normalized = rsgl_state_normalize_index(index);
    state->fbo_size_index = normalized;
    state->fbo_width = rsgl_fbo_presets[normalized].width;
    state->fbo_height = rsgl_fbo_presets[normalized].height;
}

void rsgl_state_init(RsglState *state)
{
    if (!state) {
        return;
    }

    SDL_memset(state, 0, sizeof(*state));
    state->running = SDL_TRUE;
    state->auto_cycle = SDL_TRUE;
    state->effect_index = 0;
    state->effect_count = 0;
    state->elapsed_time = 0.0f;
    state->top_margin = 0.0f;
    state->screen_width = BENCH_SCREEN_W;
    state->screen_height = BENCH_SCREEN_H;
    rsgl_state_apply_fbo_preset(state, RSGL_FBO_DEFAULT_INDEX);
    state->fbo_prev_size_index = state->fbo_size_index;
    state->fbo_dirty = SDL_FALSE;
}

void rsgl_state_update_layout(RsglState *state, BenchOverlay *overlay)
{
    if (!state) {
        return;
    }

    state->top_margin = overlay ? (float)bench_overlay_height(overlay) : 0.0f;
}

void rsgl_state_destroy(RsglState *state)
{
    if (!state) {
        return;
    }

    if (state->screen_texture) {
        SDL_DestroyTexture(state->screen_texture);
        state->screen_texture = NULL;
    }
    if (state->pixel_buffer) {
        SDL_free(state->pixel_buffer);
        state->pixel_buffer = NULL;
        state->pixel_capacity = 0;
    }
    if (state->font) {
        TTF_CloseFont(state->font);
        state->font = NULL;
    }
}

void rsgl_state_cycle_fbo_size(RsglState *state)
{
    if (!state) {
        return;
    }
    state->fbo_prev_size_index = state->fbo_size_index;
    const int next_index = (state->fbo_size_index + 1) % RSGL_FBO_PRESET_COUNT;
    rsgl_state_apply_fbo_preset(state, next_index);
    state->fbo_dirty = SDL_TRUE;
}

void rsgl_state_commit_fbo_size(RsglState *state)
{
    if (!state) {
        return;
    }
    state->fbo_prev_size_index = state->fbo_size_index;
    state->fbo_dirty = SDL_FALSE;
}

void rsgl_state_revert_fbo_size(RsglState *state)
{
    if (!state) {
        return;
    }
    rsgl_state_apply_fbo_preset(state, state->fbo_prev_size_index);
    state->fbo_dirty = SDL_FALSE;
}
