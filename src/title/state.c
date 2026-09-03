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

    const TitleCategory categories[TITLE_CATEGORY_COUNT] = {
        {"Geometry & 3D", {
            {"Hardware Double Buffer", "sdl2_bench_double_buf",
             "Rotating shape and particle field exercising MI_GFX/MI_SYS hardware double buffering. "
             "Shape and render mode are togglable in-app via UP/DOWN and X.",
             NULL, NULL},
            {"Turntable Model: Sheep", "sdl2_obj_model_loader",
             "Loads the bundled sheep OBJ/MTL model and renders it as an auto-rotating turntable "
             "via SDL_RenderGeometry -- exercises a hand-written CPU-side model/view/projection, "
             "clipping, culling, and lighting pipeline.",
             "OBJ_MODEL_NAME", "sheep"},
            {"Turntable Model: Miyoo", "sdl2_obj_model_loader",
             "Loads the bundled Miyoo shell OBJ/MTL model and renders it as an auto-rotating "
             "turntable via SDL_RenderGeometry, exercising the same CPU-side rasterizer pipeline "
             "as the sheep model.",
             "OBJ_MODEL_NAME", "miyoo"},
            {"Rotating Mesh (NEON)", "sdl2_render_suite",
             "Rotating icosahedron-subdivided mesh with particle trails, projected via a "
             "NEON-optimized SoA vertex pipeline -- render_suite's most architecturally distinct scene.",
             "RS_FORCE_SCENE", "geometry"},
        }, 4},
        {"2D Rendering", {
            {"Solid Fill Rate", "sdl2_render_suite",
             "Stress-scaled full/partial screen colored-rect fills measuring raw pixel fill throughput.",
             "RS_FORCE_SCENE", "fill"},
            {"Texture Blit Throughput", "sdl2_render_suite",
             "Rotating/pulsing scaled texture blits measuring texture sampling and blit cost.",
             "RS_FORCE_SCENE", "texture"},
            {"Line & Shape Drawing", "sdl2_render_suite",
             "Grid of line/quad-built cube columns stressing line and geometry throughput.",
             "RS_FORCE_SCENE", "lines"},
            {"Resolution Scaling", "sdl2_render_suite",
             "Cycles render target resolutions and scaling modes -- logical, viewport, and "
             "texture-target scaling.",
             "RS_FORCE_SCENE", "scaling"},
            {"Memory Management", "sdl2_render_suite",
             "Allocates and frees a pool of textures with lifetime tracking, exercising texture "
             "alloc/free churn. Currently regressed -- see README known bugs.",
             "RS_FORCE_SCENE", "memory"},
            {"Pixel Operations", "sdl2_render_suite",
             "CPU-software pixel-buffer effects (plasma, fire, mandelbrot, cellular automaton) "
             "uploaded as a texture each frame.",
             "RS_FORCE_SCENE", "pixels"},
            {"Sprite Blit Stress Test", "sdl2_sprite_bench",
             "Auto-ramping bouncing-sprite count stress test isolating the raw texture blit/present path.",
             NULL, NULL},
            {"AA Shapes (SDL2_gfx)", "sdl2_gfx_bench",
             "Antialiased circle/ellipse/shape primitives via SDL2_gfx's software rasterizer, "
             "independent of the hardware-accelerated SDL_Renderer path.",
             "GB_FORCE_SCENE", "aa_shapes"},
            {"Rounded Rects (SDL2_gfx)", "sdl2_gfx_bench",
             "Rounded-rectangle primitives (filled/outline) via SDL2_gfx's software rasterizer.",
             "GB_FORCE_SCENE", "rounded_rects"},
            {"Polygons (SDL2_gfx)", "sdl2_gfx_bench",
             "Filled/antialiased N-gon polygons via SDL2_gfx's software rasterizer.",
             "GB_FORCE_SCENE", "polygons"},
            {"Bezier Curves (SDL2_gfx)", "sdl2_gfx_bench",
             "Cubic bezier curves via SDL2_gfx's software rasterizer.",
             "GB_FORCE_SCENE", "bezier"},
            {"Thick Lines (SDL2_gfx)", "sdl2_gfx_bench",
             "Thick/wide line primitives via SDL2_gfx's software rasterizer.",
             "GB_FORCE_SCENE", "thick_lines"},
        }, 12},
        {"Shader Effects", {
            {"Sunrise Gradient", "sdl2_gl_fbo_effects",
             "GLES2 shader effect rendered offscreen into an FBO and composited through the 2D renderer.",
             "RSGL_FORCE_EFFECT", "Sunrise Gradient"},
            {"Soft Waves", "sdl2_gl_fbo_effects",
             "GLES2 shader effect rendered offscreen into an FBO and composited through the 2D renderer.",
             "RSGL_FORCE_EFFECT", "Soft Waves"},
            {"Scanline Glow", "sdl2_gl_fbo_effects",
             "GLES2 shader effect rendered offscreen into an FBO and composited through the 2D renderer.",
             "RSGL_FORCE_EFFECT", "Scanline Glow"},
            {"Floating Orbs", "sdl2_gl_fbo_effects",
             "GLES2 shader effect rendered offscreen into an FBO and composited through the 2D renderer.",
             "RSGL_FORCE_EFFECT", "Floating Orbs"},
            {"Aurora Borealis", "sdl2_gl_fbo_effects",
             "GLES2 shader effect rendered offscreen into an FBO and composited through the 2D renderer.",
             "RSGL_FORCE_EFFECT", "Aurora Borealis"},
            {"Nebula Clouds", "sdl2_gl_fbo_effects",
             "GLES2 shader effect rendered offscreen into an FBO and composited through the 2D renderer.",
             "RSGL_FORCE_EFFECT", "Nebula Clouds"},
            {"Fire Effect", "sdl2_gl_fbo_effects",
             "GLES2 shader effect rendered offscreen into an FBO and composited through the 2D renderer.",
             "RSGL_FORCE_EFFECT", "Fire Effect"},
            {"Lightning Storm", "sdl2_gl_fbo_effects",
             "GLES2 shader effect rendered offscreen into an FBO and composited through the 2D renderer.",
             "RSGL_FORCE_EFFECT", "Lightning Storm"},
            {"Crystal Cavern", "sdl2_gl_fbo_effects",
             "GLES2 shader effect rendered offscreen into an FBO and composited through the 2D renderer.",
             "RSGL_FORCE_EFFECT", "Crystal Cavern"},
            {"Plasma Flow", "sdl2_gl_fbo_effects",
             "GLES2 shader effect rendered offscreen into an FBO and composited through the 2D renderer.",
             "RSGL_FORCE_EFFECT", "Plasma Flow"},
            {"Electric Grid", "sdl2_gl_fbo_effects",
             "GLES2 shader effect rendered offscreen into an FBO and composited through the 2D renderer.",
             "RSGL_FORCE_EFFECT", "Electric Grid"},
            {"Ocean Depths", "sdl2_gl_fbo_effects",
             "GLES2 shader effect rendered offscreen into an FBO and composited through the 2D renderer.",
             "RSGL_FORCE_EFFECT", "Ocean Depths"},
            {"Retro Sun", "sdl2_gl_fbo_effects",
             "GLES2 shader effect rendered offscreen into an FBO and composited through the 2D renderer.",
             "RSGL_FORCE_EFFECT", "Retro Sun"},
            {"Digital Rain", "sdl2_gl_fbo_effects",
             "GLES2 shader effect rendered offscreen into an FBO and composited through the 2D renderer.",
             "RSGL_FORCE_EFFECT", "Digital Rain"},
            {"Chromatic Shift", "sdl2_gl_fbo_effects",
             "GLES2 shader effect rendered offscreen into an FBO and composited through the 2D renderer.",
             "RSGL_FORCE_EFFECT", "Chromatic Shift"},
        }, 15},
        {"Audio", {
            {"Audio Playback Visualizer", "sdl2_audio_bench",
             "Waveform visualizations driven by the MMIYOO audio backend, exercising audio "
             "playback alongside rendering. Visualization mode is cycled in-app.",
             NULL, NULL},
        }, 1},
        {"Interactive", {
            {"Space Shooter Stress Test", "sdl2_space_bench",
             "Interactive space-shooter stress test -- sprites, particles, and GL effects "
             "together under real gameplay load.",
             NULL, NULL},
        }, 1},
        {"Quit", {
            {"Quit", NULL, NULL, NULL, NULL},
        }, 1},
    };
    memcpy(state->categories, categories, sizeof(categories));

    state->selected_category = 0;
    state->selected_entry = 0;
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
        const int entry_count = state->categories[state->selected_category].entry_count;
        int next = state->selected_entry + delta;
        if (next < 0) {
            next = entry_count - 1;
        } else if (next >= entry_count) {
            next = 0;
        }
        state->selected_entry = next;
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

