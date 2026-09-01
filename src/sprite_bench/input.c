#include "sprite_bench/input.h"

#include <SDL2/SDL.h>

#include "controller_input.h"
#include "common/driver_support.h"
#include "common/hotkeys.h"

SDL_bool sprite_handle_input(SpriteBenchState *state, BenchMetrics *metrics, BenchOverlay *overlay)
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
                case BTN_UP:
                    state->step_size += 10;
                    if (state->step_size > SPRITE_STEP_MAX) {
                        state->step_size = SPRITE_STEP_MAX;
                    }
                    break;
                case BTN_DOWN:
                    state->step_size -= 10;
                    if (state->step_size < SPRITE_STEP_MIN) {
                        state->step_size = SPRITE_STEP_MIN;
                    }
                    break;
                case BTN_LEFT:
                    state->interval_seconds -= 0.25;
                    if (state->interval_seconds < SPRITE_INTERVAL_MIN) {
                        state->interval_seconds = SPRITE_INTERVAL_MIN;
                    }
                    break;
                case BTN_RIGHT:
                    state->interval_seconds += 0.25;
                    if (state->interval_seconds > SPRITE_INTERVAL_MAX) {
                        state->interval_seconds = SPRITE_INTERVAL_MAX;
                    }
                    break;
                case BTN_A:
                    state->direction = 1;
                    break;
                case BTN_B:
                    state->direction = -1;
                    break;
                case BTN_X:
                    state->static_mode = !state->static_mode;
                    break;
                case BTN_METRICS_RESET:
                    if (metrics) {
                        bench_reset_metrics(metrics);
                    }
                    break;
                case BTN_OVERLAY_TOGGLE:
                    bench_overlay_toggle_collapsed(overlay);
                    break;
                default:
                    break;
            }
        }
    }
    return SDL_TRUE;
}
