#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <stdio.h>

#include "space_bench/input.h"
#include "space_bench/overlay.h"
#include "space_bench/render.h"
#include "space_bench/state.h"
#include "common/loading_screen.h"

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    SDL_setenv("SDL_MMIYOO_DOUBLE_BUFFER", "1", 1);

    Uint64 perf_freq = SDL_GetPerformanceFrequency();
    Uint64 last_counter = SDL_GetPerformanceCounter();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() < 0) {
        printf("TTF_Init failed: %s\n", TTF_GetError());
    }

    SDL_Window *window = SDL_CreateWindow("SDL2 Star Wing Bench",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          SPACE_SCREEN_W,
                                          SPACE_SCREEN_H,
                                          SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    bench_driver_init(window);

    BenchLoadingScreen loading;
    SDL_bool loading_active = bench_loading_begin(&loading,
                                                  window,
                                                  renderer,
                                                  BENCH_LOADING_STYLE_RECT);
    if (loading_active) {
        bench_loading_step(&loading, 0.15f, "Preparing state");
    }

    SpaceBenchState state;
    space_state_init(&state);
    space_state_update_layout(&state, SPACE_HUD_STRIP_HEIGHT);

    BenchMetrics metrics;
    bench_reset_metrics(&metrics);
    if (loading_active) {
        bench_loading_mark_idle(&loading, "GL modules idle - starfield");
        bench_loading_finish(&loading);
        loading_active = SDL_FALSE;
    }

    printf("SDL2 star wing bench started\n");

    SDL_bool running = SDL_TRUE;
    while (running) {
        running = space_handle_input(&state, &metrics);
        if (!running) {
            break;
        }

        Uint64 current_counter = SDL_GetPerformanceCounter();
        const double delta_seconds = (double)(current_counter - last_counter) / (double)perf_freq;
        last_counter = current_counter;

        metrics.draw_calls = 0;
        metrics.vertices_rendered = 0;
        metrics.triangles_rendered = 0;

        space_state_update(&state, (float)delta_seconds);

        space_render_scene(&state, renderer, &metrics);
        space_hud_render(renderer, &state, &metrics);
        SDL_RenderPresent(renderer);

        bench_update_metrics(&metrics, delta_seconds * 1000.0);
    }

    bench_driver_shutdown();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
