#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <stdio.h>

#include "bench_common.h"
#include "gfx_bench/input.h"
#include "gfx_bench/scenes/aa_shapes.h"
#include "gfx_bench/scenes/rounded_rects.h"
#include "gfx_bench/scenes/polygons.h"
#include "gfx_bench/scenes/bezier.h"
#include "gfx_bench/scenes/thick_lines.h"
#include "gfx_bench/state.h"
#include "common/hotkeys.h"
#include "common/loading_screen.h"
#include "common/overlay_rows.h"
#ifdef DEBUG_BUILD
#include "common/overlay_debug_stats.h"
#endif

static const char *g_gb_scene_names[GB_SCENE_MAX] = {
    "AA Shapes", "Rounded Rects", "Polygons", "Bezier Curves", "Thick Lines",
};

static const OverlayRowSpec g_gb_rows[] = {
    {OVERLAY_ROW_CUSTOM, {240, 194, 94, 255}, 0, "%s"},
    {OVERLAY_ROW_CUSTOM, {240, 194, 94, 255}, 0, "%s"},
    {OVERLAY_ROW_FPS, {255, 255, 255, 255}, 0, NULL},
    {OVERLAY_ROW_FRAME_TIME, {255, 255, 255, 255}, 0, NULL},
    {OVERLAY_ROW_DRAW_CALLS, {0, 255, 160, 255}, 0, NULL},
    {OVERLAY_ROW_VERTICES, {0, 255, 160, 255}, 0, NULL},
    {OVERLAY_ROW_TRIANGLES, {0, 255, 160, 255}, 0, NULL},
    {OVERLAY_ROW_CPU_PERCENT, {0, 200, 255, 255}, 0, NULL},
    {OVERLAY_ROW_RAM_USAGE, {0, 200, 255, 255}, 0, NULL},
#ifdef DEBUG_BUILD
    {OVERLAY_ROW_CMDQUEUE_TIME, {255, 155, 106, 255}, 0, NULL},
    {OVERLAY_ROW_PRESENT_TIME, {255, 155, 106, 255}, 0, NULL},
#endif
};

static const OverlayKeybind g_gb_keybinds[] = {
    {"SELECT", "Toggle overlay"},
    {"MENU", "Reset metrics"},
    {"L2/R2", "Switch scene"},
    {"A", "Toggle auto cycle"},
    {"B", "Adjust stress level"},
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

    SDL_Window *window = SDL_CreateWindow("SDL2_gfx Bench",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          BENCH_NATIVE_W, BENCH_NATIVE_H,
                                          SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    /* Hint is the only way to disable vsync -- this SDL2 fork forces it on regardless of flags. */
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

    GfxBenchState state;
    gb_state_init(&state);

    BenchMetrics metrics;
    bench_reset_metrics(&metrics);

    BenchOverlay *overlay = bench_overlay_create(renderer, bench_logical_w(), 16, 12);
    if (!overlay) {
        printf("Overlay creation failed\n");
        if (loading_active) {
            bench_loading_abort(&loading);
        }
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    bench_overlay_configure(overlay, g_gb_rows, (int)SDL_arraysize(g_gb_rows),
                            g_gb_keybinds, (int)SDL_arraysize(g_gb_keybinds));
    if (loading_active) {
        bench_loading_step(&loading, 0.8f, "SDL2_gfx bench ready");
        bench_loading_finish(&loading);
        loading_active = SDL_FALSE;
    }

    printf("SDL2_gfx bench initialised\n");

    SDL_bool running = SDL_TRUE;
    while (running) {
        const Uint64 frame_start_counter = SDL_GetPerformanceCounter();

        if (!gb_handle_input(&state, &metrics, overlay)) {
            break;
        }

        const double delta_seconds = bench_get_delta_seconds(&last_counter, perf_freq);

        metrics.draw_calls = 0;
        metrics.vertices_rendered = 0;
        metrics.triangles_rendered = 0;

        state.phase += (float)delta_seconds;

        if (state.auto_cycle) {
            const double elapsed_seconds = metrics.accumulated_frame_time_ms / 1000.0;
            const int cycle = ((int)(elapsed_seconds / 5.0)) % GB_SCENE_MAX;
            state.active_scene = (GfxBenchSceneKind)cycle;
        }

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer, 12, 16, 28, 255);
        SDL_RenderClear(renderer);
        metrics.draw_calls++;

        switch (state.active_scene) {
            case GB_SCENE_AA_SHAPES:
                gb_scene_aa_shapes(&state, renderer, &metrics, delta_seconds);
                break;
            case GB_SCENE_ROUNDED_RECTS:
                gb_scene_rounded_rects(&state, renderer, &metrics, delta_seconds);
                break;
            case GB_SCENE_POLYGONS:
                gb_scene_polygons(&state, renderer, &metrics, delta_seconds);
                break;
            case GB_SCENE_BEZIER:
                gb_scene_bezier(&state, renderer, &metrics, delta_seconds);
                break;
            case GB_SCENE_THICK_LINES:
                gb_scene_thick_lines(&state, renderer, &metrics, delta_seconds);
                break;
            default:
                break;
        }

        bench_overlay_present(overlay, renderer, &metrics, 0, 0);
        SDL_RenderPresent(renderer);

        bench_update_metrics(&metrics, delta_seconds * 1000.0);
        bench_frame_limit_wait(frame_start_counter);

        char scene_label[64];
        snprintf(scene_label, sizeof(scene_label), "%s | Auto %s",
                 g_gb_scene_names[state.active_scene], state.auto_cycle ? "ON" : "OFF");
        char stress_label[48];
        snprintf(stress_label, sizeof(stress_label), "Stress L%d x%.1f",
                 state.stress_level, gb_state_stress_factor(&state));
        const char *custom_values[] = {scene_label, stress_label};
        bench_overlay_update(overlay, &metrics, custom_values, (int)SDL_arraysize(custom_values));
    }

    bench_driver_shutdown();
    bench_overlay_destroy(overlay);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
