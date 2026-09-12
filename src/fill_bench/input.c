#include "fill_bench/input.h"

#include <SDL2/SDL.h>

#include "controller_input.h"
#include "common/driver_support.h"
#include "common/hotkeys.h"

SDL_bool fill_handle_input(FillBenchState *state, BenchMetrics *metrics, BenchOverlay *overlay)
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
                case BTN_B:
                    state->stress_level++;
                    if (state->stress_level > 10) {
                        state->stress_level = 1;
                    }
                    break;
                case BTN_METRICS_RESET:
                    bench_reset_metrics(metrics);
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
