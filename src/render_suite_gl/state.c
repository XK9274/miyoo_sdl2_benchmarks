#include "render_suite_gl/state.h"

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
