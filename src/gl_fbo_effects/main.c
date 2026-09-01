#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "bench_common.h"
#include "gl_fbo_effects/input.h"
#include "gl_fbo_effects/scenes/effects.h"
#include "gl_fbo_effects/state.h"
#include "common/hotkeys.h"
#include "common/loading_screen.h"
#include "common/overlay_rows.h"

static const OverlayRowSpec g_rsgl_rows[] = {
    {OVERLAY_ROW_CUSTOM, {240, 194, 94, 255}, 0, "%s"},
    {OVERLAY_ROW_FPS, {255, 255, 255, 255}, 0, NULL},
    {OVERLAY_ROW_FRAME_TIME, {0, 200, 255, 255}, 0, NULL},
    {OVERLAY_ROW_DRAW_CALLS, {0, 255, 160, 255}, 0, NULL},
    {OVERLAY_ROW_TEXTURE_SWITCHES, {0, 255, 160, 255}, 0, NULL},
    {OVERLAY_ROW_CUSTOM, {255, 200, 0, 255}, 0, "%s"},
};

static const OverlayKeybind g_rsgl_keybinds[] = {
    {"SELECT", "Toggle overlay"},
    {"MENU", "Reset metrics"},
    {"Y", "Next effect"},
    {"A", "Toggle auto cycle"},
    {"X", "Change FBO size"},
};

static void rsgl_print_info(void)
{
    SDL_version ver;
    SDL_GetVersion(&ver);
    printf("=== SDL2 GL Effect Suite ===\n");
    printf("SDL Version: %d.%d.%d\n", ver.major, ver.minor, ver.patch);
    printf("SDL Revision: %s\n", SDL_GetRevision());
    printf("Platform: %s\n", SDL_GetPlatform());
    printf("============================\n\n");
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    if (TTF_Init() < 0) {
        fprintf(stderr, "TTF_Init failed: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("SDL2 GL Effect Suite",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          BENCH_NATIVE_W,
                                          BENCH_NATIVE_H,
                                          SDL_WINDOW_SHOWN);
    if (!window) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    /* SDL_CreateRenderer force-ORs in SDL_RENDERER_PRESENTVSYNC in this SDL2
     * fork regardless of flags -- the hint is the only way to turn it off. */
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "0");
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    int logical_w, logical_h;
    bench_display_config_load(&logical_w, &logical_h);
    bench_display_config_apply(renderer, logical_w, logical_h);
    bench_frame_limit_load();

    bench_driver_init(window, renderer);

    BenchLoadingScreen loading;
    SDL_bool loading_active = bench_loading_begin(&loading,
                                                  window,
                                                  renderer,
                                                  BENCH_LOADING_STYLE_GL);
    if (loading_active) {
        bench_loading_step(&loading, 0.1f, "Preparing state objects");
    }

    rsgl_print_info();

    RsglState state;
    rsgl_state_init(&state);
    state.screen_width = logical_w;
    state.screen_height = logical_h;
    if (loading_active) {
        bench_loading_step(&loading, 0.2f, "Loading fonts");
    }
    state.font = bench_load_font(16);

    BenchMetrics metrics;
    bench_reset_metrics(&metrics);
    if (loading_active) {
        bench_loading_step(&loading, 0.3f, "Allocating overlay");
    }

    BenchOverlay *overlay = bench_overlay_create(renderer, logical_w, 16, 12);
    if (!overlay) {
        fprintf(stderr, "Overlay creation failed\n");
        if (loading_active) {
            bench_loading_abort(&loading);
        }
        rsgl_state_destroy(&state);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    if (loading_active) {
        /* GL effects acquire the shared context lazily during init. */
        bench_loading_step(&loading, 0.45f, "Preparing GL context");
    }

    if (!rsgl_effects_init(&state, renderer)) {
        fprintf(stderr, "GL effect initialisation failed\n");
        if (loading_active) {
            bench_loading_abort(&loading);
        }
        bench_overlay_destroy(overlay);
        rsgl_state_destroy(&state);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    state.effect_count = rsgl_effect_count();
    if (state.effect_count == 0) {
        state.effect_index = 0;
    }

    rsgl_effects_warmup(&state);
    if (loading_active) {
        bench_loading_step(&loading, 0.9f, "Warming up shaders");
    }
    bench_overlay_configure(overlay, g_rsgl_rows, (int)SDL_arraysize(g_rsgl_rows),
                            g_rsgl_keybinds, (int)SDL_arraysize(g_rsgl_keybinds));

    if (loading_active) {
        bench_loading_step(&loading, 1.0f, "GL suite ready");
        bench_loading_finish(&loading);
        loading_active = SDL_FALSE;
    }

    Uint64 counter = SDL_GetPerformanceCounter();
    const Uint64 freq = SDL_GetPerformanceFrequency();

    SDL_bool running = SDL_TRUE;
    while (running) {
        const Uint64 frame_start_counter = SDL_GetPerformanceCounter();

        if (!rsgl_handle_input(&state, &metrics, overlay)) {
            break;
        }

        const double delta = bench_get_delta_seconds(&counter, freq);
        metrics.draw_calls = 0;
        metrics.vertices_rendered = 0;
        metrics.triangles_rendered = 0;
        metrics.texture_switches = 0;
        metrics.geometry_batches = 0;
        metrics.scaling_operations = 0;
        metrics.pixel_operations = 0;

        if (state.fbo_dirty) {
            rsgl_effects_apply_fbo_size(&state, renderer);
        }

        SDL_SetRenderDrawColor(renderer, 8, 10, 18, 255);
        SDL_RenderClear(renderer);

        rsgl_effects_render(&state, renderer, &metrics, delta);

        bench_overlay_present(overlay, renderer, &metrics, 0, 0);
        SDL_RenderPresent(renderer);

        bench_update_metrics(&metrics, delta * 1000.0);
        bench_frame_limit_wait(frame_start_counter);
        state.running = running;

        const int effect_count = state.effect_count;
        const int effect_index = (effect_count > 0) ? state.effect_index % effect_count : 0;
        const char *fbo_label = "N/A";
        if (state.fbo_size_index >= 0 && state.fbo_size_index < RSGL_FBO_PRESET_COUNT) {
            const RsglFboPreset *preset = &rsgl_fbo_presets[state.fbo_size_index];
            if (preset && preset->label) {
                fbo_label = preset->label;
            }
        }
        char effect_label[64];
        snprintf(effect_label, sizeof(effect_label), "Effect: %s (%d/%d) | Auto %s",
                 rsgl_effect_name(effect_index), effect_index + 1, effect_count,
                 state.auto_cycle ? "ON" : "OFF");
        char timer_label[64];
        snprintf(timer_label, sizeof(timer_label), "FBO %s | Timer %.2fs", fbo_label, state.elapsed_time);
        const char *custom_values[] = {effect_label, timer_label};
        bench_overlay_update(overlay, &metrics, custom_values, (int)SDL_arraysize(custom_values));
    }

    bench_driver_shutdown();
    rsgl_effects_cleanup(&state);
    rsgl_state_destroy(&state);
    bench_overlay_destroy(overlay);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
