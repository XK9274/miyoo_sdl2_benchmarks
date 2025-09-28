#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "bench_common.h"
#include "render_suite/input.h"
#include "render_suite/overlay.h"
#include "render_suite/resources.h"
#include "render_suite/scenes/fill.h"
#include "render_suite/scenes/lines.h"
#include "render_suite/scenes/texture.h"
#include "render_suite/scenes/geometry.h"
#include "render_suite/scenes/scaling.h"
#include "render_suite/scenes/memory.h"
#include "render_suite/scenes/pixels.h"
#include "render_suite/state.h"
#include "common/loading_screen.h"

static void rs_print_system_info(void)
{
    SDL_version version;
    SDL_GetVersion(&version);

    printf("=== System Information ===\n");
    printf("SDL Version: %d.%d.%d\n", version.major, version.minor, version.patch);
    printf("SDL Revision: %s\n", SDL_GetRevision());
    printf("Platform: %s\n", SDL_GetPlatform());
    printf("CPU Count: %d\n", SDL_GetCPUCount());
    printf("RAM: %d MB\n", SDL_GetSystemRAM());
    printf("=========================\n\n");
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    SDL_setenv("SDL_MMIYOO_DOUBLE_BUFFER", "1", 1);

    const Uint64 perf_freq = SDL_GetPerformanceFrequency();
    Uint64 last_counter = SDL_GetPerformanceCounter();

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() < 0) {
        printf("TTF_Init failed: %s\n", TTF_GetError());
    }

    SDL_Window *window = SDL_CreateWindow("SDL2 Render Suite",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          BENCH_SCREEN_W, BENCH_SCREEN_H,
                                          SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    BenchLoadingScreen loading;
    SDL_bool loading_active = bench_loading_begin(&loading,
                                                  window,
                                                  renderer,
                                                  BENCH_LOADING_STYLE_RECT);
    if (loading_active) {
        bench_loading_step(&loading, 0.1f, "Initialising state");
    }

    rs_print_system_info();

    RenderSuiteState state;
    rs_state_init(&state);
    if (loading_active) {
        bench_loading_step(&loading, 0.2f, "Loading fonts");
    }

    state.font = bench_load_font(16);
    state.checker_texture = rs_create_checker_texture(renderer, 192, 192);
    if (loading_active) {
        bench_loading_step(&loading, 0.35f, "Preparing scenes");
    }

    // Initialize new benchmark scenes
    rs_scene_scaling_init(&state, renderer);
    rs_scene_memory_init(&state, renderer);
    rs_scene_pixels_init(&state, renderer);

    srand((unsigned int)time(NULL));

    BenchMetrics metrics;
    bench_reset_metrics(&metrics);

    BenchOverlay *overlay = bench_overlay_create(renderer, BENCH_SCREEN_W, 16, 12);
    if (!overlay) {
        printf("Overlay creation failed\n");
        if (loading_active) {
            bench_loading_abort(&loading);
        }
        rs_state_destroy(&state, renderer);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    rs_state_update_layout(&state, overlay);
    rs_overlay_submit(overlay, &state, &metrics);
    if (loading_active) {
        bench_loading_step(&loading, 0.8f, "Render suite ready");
        bench_loading_finish(&loading);
        loading_active = SDL_FALSE;
    }

    printf("SDL2 Render Suite initialised\n");

    SDL_bool running = SDL_TRUE;
    while (running) {
        if (!rs_handle_input(&state, &metrics)) {
            break;
        }

        const double delta_seconds = bench_get_delta_seconds(&last_counter, perf_freq);

        metrics.draw_calls = 0;
        metrics.vertices_rendered = 0;
        metrics.triangles_rendered = 0;

        rs_state_update_layout(&state, overlay);

        if (state.auto_cycle) {
            const double elapsed_seconds = metrics.accumulated_frame_time_ms / 1000.0;
            const int cycle = ((int)(elapsed_seconds / 5.0)) % SCENE_MAX;
            state.active_scene = (SceneKind)cycle;
        }

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer, 12, 16, 28, 255);
        SDL_RenderClear(renderer);
        metrics.draw_calls++;

        const double time_seconds = metrics.accumulated_frame_time_ms / 1000.0;

        switch (state.active_scene) {
            case SCENE_FILL:
                rs_scene_fill(&state, renderer, &metrics, delta_seconds);
                break;
            case SCENE_TEXTURE:
                rs_scene_texture(&state, renderer, &metrics, delta_seconds);
                break;
            case SCENE_LINES:
                rs_scene_lines(&state, renderer, &metrics, time_seconds);
                break;
            case SCENE_GEOMETRY:
                rs_scene_geometry(&state, renderer, &metrics, delta_seconds);
                break;
            case SCENE_SCALING:
                rs_scene_scaling(&state, renderer, &metrics, delta_seconds);
                break;
            case SCENE_MEMORY:
                rs_scene_memory(&state, renderer, &metrics, delta_seconds);
                break;
            case SCENE_PIXELS:
                rs_scene_pixels(&state, renderer, &metrics, delta_seconds);
                break;
            default:
                break;
        }

        bench_overlay_present(overlay, renderer, &metrics, 0, 0);
        SDL_RenderPresent(renderer);

        bench_update_metrics(&metrics, delta_seconds * 1000.0);
        rs_overlay_submit(overlay, &state, &metrics);
    }

    // Cleanup new benchmark scenes
    rs_scene_scaling_cleanup(&state);
    rs_scene_memory_cleanup(&state);
    rs_scene_pixels_cleanup(&state);

    rs_state_destroy(&state, renderer);
    bench_overlay_destroy(overlay);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
