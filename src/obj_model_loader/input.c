#include "obj_model_loader/input.h"

#include "common/driver_support.h"
#include "common/hotkeys.h"
#include "controller_input.h"

SDL_bool obj_handle_input(SDL_Renderer *renderer, ObjModelLoaderState *state, BenchMetrics *metrics, BenchOverlay *overlay)
{
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            return SDL_FALSE;
        }

        SDL_Keycode sym = 0;
        SDL_bool pressed = SDL_FALSE;
        if (!bench_driver_translate_button_event(&e, &sym, &pressed)) {
            continue;
        }

        /* Dpad and L1/R1 are held state (for smooth panning/zoom), tracked
         * on both press and release; everything else only acts on press. */
        switch (sym) {
            case BTN_UP:
                state->pan_held_up = pressed;
                if (pressed) state->auto_rotate = SDL_FALSE;
                break;
            case BTN_DOWN:
                state->pan_held_down = pressed;
                if (pressed) state->auto_rotate = SDL_FALSE;
                break;
            case BTN_LEFT:
                state->pan_held_left = pressed;
                if (pressed) state->auto_rotate = SDL_FALSE;
                break;
            case BTN_RIGHT:
                state->pan_held_right = pressed;
                if (pressed) state->auto_rotate = SDL_FALSE;
                break;
            case BTN_L1:
                state->zoom_held_in = pressed;
                break;
            case BTN_R1:
                state->zoom_held_out = pressed;
                break;
            default:
                break;
        }

        if (!pressed) {
            continue;
        }

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
                obj_state_cycle_model(renderer, state, -1);
                break;
            case BTN_R2:
                obj_state_cycle_model(renderer, state, 1);
                break;
            case BTN_A:
                state->auto_rotate = !state->auto_rotate;
                break;
            case BTN_B:
                state->wireframe = !state->wireframe;
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
    return SDL_TRUE;
}
