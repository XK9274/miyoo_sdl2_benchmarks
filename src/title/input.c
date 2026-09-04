#include "title/input.h"

#include <SDL2/SDL.h>

#include "controller_input.h"
#include "common/driver_support.h"

#define TITLE_NAV_REPEAT_DELAY_MS 350
#define TITLE_NAV_REPEAT_INTERVAL_MS 90

static SDL_Keycode g_held_nav_key = 0;
static Uint32 g_held_since = 0;
static Uint32 g_last_repeat = 0;

static void title_track_nav_hold(SDL_Keycode sym, SDL_bool pressed)
{
    if (sym != BTN_UP && sym != BTN_DOWN) {
        return;
    }
    if (pressed) {
        g_held_nav_key = sym;
        g_held_since = SDL_GetTicks();
        g_last_repeat = g_held_since;
    } else if (g_held_nav_key == sym) {
        g_held_nav_key = 0;
    }
}

/* Synthesizes repeat while Up/Down is held -- joystick events don't auto-repeat like keyboard does. */
static void title_apply_nav_repeat(TitleState *state)
{
    if (g_held_nav_key == 0) {
        return;
    }
    const Uint32 now = SDL_GetTicks();
    if (now - g_held_since < TITLE_NAV_REPEAT_DELAY_MS) {
        return;
    }
    if (now - g_last_repeat < TITLE_NAV_REPEAT_INTERVAL_MS) {
        return;
    }
    g_last_repeat = now;
    title_state_move_selection(state, g_held_nav_key == BTN_UP ? -1 : 1);
}

TitleAction title_handle_input(TitleState *state)
{
    TitleAction action = TITLE_ACTION_NONE;
    SDL_Event e;

    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            return TITLE_ACTION_QUIT;
        }

        SDL_Keycode raw_sym = 0;
        SDL_bool pressed = SDL_FALSE;
        if (!bench_driver_translate_button_event(&e, &raw_sym, &pressed)) {
            continue;
        }
        title_track_nav_hold(raw_sym, pressed);

        if (!pressed) {
            continue;
        }
        const SDL_Keycode sym = raw_sym;

        if (state->mode == TITLE_MODE_CHILD_ERROR) {
            /* Any input dismisses the error banner; swallow it, don't also act on it. */
            title_state_clear_error(state);
            continue;
        }
        if (state->mode == TITLE_MODE_INFO_MODAL) {
            /* Any input dismisses the modal; swallow it, don't also act on it. */
            title_state_close_info_modal(state);
            continue;
        }

        switch (sym) {
            case BTN_EXIT:
            case BTN_MENU:
            case SDLK_ESCAPE:
                return TITLE_ACTION_QUIT;
            case BTN_UP:
                title_state_move_selection(state, -1);
                break;
            case BTN_DOWN:
                title_state_move_selection(state, 1);
                break;
            case BTN_LEFT:
                if (state->focus == TITLE_FOCUS_CONFIG && state->editing) {
                    title_state_cycle_config(state, -1);
                } else if (state->focus == TITLE_FOCUS_LIST) {
                    title_state_move_category(state, -1);
                }
                break;
            case BTN_RIGHT:
                if (state->focus == TITLE_FOCUS_CONFIG && state->editing) {
                    title_state_cycle_config(state, 1);
                } else if (state->focus == TITLE_FOCUS_LIST) {
                    title_state_move_category(state, 1);
                }
                break;
            case BTN_L1:
                title_state_move_focus_horizontal(state, -1);
                break;
            case BTN_R1:
                title_state_move_focus_horizontal(state, 1);
                break;
            case BTN_SELECT:
                title_state_open_info_modal(state);
                break;
            case BTN_A:
            case BTN_START:
                if (state->focus == TITLE_FOCUS_CONFIG) {
                    title_state_toggle_edit(state);
                } else {
                    action = TITLE_ACTION_LAUNCH;
                }
                break;
            default:
                break;
        }
    }

    if (state->mode == TITLE_MODE_MENU) {
        title_apply_nav_repeat(state);
    }

    return action;
}
