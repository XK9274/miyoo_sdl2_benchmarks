#include "render_suite_gl/input.h"

#include <SDL2/SDL.h>

#include "controller_input.h"
#include "render_suite_gl/scenes/effects.h"

SDL_bool rsgl_handle_input(RsglState *state, BenchMetrics *metrics)
{
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            return SDL_FALSE;
        }
        if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case BTN_START:
                case BTN_EXIT:
                case SDLK_ESCAPE:
                    return SDL_FALSE;
                case BTN_A:
                    state->auto_cycle = !state->auto_cycle;
                    break;
                case BTN_Y:
                    if (state->effect_count > 0) {
                        state->effect_index = (state->effect_index + 1) % state->effect_count;
                    }
                    break;
                case BTN_X:
                    bench_reset_metrics(metrics);
                    break;
                case BTN_SELECT:
                    bench_reset_metrics(metrics);
                    break;
                default:
                    break;
            }
        }
    }
    return SDL_TRUE;
}