void title_state_move_category(TitleState *state, int delta)
{
    if (!state || delta == 0) {
        return;
    }
    int next = state->selected_category + delta;
    if (next < 0) {
        next = TITLE_CATEGORY_COUNT - 1;
    } else if (next >= TITLE_CATEGORY_COUNT) {
        next = 0;
    }
    state->selected_category = next;
    state->selected_entry = 0;
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

const TitleSuiteEntry *title_state_selected_entry(const TitleState *state)
{
    if (!state || state->selected_category < 0 || state->selected_category >= TITLE_CATEGORY_COUNT) {
        return NULL;
    }
    const TitleCategory *category = &state->categories[state->selected_category];
    if (state->selected_entry < 0 || state->selected_entry >= category->entry_count) {
        return NULL;
    }
    return &category->entries[state->selected_entry];
}

SDL_bool title_state_quit_selected(const TitleState *state)
{
    return state && state->selected_category == TITLE_CATEGORY_COUNT - 1;
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
    state->info_modal_category = state->selected_category;
    state->info_modal_entry = state->selected_entry;
    state->mode = TITLE_MODE_INFO_MODAL;
}

void title_state_close_info_modal(TitleState *state)
{
    if (!state) {
        return;
    }
    state->mode = TITLE_MODE_MENU;
}
