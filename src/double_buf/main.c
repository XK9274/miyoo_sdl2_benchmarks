#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <stdio.h>

#include "double_buf/input.h"
#include "double_buf/particles.h"
#include "double_buf/render.h"
#include "double_buf/state.h"
#include "common/geometry/shapes.h"
#include "common/hotkeys.h"
#include "common/loading_screen.h"
#include "common/overlay_rows.h"

static const OverlayRowSpec g_db_rows[] = {
    {OVERLAY_ROW_CUSTOM, {255, 180, 120, 255}, 0, "%s"},
    {OVERLAY_ROW_CUSTOM, {255, 255, 255, 255}, 0, "%s"},
    {OVERLAY_ROW_FPS, {255, 255, 255, 255}, 0, NULL},
    {OVERLAY_ROW_FRAME_TIME, {0, 200, 255, 255}, 0, NULL},
    {OVERLAY_ROW_DRAW_CALLS, {0, 255, 160, 255}, 0, NULL},
    {OVERLAY_ROW_VERTICES, {0, 255, 160, 255}, 0, NULL},
    {OVERLAY_ROW_TRIANGLES, {0, 255, 160, 255}, 0, NULL},
    {OVERLAY_ROW_CUSTOM, {255, 180, 120, 255}, 0, "%s"},
};

static const OverlayKeybind g_db_keybinds[] = {
    {"SELECT", "Toggle overlay"},
    {"MENU", "Reset metrics"},
    {"A/B", "+/-150 particles"},
    {"X", "Change render mode"},
    {"Y", "Toggle particles"},
    {"L1", "Toggle backdrop"},
    {"R1", "Toggle cube"},
    {"L2/R2", "Particle speed"},
    {"UP/DOWN", "Change shape"},
};

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

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
                                          BENCH_NATIVE_W, BENCH_NATIVE_H,
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
                                                  BENCH_LOADING_STYLE_RECT);
    if (loading_active) {
        bench_loading_step(&loading, 0.15f, "Initialising state");
    }

    BenchOverlay *overlay = bench_overlay_create(renderer, DB_SCREEN_W, 16, 12);
    if (!overlay) {
        if (loading_active) {
            bench_loading_abort(&loading);
        }
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    DoubleBenchState state;
    db_state_init(&state);
    if (loading_active) {
        bench_loading_step(&loading, 0.35f, "Preparing overlay");
    }

    BenchMetrics metrics;
    bench_reset_metrics(&metrics);
    bench_overlay_configure(overlay, g_db_rows, (int)SDL_arraysize(g_db_rows),
                            g_db_keybinds, (int)SDL_arraysize(g_db_keybinds));
    if (loading_active) {
        bench_loading_mark_idle(&loading, "GL modules idle - renderer path");
        bench_loading_finish(&loading);
        loading_active = SDL_FALSE;
    }

    printf("SDL2 hardware double buffer benchmark started\n");

    SDL_bool running = SDL_TRUE;
    while (running) {
        const Uint64 frame_start_counter = SDL_GetPerformanceCounter();

        running = db_handle_input(&state, &metrics, overlay);
        if (!running) {
            break;
        }

        const double delta_seconds = bench_get_delta_seconds(&last_counter, perf_freq);

        metrics.draw_calls = 0;
        metrics.vertices_rendered = 0;
        metrics.triangles_rendered = 0;

        db_particles_update(&state, delta_seconds);
        state.cube_rotation += (float)(delta_seconds * 1.8f);

        db_render_backdrop(&state, renderer, &metrics);
        db_render_cube_and_particles(&state, renderer, &metrics);

        bench_overlay_present(overlay, renderer, &metrics, 0, 0);
        SDL_RenderPresent(renderer);

        bench_update_metrics(&metrics, delta_seconds * 1000.0);
        bench_frame_limit_wait(frame_start_counter);

        char shape_label[64];
        snprintf(shape_label, sizeof(shape_label), "Shape %d/%d: %s",
                 state.shape_type + 1, SHAPE_COUNT, bench_get_shape_name(state.shape_type));
        char particle_label[96];
        snprintf(particle_label, sizeof(particle_label), "Particles %d/%d | Cube %s | Grid %s",
                 state.particle_count, DB_MAX_PARTICLES,
                 state.show_cube ? "ON" : "OFF", state.backdrop_grid ? "ON" : "OFF");
        char state_label[64];
        snprintf(state_label, sizeof(state_label), "Speed %.0f | Mode %d | Rot %.2f",
                 state.particle_speed, state.render_mode, state.cube_rotation);
        const char *custom_values[] = {shape_label, particle_label, state_label};
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
