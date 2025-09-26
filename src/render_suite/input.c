#include "render_suite/input.h"

#include <SDL2/SDL.h>

#include "controller_input.h"

SDL_bool rs_handle_input(RenderSuiteState *state, BenchMetrics *metrics)
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
                case BTN_L2:
                    state->active_scene = (SceneKind)((state->active_scene + 1) % SCENE_MAX);
                    state->auto_cycle = SDL_FALSE;
                    break;
                case BTN_R2:
                    state->active_scene = (SceneKind)((state->active_scene == 0) ? (SCENE_MAX - 1) : (state->active_scene - 1));
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
                case BTN_X:
                    bench_reset_metrics(metrics);
                    break;
                default:
                    break;
            }
        }
    }
    return SDL_TRUE;
}
