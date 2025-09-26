#include "software_buf/input.h"

#include <SDL2/SDL.h>

#include "controller_input.h"

SDL_bool sb_handle_input(SoftwareBenchState *state, BenchMetrics *metrics)
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
                    state->particle_count += 100;
                    if (state->particle_count > SB_MAX_PARTICLES) {
                        state->particle_count = SB_MAX_PARTICLES;
                    }
                    break;
                case BTN_B:
                    state->particle_count -= 100;
                    if (state->particle_count < 0) {
                        state->particle_count = 0;
                    }
                    break;
                case BTN_X:
                    state->render_mode = (state->render_mode + 1) % 3;
                    break;
                case BTN_Y:
                    state->show_particles = !state->show_particles;
                    break;
                case BTN_L1:
                    state->stress_grid = !state->stress_grid;
                    break;
                case BTN_R1:
                    state->show_cube = !state->show_cube;
                    break;
                case BTN_L2:
                    state->particle_speed -= 20.0f;
                    if (state->particle_speed < 10.0f) {
                        state->particle_speed = 10.0f;
                    }
                    break;
                case BTN_R2:
                    state->particle_speed += 20.0f;
                    if (state->particle_speed > 600.0f) {
                        state->particle_speed = 600.0f;
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
