#include "title/state.h"

#include <stdio.h>
#include <string.h>

#include "title/config_panel.h"

void title_state_init(TitleState *state)
{
    if (!state) {
        return;
    }

    memset(state, 0, sizeof(*state));

    const TitleSuiteEntry suites[TITLE_SUITE_COUNT] = {
        {"Render Suite", "sdl2_render_suite",
         "Cycles 7 2D rendering scenes -- fills, lines, textures, geometry, scaling, memory, "
         "pixel ops -- exercising the MMIYOO hardware-accelerated SDL2 renderer."},
        {"GL FBO Effects", "sdl2_gl_fbo_effects",
         "Shader-based effects rendered offscreen via GLES2 into an FBO, composited through "
         "the 2D renderer -- exercises the shared GLES2/SwiftShader pipeline."},
        {"Hardware Double Buffer", "sdl2_bench_double_buf",
         "Rotating cube and particle field exercising MI_GFX/MI_SYS hardware double buffering."},
        {"Space Bench", "sdl2_space_bench",
         "Interactive space-shooter stress test -- sprites, particles, and GL effects "
         "together under real gameplay load."},
        {"Sprite Bench", "sdl2_sprite_bench",
         "Auto-ramping bouncing-sprite count stress test isolating the raw texture blit/present path."},
        {"Audio Bench", "sdl2_audio_bench",
         "Waveform visualizations driven by the MMIYOO audio backend, exercising audio "
         "playback alongside rendering."},
        {"SDL2_gfx Bench", "sdl2_gfx_bench",
         "Cycles antialiased shapes, rounded rects, polygons, bezier curves, and thick lines "
         "via SDL2_gfx's software primitive renderer -- exercises CPU-side rasterization "
         "independent of the hardware-accelerated SDL_Renderer path."},
        {"Obj Model Loader", "sdl2_obj_model_loader",
         "Loads a Wavefront OBJ/MTL model and renders it as an auto-rotating turntable "
         "via SDL_RenderGeometry -- exercises OBJ/MTL parsing, texture loading, and a "
         "hand-written CPU-side model/view/projection, clipping, culling, and lighting pipeline."},
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
        /* +1 for the trailing Quit row at index TITLE_SUITE_COUNT. */
        int next = state->selected_suite + delta;
        if (next < 0) {
            next = TITLE_SUITE_COUNT;
        } else if (next > TITLE_SUITE_COUNT) {
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
        state->editing = SDL_FALSE; /* editing is row-specific -- changing row exits it */
    }
}

void title_state_move_focus_horizontal(TitleState *state, int delta)
{
    if (!state || delta == 0) {
        return;
    }
    state->editing = SDL_FALSE;
    /* List sits left, config sits right -- move toward that side. */
    if (delta < 0) {
        state->focus = TITLE_FOCUS_LIST;
    } else {
        state->focus = TITLE_FOCUS_CONFIG;
    }
}

void title_state_toggle_edit(TitleState *state)
{
    if (!state || state->focus != TITLE_FOCUS_CONFIG) {
        return;
    }
    if (title_config_row_disabled((TitleConfigRow)state->config_row)) {
        return;
    }
    state->editing = !state->editing;
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
    if (title_config_row_disabled((TitleConfigRow)state->config_row)) {
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

SDL_bool title_state_quit_selected(const TitleState *state)
{
    return state && state->selected_suite == TITLE_SUITE_COUNT;
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

void title_state_open_info_modal(TitleState *state)
{
    if (!state || state->focus != TITLE_FOCUS_LIST || title_state_quit_selected(state)) {
        return;
    }
    state->info_modal_suite = state->selected_suite;
    state->mode = TITLE_MODE_INFO_MODAL;
}

void title_state_close_info_modal(TitleState *state)
{
    if (!state) {
        return;
    }
    state->mode = TITLE_MODE_MENU;
}
