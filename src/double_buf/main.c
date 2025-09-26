#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <stdio.h>

#include "double_buf/input.h"
#include "double_buf/overlay.h"
#include "double_buf/particles.h"
#include "double_buf/render.h"
#include "double_buf/state.h"

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    SDL_setenv("SDL_MMIYOO_DOUBLE_BUFFER", "1", 1);

    Uint64 perf_freq = SDL_GetPerformanceFrequency();
    Uint64 last_counter = SDL_GetPerformanceCounter();

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() < 0) {
        printf("TTF_Init failed: %s\n", TTF_GetError());
    }

    SDL_Window *window = SDL_CreateWindow("SDL2 Hardware Double Buffer Bench",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          DB_SCREEN_W, DB_SCREEN_H,
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

    BenchOverlay *overlay = bench_overlay_create(renderer, DB_SCREEN_W, 16, 12);
    if (!overlay) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    DoubleBenchState state;
    db_state_init(&state);

    BenchMetrics metrics;
    bench_reset_metrics(&metrics);
    db_overlay_submit(overlay, &state, &metrics);

    printf("SDL2 hardware double buffer benchmark started\n");

    SDL_bool running = SDL_TRUE;
    while (running) {
        running = db_handle_input(&state, &metrics);
        if (!running) {
            break;
        }

        const double delta_seconds = bench_get_delta_seconds(&last_counter, perf_freq);

        metrics.draw_calls = 0;
        metrics.vertices_rendered = 0;
        metrics.triangles_rendered = 0;

        db_state_update_layout(&state, bench_overlay_height(overlay));
        db_particles_update(&state, delta_seconds);
        state.cube_rotation += (float)(delta_seconds * 1.8f);

        db_render_backdrop(&state, renderer, &metrics);
        db_render_cube_and_particles(&state, renderer, &metrics);

        bench_overlay_present(overlay, renderer, &metrics, 0, 0);
        SDL_RenderPresent(renderer);

        bench_update_metrics(&metrics, delta_seconds * 1000.0);
        db_overlay_submit(overlay, &state, &metrics);
    }

    bench_overlay_destroy(overlay);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
