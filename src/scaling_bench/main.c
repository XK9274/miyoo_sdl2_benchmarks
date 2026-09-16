#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <stdio.h>

#include "bench_common.h"
#include "scaling_bench/input.h"
#include "scaling_bench/render.h"
#include "scaling_bench/state.h"
#include "common/hotkeys.h"
#include "common/loading_screen.h"
#include "common/overlay_rows.h"
#ifdef DEBUG_BUILD
#include "common/overlay_debug_stats.h"
#endif

static const OverlayRowSpec g_scaling_rows[] = {
    {OVERLAY_ROW_CUSTOM, {255, 180, 120, 255}, 0, "%s"},
    {OVERLAY_ROW_FPS, {255, 255, 255, 255}, 0, NULL},
    {OVERLAY_ROW_FRAME_TIME, {0, 200, 255, 255}, 0, NULL},
    {OVERLAY_ROW_DRAW_CALLS, {0, 255, 160, 255}, 0, NULL},
    {OVERLAY_ROW_TIMING_OVERHEAD, {0, 200, 255, 255}, 0, NULL},
    {OVERLAY_ROW_CUSTOM, {255, 180, 120, 255}, 0, "%s"},
};

static const OverlayKeybind g_scaling_keybinds[] = {
    {"SELECT", "Toggle overlay"},
    {"MENU", "Reset metrics"},
    {"B", "Adjust stress level"},
    {"X", "Lock scaling mode"},
    {"Y", "Lock content mode"},
    {"L1", "Toggle NEON"},
};

/* SCALING_BENCH_FORCE_MODE=<name> pins the scaling mode and disables
 * auto-cycle; the two arbitrary_* names additionally select whether the
 * bilinear NEON hint is set before SDL_CreateRenderer. */
static SDL_bool scaling_force_mode_from_name(const char *name, ScalingMode *out_mode,
                                             SDL_bool *out_bilinear)
{
    static const struct { const char *name; ScalingMode mode; SDL_bool bilinear; } table[] = {
        {"arbitrary_hw", SCALING_MODE_ARBITRARY_RATIO, SDL_FALSE},
        {"arbitrary_bilinear", SCALING_MODE_ARBITRARY_RATIO, SDL_TRUE},
        {"downscale_composite", SCALING_MODE_DOWNSCALE_COMPOSITE, SDL_FALSE},
    };
    for (size_t i = 0; i < SDL_arraysize(table); i++) {
        if (SDL_strcasecmp(name, table[i].name) == 0) {
            *out_mode = table[i].mode;
            *out_bilinear = table[i].bilinear;
            return SDL_TRUE;
        }
    }
    return SDL_FALSE;
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    const Uint64 perf_freq = SDL_GetPerformanceFrequency();
    Uint64 last_counter = SDL_GetPerformanceCounter();

    ScalingMode forced_mode = SCALING_MODE_MAX;
    SDL_bool forced_mode_valid = SDL_FALSE;
    const char *force_mode_name = SDL_getenv("SCALING_BENCH_FORCE_MODE");
    if (force_mode_name) {
        SDL_bool want_bilinear = SDL_FALSE;
        forced_mode_valid = scaling_force_mode_from_name(force_mode_name, &forced_mode, &want_bilinear);
        if (forced_mode_valid && want_bilinear) {
            SDL_setenv("SDL_MMIYOO_SCALE_FILTER", "bilinear", 1);
        }
    }

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() < 0) {
        printf("TTF_Init failed: %s\n", TTF_GetError());
    }

    SDL_Window *window = SDL_CreateWindow("SDL2 Resolution Scaling Bench",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          BENCH_NATIVE_W, BENCH_NATIVE_H,
                                          SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    /* SDL_CreateRenderer force-ORs in SDL_RENDERER_PRESENTVSYNC in this SDL2
     * fork regardless of flags -- the hint is the only way to turn it off. */
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "0");
#ifdef DEBUG_BUILD
    overlay_debug_stats_enable_hints();
#endif
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
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
                                                  BENCH_LOADING_STYLE_RECT);
    if (loading_active) {
        bench_loading_step(&loading, 0.2f, "Initialising state");
    }

    ScalingBenchState state;
    scaling_state_init(&state);
    if (forced_mode_valid) {
        state.forced_scaling_mode = (int)forced_mode;
    }

    BenchProfileCapture profile;
    bench_profile_load(&profile);

    if (loading_active) {
        bench_loading_step(&loading, 0.4f, "Loading fonts");
    }
    state.font = bench_load_font(16);

    if (loading_active) {
        bench_loading_step(&loading, 0.6f, "Preparing render targets");
    }
    scaling_render_init(&state, renderer);

    BenchMetrics metrics;
    bench_reset_metrics(&metrics);

    BenchOverlay *overlay = bench_overlay_create(renderer, bench_logical_w(), 16, 12);
    if (!overlay) {
        printf("Overlay creation failed\n");
        if (loading_active) {
            bench_loading_abort(&loading);
        }
        scaling_render_cleanup(&state);
        scaling_state_destroy(&state, renderer);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    bench_overlay_configure(overlay, g_scaling_rows, (int)SDL_arraysize(g_scaling_rows),
                            g_scaling_keybinds, (int)SDL_arraysize(g_scaling_keybinds));
    if (loading_active) {
        bench_loading_step(&loading, 0.85f, "Resolution scaling bench ready");
        bench_loading_finish(&loading);
        loading_active = SDL_FALSE;
    }

    printf("SDL2 Resolution Scaling Bench initialised\n");

    SDL_bool running = SDL_TRUE;
    while (running) {
        const Uint64 frame_start_counter = SDL_GetPerformanceCounter();

        if (!scaling_handle_input(&state, &metrics, overlay)) {
            break;
        }

        const double delta_seconds = bench_get_delta_seconds(&last_counter, perf_freq);

        metrics.draw_calls = 0;
        metrics.vertices_rendered = 0;
        metrics.triangles_rendered = 0;
        metrics.texture_switches = 0;
        metrics.scaling_operations = 0;
        metrics.scaling_overhead_ms = 0.0;

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer, 12, 16, 28, 255);
        SDL_RenderClear(renderer);
        metrics.draw_calls++;

        scaling_render_scene(&state, renderer, &metrics, delta_seconds);

        bench_overlay_present(overlay, renderer, &metrics, 0, 0);
        SDL_RenderPresent(renderer);

        bench_update_metrics(&metrics, delta_seconds * 1000.0);
        bench_frame_limit_wait(frame_start_counter);

        char stress_label[96];
        snprintf(stress_label, sizeof(stress_label), "Stress L%d x%.1f | Res %dx%d",
                 state.stress_level, scaling_state_stress_factor(&state),
                 state.scaling_current_width, state.scaling_current_height);
        char mode_label[96];
        snprintf(mode_label, sizeof(mode_label), "%s%s | %s%s | %s",
                 scaling_render_mode_name(state.scaling_mode),
                 state.forced_scaling_mode >= 0 ? "*" : "",
                 scaling_render_content_name(state.content_mode),
                 state.forced_content_mode >= 0 ? "*" : "",
                 state.neon_enabled ? "NEON" : "Scalar");
        const char *custom_values[] = {stress_label, mode_label};
        bench_overlay_update(overlay, &metrics, custom_values, (int)SDL_arraysize(custom_values));

        if (bench_profile_update(&profile, &metrics, renderer)) {
            running = SDL_FALSE;
        }
    }

    bench_profile_shutdown(&profile);
    bench_driver_shutdown();
    scaling_render_cleanup(&state);
    scaling_state_destroy(&state, renderer);
    bench_overlay_destroy(overlay);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
