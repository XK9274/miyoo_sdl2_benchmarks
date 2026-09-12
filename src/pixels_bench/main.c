#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "bench_common.h"
#include "pixels_bench/input.h"
#include "pixels_bench/render.h"
#include "pixels_bench/state.h"
#include "common/hotkeys.h"
#include "common/loading_screen.h"
#include "common/overlay_rows.h"

static const OverlayRowSpec g_pixels_rows[] = {
    {OVERLAY_ROW_CUSTOM, {255, 180, 120, 255}, 0, "%s"},
    {OVERLAY_ROW_FPS, {255, 255, 255, 255}, 0, NULL},
    {OVERLAY_ROW_FRAME_TIME, {0, 200, 255, 255}, 0, NULL},
    {OVERLAY_ROW_DRAW_CALLS, {0, 255, 160, 255}, 0, NULL},
    {OVERLAY_ROW_TEXTURE_SWITCHES, {0, 255, 160, 255}, 0, NULL},
    {OVERLAY_ROW_RESOURCE_OPS, {0, 200, 255, 255}, 0, NULL},
    {OVERLAY_ROW_CUSTOM, {255, 180, 120, 255}, 0, "%s"},
};

static const OverlayKeybind g_pixels_keybinds[] = {
    {"SELECT", "Toggle overlay"},
    {"MENU", "Reset metrics"},
    {"B", "Adjust stress level"},
    {"X", "Lock mode"},
    {"Y", "Toggle NEON copy"},
    {"L1", "Cycle blend mode"},
    {"R1", "Toggle upload path"},
};

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    const Uint64 perf_freq = SDL_GetPerformanceFrequency();
    Uint64 last_counter = SDL_GetPerformanceCounter();

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() < 0) {
        printf("TTF_Init failed: %s\n", TTF_GetError());
    }

    SDL_Window *window = SDL_CreateWindow("SDL2 Pixel Operations Bench",
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

    PixelsBenchState state;
    pixels_state_init(&state);

    const char *bench_duration_str = SDL_getenv("PIXELS_BENCH_DURATION_S");
    const double bench_duration_s = bench_duration_str ? SDL_atof(bench_duration_str) : 0.0;
    const char *bench_tag = SDL_getenv("PIXELS_BENCH_TAG");

    if (loading_active) {
        bench_loading_step(&loading, 0.4f, "Loading fonts");
    }
    state.font = bench_load_font(16);

    pixels_render_init(&state, renderer);
    srand((unsigned int)time(NULL));

    BenchMetrics metrics;
    bench_reset_metrics(&metrics);

    BenchOverlay *overlay = bench_overlay_create(renderer, bench_logical_w(), 16, 12);
    if (!overlay) {
        printf("Overlay creation failed\n");
        if (loading_active) {
            bench_loading_abort(&loading);
        }
        pixels_render_cleanup(&state);
        pixels_state_destroy(&state, renderer);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    bench_overlay_configure(overlay, g_pixels_rows, (int)SDL_arraysize(g_pixels_rows),
                            g_pixels_keybinds, (int)SDL_arraysize(g_pixels_keybinds));
    if (loading_active) {
        bench_loading_step(&loading, 0.8f, "Pixel operations bench ready");
        bench_loading_finish(&loading);
        loading_active = SDL_FALSE;
    }

    printf("SDL2 Pixel Operations Bench initialised\n");

    double next_bench_log_ms = 0.0;

    SDL_bool running = SDL_TRUE;
    while (running) {
        const Uint64 frame_start_counter = SDL_GetPerformanceCounter();

        if (!pixels_handle_input(&state, &metrics, overlay)) {
            break;
        }

        const double delta_seconds = bench_get_delta_seconds(&last_counter, perf_freq);

        metrics.draw_calls = 0;
        metrics.vertices_rendered = 0;
        metrics.triangles_rendered = 0;
        metrics.texture_switches = 0;

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer, 12, 16, 28, 255);
        SDL_RenderClear(renderer);
        metrics.draw_calls++;

        pixels_render_scene(&state, renderer, &metrics, delta_seconds);

        bench_overlay_present(overlay, renderer, &metrics, 0, 0);
        SDL_RenderPresent(renderer);

        bench_update_metrics(&metrics, delta_seconds * 1000.0);
        bench_frame_limit_wait(frame_start_counter);

        char stress_label[48];
        snprintf(stress_label, sizeof(stress_label), "Stress L%d x%.1f",
                 state.stress_level, pixels_state_stress_factor(&state));
        char mode_label[80];
        snprintf(mode_label, sizeof(mode_label), "%s%s %dx%d | %s | %s",
                 pixels_render_mode_name(state.current_mode),
                 state.forced_mode >= 0 ? "*" : "",
                 state.buffer_width, state.buffer_height,
                 pixels_render_blend_name(state.blend_mode),
                 pixels_render_upload_name(state.upload_path));
        const char *custom_values[] = {stress_label, mode_label};
        bench_overlay_update(overlay, &metrics, custom_values, (int)SDL_arraysize(custom_values));

        if (bench_tag && metrics.accumulated_frame_time_ms >= next_bench_log_ms) {
            printf("[BENCH] tag=%s elapsed_s=%.1f frame=%llu fps=%.2f avg_fps=%.2f "
                   "min_fps=%.2f max_fps=%.2f frame_ms=%.3f lock_ms=%.3f pixel_ops=%llu\n",
                   bench_tag, metrics.accumulated_frame_time_ms / 1000.0,
                   (unsigned long long)metrics.frame_count, metrics.current_fps, metrics.avg_fps,
                   metrics.min_fps, metrics.max_fps, metrics.frame_time_ms,
                   metrics.lock_unlock_overhead_ms,
                   (unsigned long long)metrics.pixel_operations);
            fflush(stdout);
            next_bench_log_ms = metrics.accumulated_frame_time_ms + 2000.0;
        }

        if (bench_duration_s > 0.0 && metrics.accumulated_frame_time_ms >= bench_duration_s * 1000.0) {
            running = SDL_FALSE;
        }
    }

    bench_driver_shutdown();
    pixels_render_cleanup(&state);
    pixels_state_destroy(&state, renderer);
    bench_overlay_destroy(overlay);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
