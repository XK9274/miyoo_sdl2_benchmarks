#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <stdio.h>

#include "audio_bench/audio_device.h"
#include "audio_bench/input.h"
#include "audio_bench/overlay.h"
#include "audio_bench/waveform.h"
#include "bench_common.h"
#include "common/loading_screen.h"
#include "controller_input.h"

#define SCREEN_W BENCH_SCREEN_W
#define SCREEN_H BENCH_SCREEN_H

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    if (SDL_getenv("SDL_AUDIODRIVER") == NULL) {
        SDL_setenv("SDL_AUDIODRIVER", "mmiyoo", 0);
    }

    SDL_setenv("SDL_MMIYOO_DOUBLE_BUFFER", "1", 1);

    Uint64 perf_freq = SDL_GetPerformanceFrequency();
    Uint64 last_counter = SDL_GetPerformanceCounter();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() < 0) {
        printf("TTF_Init failed: %s\n", TTF_GetError());
    }

    SDL_Window *window = SDL_CreateWindow("SDL2 Audio Bench",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          SCREEN_W, SCREEN_H,
                                          SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    BenchLoadingScreen loading;
    SDL_bool loading_active = bench_loading_begin(&loading,
                                                  window,
                                                  renderer,
                                                  BENCH_LOADING_STYLE_RECT);
    if (loading_active) {
        bench_loading_step(&loading, 0.1f, "Setting up overlay");
    }

    BenchOverlay *overlay = bench_overlay_create(renderer, SCREEN_W, 16, 12);
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

    if (loading_active) {
        bench_loading_step(&loading, 0.25f, "Initialising audio device");
    }
    if (!audio_device_init()) {
        if (loading_active) {
            bench_loading_abort(&loading);
        }
        bench_overlay_destroy(overlay);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    BenchMetrics metrics;
    bench_reset_metrics(&metrics);

    audio_overlay_start(overlay, &metrics);

    audio_device_play();
    if (loading_active) {
        bench_loading_step(&loading, 1.0f, "Audio bench ready");
        bench_loading_mark_idle(&loading, "GL modules idle - audio focus");
        bench_loading_finish(&loading);
        loading_active = SDL_FALSE;
    }
    printf("SDL2 audio bench started\n");

    SDL_bool running = SDL_TRUE;
    while (running) {
        running = audio_handle_input(&metrics);
        if (!running) {
            break;
        }

        const double delta_seconds = bench_get_delta_seconds(&last_counter, perf_freq);

        // Reduce render frequency when paused
        static Uint32 last_render = 0;
        Uint32 now = SDL_GetTicks();
        const SDL_bool is_playing = audio_device_is_playing();
        const Uint32 render_interval = is_playing ? 16 : 33; // 60fps vs 30fps

        if (now - last_render < render_interval) {
            SDL_Delay(1); // Small delay to prevent 100% CPU
            continue;
        }
        last_render = now;

        metrics.draw_calls = 0;
        metrics.vertices_rendered = 0;
        metrics.triangles_rendered = 0;

        const int overlay_height = bench_overlay_height(overlay);
        const int margin = 12;
        const int available_height = SCREEN_H - overlay_height - margin * 2;

        // Split available space: 30% for new UI area, 70% for waveform
        const int ui_area_height = (int)(available_height * 0.3f);
        const int waveform_height = available_height - ui_area_height - margin;
        const int ui_area_y = overlay_height + margin;
        const int waveform_y = ui_area_y + ui_area_height + margin;

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer, 8, 10, 18, 255);
        SDL_RenderClear(renderer);
        metrics.draw_calls++;

        // Draw new UI area
        if (ui_area_height > 20) {
            waveform_draw_ui_area(renderer,
                                  &metrics,
                                  margin,
                                  ui_area_y,
                                  SCREEN_W - margin * 2,
                                  ui_area_height);
        }

        // Draw main waveform visualizer
        if (waveform_height > 36) {
            waveform_draw(renderer,
                          &metrics,
                          margin,
                          waveform_y,
                          SCREEN_W - margin * 2,
                          waveform_height);
        }

        bench_overlay_present(overlay, renderer, &metrics, 0, 0);
        SDL_RenderPresent(renderer);

        bench_update_metrics(&metrics, delta_seconds * 1000.0);
    }

    audio_overlay_stop();
    audio_device_stop(SDL_FALSE);
    audio_device_shutdown();

    bench_overlay_destroy(overlay);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
