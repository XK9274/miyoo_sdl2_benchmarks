#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <stdio.h>

#include "software_buf/input.h"
#include "software_buf/overlay.h"
#include "software_buf/particles.h"
#include "software_buf/render.h"
#include "software_buf/state.h"
#include "common/loading_screen.h"

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    if (SDL_getenv("SDL_MMIYOO_DOUBLE_BUFFER") == NULL) {
        SDL_setenv("SDL_MMIYOO_DOUBLE_BUFFER", "0", 1);
    }

    Uint64 perf_freq = SDL_GetPerformanceFrequency();
    Uint64 last_counter = SDL_GetPerformanceCounter();

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() < 0) {
        printf("TTF_Init failed: %s\n", TTF_GetError());
    }

    SDL_Window *window = SDL_CreateWindow("SDL2 Software Double Buffer Bench",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          SB_SCREEN_W, SB_SCREEN_H,
                                          SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
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
        bench_loading_step(&loading, 0.15f, "Initialising renderer");
    }

    SDL_Texture *backbuffer = SDL_CreateTexture(renderer,
                                                SDL_PIXELFORMAT_RGBA8888,
                                                SDL_TEXTUREACCESS_TARGET,
                                                SB_SCREEN_W,
                                                SB_SCREEN_H);
    if (!backbuffer) {
        printf("Backbuffer creation failed: %s\n", SDL_GetError());
        if (loading_active) {
            bench_loading_abort(&loading);
        }
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_SetTextureBlendMode(backbuffer, SDL_BLENDMODE_NONE);

    BenchOverlay *overlay = bench_overlay_create(renderer, SB_SCREEN_W, 16, 12);
    if (!overlay) {
        if (loading_active) {
            bench_loading_abort(&loading);
        }
        SDL_DestroyTexture(backbuffer);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SoftwareBenchState state;
    sb_state_init(&state);
    if (loading_active) {
        bench_loading_step(&loading, 0.35f, "Preparing state");
    }

    BenchMetrics metrics;
    bench_reset_metrics(&metrics);
    sb_overlay_submit(overlay, &state, &metrics);
    if (loading_active) {
        bench_loading_mark_idle(&loading, "GL modules idle - software path");
        bench_loading_finish(&loading);
        loading_active = SDL_FALSE;
    }

    printf("SDL2 software double buffer benchmark started\n");

    SDL_bool running = SDL_TRUE;
    while (running) {
        running = sb_handle_input(&state, &metrics);
        if (!running) {
            break;
        }

        const double delta_seconds = bench_get_delta_seconds(&last_counter, perf_freq);

        metrics.draw_calls = 0;
        metrics.vertices_rendered = 0;
        metrics.triangles_rendered = 0;

        sb_state_update_layout(&state, bench_overlay_height(overlay));
        sb_particles_update(&state, delta_seconds);
        state.cube_rotation += (float)(delta_seconds * 1.6f);

        sb_render_scene(renderer, backbuffer, &state, &metrics);
        bench_overlay_present(overlay, renderer, &metrics, 0, 0);

        SDL_SetRenderTarget(renderer, NULL);
        SDL_RenderCopy(renderer, backbuffer, NULL, NULL);
        SDL_RenderPresent(renderer);

        bench_update_metrics(&metrics, delta_seconds * 1000.0);
        sb_overlay_submit(overlay, &state, &metrics);
    }

    bench_overlay_destroy(overlay);
    SDL_DestroyTexture(backbuffer);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
