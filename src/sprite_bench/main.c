#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "bench_common.h"
#include "sprite_bench/input.h"
#include "sprite_bench/state.h"

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

    SDL_Window *window = SDL_CreateWindow("SDL2 Sprite Bench",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          BENCH_NATIVE_W, BENCH_NATIVE_H,
                                          SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    /* This SDL2 fork forces renderer vsync unless the hint disables it. */
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "0");
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
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
    srand((unsigned int)time(NULL));

    SpriteBenchState state;
    if (!sprite_state_init(&state, renderer)) {
        printf("sprite_state_init failed\n");
        bench_driver_shutdown();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    BenchMetrics metrics;
    bench_reset_metrics(&metrics);

    printf("SDL2 Sprite Bench initialised (no overlay, no vsync)\n");

    SDL_bool running = SDL_TRUE;
    while (running) {
        const Uint64 frame_start_counter = SDL_GetPerformanceCounter();

        if (!sprite_handle_input(&state, &metrics)) {
            break;
        }

        const double delta_seconds = bench_get_delta_seconds(&last_counter, perf_freq);

        metrics.draw_calls = 0;
        metrics.vertices_rendered = 0;
        metrics.triangles_rendered = 0;

        sprite_state_update_ramp(&state, delta_seconds);
        sprite_state_update_pool(&state, (float)delta_seconds);
        sprite_state_update_instances(&state, (float)delta_seconds);

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer, 6, 6, 10, 255);
        SDL_RenderClear(renderer);
        metrics.draw_calls++;

        sprite_state_render(&state, renderer, &metrics);
        sprite_state_render_status_bg(&state, renderer, &metrics);

        bench_update_metrics(&metrics, delta_seconds * 1000.0);
        sprite_state_render_text(&state, renderer, &metrics, delta_seconds);

        SDL_RenderPresent(renderer);
        bench_frame_limit_wait(frame_start_counter);
    }

    bench_driver_shutdown();
    sprite_state_destroy(&state);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
