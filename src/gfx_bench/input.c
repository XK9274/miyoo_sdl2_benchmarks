#include "gfx_bench/input.h"

#include <SDL2/SDL.h>

#include "controller_input.h"
#include "common/driver_support.h"

SDL_bool gb_handle_input(GfxBenchState *state, BenchMetrics *metrics)
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
                case BTN_L2:
                    state->active_scene = (GfxBenchSceneKind)((state->active_scene + 1) % GB_SCENE_MAX);
                    state->auto_cycle = SDL_FALSE;
                    break;
                case BTN_R2:
                    state->active_scene = (GfxBenchSceneKind)((state->active_scene == 0) ? (GB_SCENE_MAX - 1) : (state->active_scene - 1));
                    state->auto_cycle = SDL_FALSE;
                    break;
                case BTN_A:
                    state->auto_cycle = !state->auto_cycle;
                    break;
                case BTN_B:
                    state->stress_level++;
                    if (state->stress_level > 10) {
                        state->stress_level = 1;
                    }
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
