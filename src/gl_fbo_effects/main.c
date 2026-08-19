#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "bench_common.h"
#include "gl_fbo_effects/input.h"
#include "gl_fbo_effects/overlay.h"
#include "gl_fbo_effects/scenes/effects.h"
#include "gl_fbo_effects/state.h"
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

    /* SDL_CreateRenderer force-ORs in SDL_RENDERER_PRESENTVSYNC in this SDL2
     * fork regardless of flags -- the hint is the only way to turn it off. */
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "0");
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    bench_driver_init(window, renderer);

    BenchLoadingScreen loading;
    SDL_bool loading_active = bench_loading_begin(&loading,
                                                  window,
                                                  renderer,
                                                  BENCH_LOADING_STYLE_GL);
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
        /* No loading-screen GL context handoff (unlike before the
         * common/gl_effect.c refactor) -- the shared refcounted context is
         * acquired lazily by rsgl_effects_init() below instead. One extra
         * context creation at startup, but far simpler ownership. */
        bench_loading_step(&loading, 0.45f, "Preparing GL context");
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

    double next_status_refresh_ms = 0.0;

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

        if (state.fbo_dirty) {
            rsgl_effects_apply_fbo_size(&state, renderer);
        }

        rsgl_state_update_layout(&state, overlay);

        SDL_SetRenderDrawColor(renderer, 8, 10, 18, 255);
        SDL_RenderClear(renderer);

        rsgl_effects_render(&state, renderer, &metrics, delta);

        bench_overlay_present(overlay, renderer, &metrics, 0, 0);
        SDL_RenderPresent(renderer);

        bench_update_metrics(&metrics, delta * 1000.0);
        state.running = running;

        if (metrics.accumulated_frame_time_ms >= next_status_refresh_ms) {
            char status_fields[BENCH_STATUS_GRID_CELLS][BENCH_STATUS_FIELD_LEN];
            bench_driver_format_status_grid(status_fields);
            bench_overlay_set_status_grid(overlay, status_fields, (SDL_Color){255, 255, 255, 255});
            next_status_refresh_ms = metrics.accumulated_frame_time_ms + 150.0;
        }

        rsgl_overlay_submit(overlay, &state, &metrics);
    }

    bench_driver_shutdown();
    rsgl_effects_cleanup(&state);
    rsgl_state_destroy(&state);
    bench_overlay_destroy(overlay);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
