#include "title/input.h"

#include <SDL2/SDL.h>

#include "controller_input.h"
#include "common/driver_support.h"

TitleAction title_handle_input(TitleState *state)
{
    TitleAction action = TITLE_ACTION_NONE;
    SDL_Event e;

    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            return TITLE_ACTION_QUIT;
        }

        const SDL_Keycode sym = bench_driver_translate_event(&e);
        if (sym == 0) {
            continue;
        }

        if (state->mode == TITLE_MODE_CHILD_ERROR) {
            /* Any input dismisses the error banner; swallow it, don't also act on it. */
            title_state_clear_error(state);
            continue;
        }

        switch (sym) {
            case BTN_EXIT:
                return TITLE_ACTION_QUIT;
            case BTN_UP:
                title_state_move_selection(state, -1);
                break;
            case BTN_DOWN:
                title_state_move_selection(state, 1);
                break;
            case BTN_LEFT:
                title_state_cycle_config(state, -1);
                break;
            case BTN_RIGHT:
                title_state_cycle_config(state, 1);
                break;
            case BTN_L1:
            case BTN_R1:
                title_state_toggle_focus(state);
                break;
            case BTN_A:
            case BTN_START:
                action = TITLE_ACTION_LAUNCH;
                break;
            default:
                break;
        }
    }

    return action;
}
