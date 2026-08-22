#include "title/state.h"

#include <stdio.h>
#include <string.h>

void title_state_init(TitleState *state)
{
    if (!state) {
        return;
    }

    memset(state, 0, sizeof(*state));

    const TitleSuiteEntry suites[TITLE_SUITE_COUNT] = {
        {"Render Suite",         "sdl2_render_suite"},
        {"GL FBO Effects",       "sdl2_gl_fbo_effects"},
        {"Hardware Double Buffer", "sdl2_bench_double_buf"},
        {"Space Bench",          "sdl2_space_bench"},
        {"Sprite Bench",         "sdl2_sprite_bench"},
        {"Audio Bench",          "sdl2_audio_bench"},
    };
    memcpy(state->suites, suites, sizeof(suites));

    state->selected_suite = 0;
    state->logical_res = TITLE_RES_NATIVE;
    state->vsync_mode = BENCH_VSYNC_STATUS_OFF;
    state->input_mode = BENCH_INPUT_SOURCE_KEYBOARD;
    state->frame_limit_fps = 0;
    state->focus = TITLE_FOCUS_LIST;
    state->config_row = 0;
    state->mode = TITLE_MODE_MENU;
}

void title_state_move_selection(TitleState *state, int delta)
{
    if (!state || delta == 0) {
        return;
    }

    if (state->focus == TITLE_FOCUS_LIST) {
        int next = state->selected_suite + delta;
        if (next < 0) {
            next = TITLE_SUITE_COUNT - 1;
        } else if (next >= TITLE_SUITE_COUNT) {
            next = 0;
        }
        state->selected_suite = next;
    } else {
        int next = state->config_row + delta;
        if (next < 0) {
            next = TITLE_CONFIG_COUNT - 1;
        } else if (next >= TITLE_CONFIG_COUNT) {
            next = 0;
        }
        state->config_row = next;
    }
}

void title_state_toggle_focus(TitleState *state)
{
    if (!state) {
        return;
    }
    state->focus = (state->focus == TITLE_FOCUS_LIST) ? TITLE_FOCUS_CONFIG : TITLE_FOCUS_LIST;
}

static void title_cycle_enum(int *value, int count, int delta)
{
    int next = (*value + delta) % count;
    if (next < 0) {
        next += count;
    }
    *value = next;
}

void title_state_cycle_config(TitleState *state, int delta)
{
    if (!state || state->focus != TITLE_FOCUS_CONFIG || delta == 0) {
        return;
    }

    switch ((TitleConfigRow)state->config_row) {
        case TITLE_CONFIG_RESOLUTION: {
            int res = (int)state->logical_res;
            title_cycle_enum(&res, TITLE_RES_COUNT, delta);
            state->logical_res = (TitleLogicalRes)res;
            break;
        }
        case TITLE_CONFIG_VSYNC: {
            int vsync = (int)state->vsync_mode;
            title_cycle_enum(&vsync, 3, delta);
            state->vsync_mode = (BenchVSyncStatus)vsync;
            break;
        }
        case TITLE_CONFIG_FRAME_LIMIT: {
            static const int steps[] = {0, 30, 60};
            int index = 0;
            for (int i = 0; i < 3; i++) {
                if (steps[i] == state->frame_limit_fps) {
                    index = i;
                    break;
                }
            }
            title_cycle_enum(&index, 3, delta);
            state->frame_limit_fps = steps[index];
            break;
        }
        case TITLE_CONFIG_INPUT_MODE: {
            int mode = (int)state->input_mode;
            title_cycle_enum(&mode, 2, delta);
            state->input_mode = (BenchInputSource)mode;
            break;
        }
        default:
            break;
    }
}

const TitleSuiteEntry *title_state_selected_suite(const TitleState *state)
{
    if (!state || state->selected_suite < 0 || state->selected_suite >= TITLE_SUITE_COUNT) {
        return NULL;
    }
    return &state->suites[state->selected_suite];
}

void title_state_set_child_error(TitleState *state, const char *bin_name, SDL_bool crashed, int code_or_signal)
{
    if (!state) {
        return;
    }
    state->mode = TITLE_MODE_CHILD_ERROR;
    if (crashed) {
        snprintf(state->error_message, sizeof(state->error_message),
                 "%s exited with signal %d", bin_name ? bin_name : "suite", code_or_signal);
    } else {
        snprintf(state->error_message, sizeof(state->error_message),
                 "%s exited with code %d", bin_name ? bin_name : "suite", code_or_signal);
    }
}

void title_state_clear_error(TitleState *state)
{
    if (!state) {
        return;
    }
    state->mode = TITLE_MODE_MENU;
    state->error_message[0] = '\0';
}
