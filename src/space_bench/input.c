#include "space_bench/input.h"

#include <SDL2/SDL.h>

#include "controller_input.h"

SDL_bool space_handle_input(SpaceBenchState *state, BenchMetrics *metrics)
{
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            return SDL_FALSE;
        }

        if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
            const SDL_bool pressed = (e.type == SDL_KEYDOWN) ? SDL_TRUE : SDL_FALSE;

            // Handle game over input
            if (state->game_state == SPACE_GAME_OVER) {
                if (pressed) {
                    switch (e.key.keysym.sym) {
                        case BTN_LEFT:
                            state->gameover_selected = SPACE_GAMEOVER_RETRY;
                            break;
                        case BTN_RIGHT:
                            state->gameover_selected = SPACE_GAMEOVER_QUIT;
                            break;
                        case BTN_A:
                            // Block A input during countdown
                            if (state->gameover_countdown <= 0.0f) {
                                if (state->gameover_selected == SPACE_GAMEOVER_QUIT) {
                                    return SDL_FALSE; // Exit game
                                } else {
                                    // Restart game
                                    space_state_init(state);
                                    return SDL_TRUE;
                                }
                            }
                            break;
                        case BTN_START:
                        case BTN_EXIT:
                        case SDLK_ESCAPE:
                            return SDL_FALSE;
                            break;
                    }
                }
                continue; // Skip normal game input when in game over
            }

            switch (e.key.keysym.sym) {
                case BTN_START:
                case BTN_EXIT:
                case SDLK_ESCAPE:
                    if (pressed) {
                        return SDL_FALSE;
                    }
                    break;
                case BTN_SELECT:
                    if (pressed && metrics) {
                        bench_reset_metrics(metrics);
                    }
                    break;
                case BTN_UP:
                    state->input.up = pressed;
                    break;
                case BTN_DOWN:
                    state->input.down = pressed;
                    break;
                case BTN_LEFT:
                    state->input.left = pressed;
                    break;
                case BTN_RIGHT:
                    state->input.right = pressed;
                    break;
                case BTN_A:
                    state->input.fire_gun = pressed;
                    break;
                case BTN_B:
                    state->input.fire_laser = pressed;
                    break;
                default:
                    break;
            }
        }
    }

    return SDL_TRUE;
}
