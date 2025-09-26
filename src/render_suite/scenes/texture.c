#include "render_suite/scenes/texture.h"

void rs_scene_texture(RenderSuiteState *state,
                      SDL_Renderer *renderer,
                      BenchMetrics *metrics,
                      double delta_seconds)
{
    if (!state->checker_texture) {
        return;
    }

    const float factor = rs_state_stress_factor(state);
    const float table_size = (float)state->sin_table_size;

    state->texture_phase_units += (float)(delta_seconds * 32.0f);
    while (state->texture_phase_units >= table_size) {
        state->texture_phase_units -= table_size;
    }

    const float wave = rs_state_sin(state, state->texture_phase_units);
    const float scale = 1.0f + 0.40f * wave;

    SDL_FRect dest = {
        BENCH_SCREEN_W * 0.5f - 96.0f * scale,
        (state->top_margin + (BENCH_SCREEN_H - state->top_margin) * 0.5f) - 96.0f * scale,
        192.0f * scale,
        192.0f * scale
    };

    SDL_RenderCopyExF(renderer, state->checker_texture, NULL, &dest, state->texture_angle, NULL, SDL_FLIP_NONE);
    if (metrics) {
        metrics->draw_calls++;
        metrics->vertices_rendered += 4;
        metrics->triangles_rendered += 2;
    }

    int sprite_count = (int)(12.0f * factor);
    if (sprite_count < 10) {
        sprite_count = 10;
    } else if (sprite_count > 192) {
        sprite_count = 192;
    }

    const int vertical_space = SDL_max(1, BENCH_SCREEN_H - (int)state->top_margin);
    const int sprite_stride = SDL_max(18, 64 - (int)(factor * 3.0f));

    for (int i = 0; i < sprite_count; ++i) {
        SDL_Rect rect = {
            (i * sprite_stride + (int)(state->texture_angle * 2.0f)) % BENCH_SCREEN_W - 16,
            (int)state->top_margin + (i * sprite_stride + (int)(state->texture_angle * 1.3f)) % vertical_space - 16,
            32,
            32
        };
        if (rect.y < (int)state->top_margin) {
            rect.y = (int)state->top_margin;
        }
        SDL_RenderCopy(renderer, state->checker_texture, NULL, &rect);
        if (metrics) {
            metrics->draw_calls++;
            metrics->vertices_rendered += 4;
            metrics->triangles_rendered += 2;
        }
    }

    state->texture_angle += (float)(delta_seconds * 180.0);
}
