#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "bench_common.h"
#include "render_suite_gl/input.h"
#include "render_suite_gl/overlay.h"
#include "render_suite_gl/scenes/effects.h"
#include "render_suite_gl/state.h"
#include "common/loading_screen.h"

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

    SDL_setenv("SDL_MMIYOO_DOUBLE_BUFFER", "1", 1);

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
                                          BENCH_SCREEN_W,
                                          BENCH_SCREEN_H,
                                          SDL_WINDOW_SHOWN);
    if (!window) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    BenchLoadingScreen loading;
    SDL_bool loading_active = bench_loading_begin(&loading,
                                                  window,
                                                  renderer,
                                                  BENCH_LOADING_STYLE_GL);
    SDL_Window *loader_gl_window = NULL;
    SDL_GLContext loader_gl_context = NULL;
    if (loading_active) {
        bench_loading_step(&loading, 0.1f, "Preparing state objects");
    }

    rsgl_print_info();

    RsglState state;
    rsgl_state_init(&state);
    if (loading_active) {
        bench_loading_step(&loading, 0.2f, "Loading fonts");
    }
    state.font = bench_load_font(16);

    BenchMetrics metrics;
    bench_reset_metrics(&metrics);
    if (loading_active) {
        bench_loading_step(&loading, 0.3f, "Allocating overlay");
    }

    BenchOverlay *overlay = bench_overlay_create(renderer, BENCH_SCREEN_W, 16, 12);
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

    rsgl_state_update_layout(&state, overlay);
    if (loading_active) {
        bench_loading_step(&loading, 0.45f, "Preparing GL context");
        if (bench_loading_obtain_gl(&loading, &loader_gl_window, &loader_gl_context)) {
            state.gl_window = loader_gl_window;
            state.gl_context = loader_gl_context;
            state.gl_external = SDL_TRUE;
            state.gl_library_loaded = SDL_TRUE;
        }
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
    rsgl_overlay_submit(overlay, &state, &metrics);

    if (loading_active) {
        bench_loading_step(&loading, 1.0f, "GL suite ready");
        bench_loading_finish(&loading);
        loading_active = SDL_FALSE;
    }

    Uint64 counter = SDL_GetPerformanceCounter();
    const Uint64 freq = SDL_GetPerformanceFrequency();

    SDL_bool running = SDL_TRUE;
    while (running) {
        if (!rsgl_handle_input(&state, &metrics)) {
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

        rsgl_state_update_layout(&state, overlay);

        SDL_SetRenderDrawColor(renderer, 8, 10, 18, 255);
        SDL_RenderClear(renderer);

        rsgl_effects_render(&state, renderer, &metrics, delta);

        bench_overlay_present(overlay, renderer, &metrics, 0, 0);
        SDL_RenderPresent(renderer);

        bench_update_metrics(&metrics, delta * 1000.0);
        state.running = running;
        rsgl_overlay_submit(overlay, &state, &metrics);
    }

    rsgl_effects_cleanup(&state);
    rsgl_state_destroy(&state);
    bench_overlay_destroy(overlay);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
