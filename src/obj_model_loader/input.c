#include "obj_model_loader/input.h"

#include "common/driver_support.h"
#include "common/hotkeys.h"
#include "controller_input.h"

#define OBJ_ORBIT_STEP_RADIANS (15.0f * 3.14159265358979323846f / 180.0f)
#define OBJ_ZOOM_FACTOR 0.1f /* fraction of current distance per press -- scales with model size, unlike a fixed step */

SDL_bool obj_handle_input(SDL_Renderer *renderer, ObjModelLoaderState *state, BenchMetrics *metrics, BenchOverlay *overlay)
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
                case BTN_UP:
                    camera3d_orbit(&state->camera, 0.0f, OBJ_ORBIT_STEP_RADIANS);
                    state->auto_rotate = SDL_FALSE;
                    break;
                case BTN_DOWN:
                    camera3d_orbit(&state->camera, 0.0f, -OBJ_ORBIT_STEP_RADIANS);
                    state->auto_rotate = SDL_FALSE;
                    break;
                case BTN_LEFT:
                    camera3d_orbit(&state->camera, -OBJ_ORBIT_STEP_RADIANS, 0.0f);
                    state->auto_rotate = SDL_FALSE;
                    break;
                case BTN_RIGHT:
                    camera3d_orbit(&state->camera, OBJ_ORBIT_STEP_RADIANS, 0.0f);
                    state->auto_rotate = SDL_FALSE;
                    break;
                case BTN_L1:
                    camera3d_zoom(&state->camera, -state->camera.distance * OBJ_ZOOM_FACTOR);
                    break;
                case BTN_R1:
                    camera3d_zoom(&state->camera, state->camera.distance * OBJ_ZOOM_FACTOR);
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
    }
    return SDL_TRUE;
}
