#include "pixels_bench/input.h"
#include "pixels_bench/render.h"

#include <SDL2/SDL.h>

#include "controller_input.h"
#include "common/driver_support.h"
#include "common/hotkeys.h"

SDL_bool pixels_handle_input(PixelsBenchState *state, BenchMetrics *metrics, BenchOverlay *overlay)
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
                case BTN_X:
                    if (state->forced_mode < 0) {
                        state->forced_mode = state->current_mode;
                    } else {
                        state->forced_mode = -1;
                    }
                    break;
                case BTN_Y:
                    state->neon_copy_enabled = state->neon_copy_enabled ? SDL_FALSE : SDL_TRUE;
                    break;
                case BTN_L1:
                    state->blend_mode = (state->blend_mode + 1) % PIXELS_BLEND_MAX;
                    break;
                case BTN_R1:
                    state->upload_path = (state->upload_path + 1) % PIXELS_UPLOAD_MAX;
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
