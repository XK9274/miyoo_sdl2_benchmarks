#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <stdio.h>

#include "space_bench/gl_effects.h"
#include "space_bench/input.h"
#include "space_bench/render.h"
#include "space_bench/state.h"
#include "common/hotkeys.h"
#include "common/loading_screen.h"
#include "common/overlay_rows.h"

static const OverlayRowSpec g_space_rows[] = {
    {OVERLAY_ROW_FPS, {255, 255, 255, 255}, 0, NULL},
    {OVERLAY_ROW_FRAME_TIME, {0, 200, 255, 255}, 0, NULL},
    {OVERLAY_ROW_CUSTOM, {0, 255, 160, 255}, 0, "%s"},
    {OVERLAY_ROW_CUSTOM, {255, 180, 120, 255}, 0, "%s"},
};

static const OverlayKeybind g_space_keybinds[] = {
    {"SELECT", "Toggle overlay"},
    {"MENU", "Reset metrics"},
    {"D-Pad", "Move"},
    {"A", "Fire gun"},
    {"B", "Fire laser"},
    {"START", "Input mode"},
};

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    Uint64 perf_freq = SDL_GetPerformanceFrequency();
    Uint64 last_counter = SDL_GetPerformanceCounter();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() < 0) {
        printf("TTF_Init failed: %s\n", TTF_GetError());
    }

    SDL_Window *window = SDL_CreateWindow("SDL2 Space Bench",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          BENCH_NATIVE_W,
                                          BENCH_NATIVE_H,
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
                                                  BENCH_LOADING_STYLE_SHIP);
    if (loading_active) {
        bench_loading_set_colors(&loading, (SDL_Color){255, 150, 40, 255}, (SDL_Color){255, 150, 40, 255});
        bench_loading_step(&loading, 0.15f, "Preparing state");
    }

    SpaceBenchState state;
    space_state_init(&state);
    space_state_update_layout(&state, 0);

    if (loading_active) {
        bench_loading_step(&loading, 0.4f, "Compiling effect shaders");
    }
    /* Falls back to primitive draws in render/ if this fails. */
    if (!space_gl_effects_init(renderer)) {
        printf("space_gl_effects_init failed, continuing without GL effects\n");
    } else {
        space_gl_effects_warmup();
    }
    if (loading_active) {
        bench_loading_step(&loading, 0.9f, "Starfield ready");
        bench_loading_step(&loading, 1.0f, "Ready");
    }

    BenchMetrics metrics;
    bench_reset_metrics(&metrics);

    BenchOverlay *overlay = bench_overlay_create(renderer, bench_logical_w(), 16, 12);
    if (!overlay) {
        printf("Overlay creation failed\n");
        if (loading_active) {
            bench_loading_abort(&loading);
        }
        space_gl_effects_shutdown();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    bench_overlay_configure(overlay, g_space_rows, (int)SDL_arraysize(g_space_rows),
                            g_space_keybinds, (int)SDL_arraysize(g_space_keybinds));

    if (loading_active) {
        bench_loading_finish(&loading);
        loading_active = SDL_FALSE;
    }

    printf("SDL2 space bench started\n");

    SDL_bool running = SDL_TRUE;
    while (running) {
        const Uint64 frame_start_counter = SDL_GetPerformanceCounter();

        running = space_handle_input(&state, &metrics, overlay);
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
        bench_overlay_present(overlay, renderer, &metrics, 0, 0);
        SDL_RenderPresent(renderer);

        bench_update_metrics(&metrics, delta_seconds * 1000.0);
        bench_frame_limit_wait(frame_start_counter);

        const char *guidance = state.weapon_upgrades.guidance_active ? "ON" : "--";
        const char *thumper = state.weapon_upgrades.thumper_active ? "ON" : "--";
        char status_line[96];
        snprintf(status_line, sizeof(status_line), "Score %d | Enemies %d | Missed %d",
                 state.score, state.total_enemies_killed, state.player_hits);
        char upgrades_line[96];
        snprintf(upgrades_line, sizeof(upgrades_line), "Split %d | Guide %s | Drones %d | Thumper %s",
                 state.weapon_upgrades.split_level, guidance, state.weapon_upgrades.drone_count, thumper);
        const char *custom_values[] = {status_line, upgrades_line};
        bench_overlay_update(overlay, &metrics, custom_values, (int)SDL_arraysize(custom_values));
    }

    bench_driver_shutdown();
    bench_overlay_destroy(overlay);
    space_gl_effects_shutdown();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
