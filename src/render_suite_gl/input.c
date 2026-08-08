#include "render_suite_gl/input.h"

#include <SDL2/SDL.h>

#include "controller_input.h"
#include "common/driver_support.h"
#include "render_suite_gl/scenes/effects.h"

SDL_bool rsgl_handle_input(RsglState *state, BenchMetrics *metrics)
{
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            return SDL_FALSE;
        }
        const SDL_Keycode sym = bench_driver_translate_event(&e);
        if (sym != 0) {
            switch (sym) {
                case BTN_EXIT:
                case SDLK_ESCAPE:
                    return SDL_FALSE;
                case BTN_START:
                    bench_driver_toggle_input_mode();
                    break;
                case BTN_VSYNC_TOGGLE:
                    bench_driver_toggle_vsync();
                    break;
                case BTN_VSYNC_MODE_TOGGLE:
                    bench_driver_toggle_vsync_mode();
                    break;
                case BTN_A:
                    state->auto_cycle = !state->auto_cycle;
                    break;
                case BTN_Y:
                    if (state->effect_count > 0) {
                        state->effect_index = (state->effect_index + 1) % state->effect_count;
                    }
                    break;
                case BTN_X:
                    rsgl_state_cycle_fbo_size(state);
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
