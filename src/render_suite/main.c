#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "bench_common.h"
#include "render_suite/input.h"
#include "render_suite/resources.h"
#include "render_suite/scenes/fill.h"
#include "render_suite/scenes/lines.h"
#include "render_suite/scenes/texture.h"
#include "render_suite/scenes/geometry.h"
#include "render_suite/scenes/scaling.h"
#include "render_suite/scenes/memory.h"
#include "render_suite/scenes/pixels.h"
#include "render_suite/state.h"
#include "common/hotkeys.h"
#include "common/loading_screen.h"
#include "common/overlay_rows.h"

static const char *g_rs_scene_names[SCENE_MAX] = {
    "Fill Rate", "Texture", "Lines/Geometry", "3D Geometry",
    "Resolution Scaling", "Memory Management", "Pixel Operations",
};

static const char *g_rs_geometry_mode_labels[RS_GEOMETRY_RENDER_MODE_MAX] = {
    "Filled Faces", "Wireframe", "Vertex Points",
};

static const OverlayRowSpec g_rs_rows[] = {
    {OVERLAY_ROW_CUSTOM, {240, 194, 94, 255}, 0, "%s"},
    {OVERLAY_ROW_CUSTOM, {240, 194, 94, 255}, 0, "%s"},
    {OVERLAY_ROW_FPS, {255, 255, 255, 255}, 0, NULL},
    {OVERLAY_ROW_FRAME_TIME, {0, 200, 255, 255}, 0, NULL},
    {OVERLAY_ROW_DRAW_CALLS, {0, 255, 160, 255}, 0, NULL},
    {OVERLAY_ROW_VERTICES, {0, 255, 160, 255}, 0, NULL},
    {OVERLAY_ROW_TRIANGLES, {0, 255, 160, 255}, 0, NULL},
    {OVERLAY_ROW_TEXTURE_SWITCHES, {0, 255, 160, 255}, 0, NULL},
    {OVERLAY_ROW_MEMORY, {0, 200, 255, 255}, 0, NULL},
    {OVERLAY_ROW_RESOURCE_OPS, {255, 180, 120, 255}, 0, NULL},
    {OVERLAY_ROW_TIMING_OVERHEAD, {0, 200, 255, 255}, 0, NULL},
    {OVERLAY_ROW_CUSTOM, {255, 180, 120, 255}, 0, "%s"},
};

static const OverlayKeybind g_rs_keybinds[] = {
    {"SELECT", "Toggle overlay"},
    {"MENU", "Reset metrics"},
    {"L2/R2", "Switch scene"},
    {"A", "Toggle auto cycle"},
    {"B", "Adjust stress level"},
    {"X", "Cycle geometry mode"},
};

/* RS_FORCE_SCENE=<name> pins active_scene and disables auto-cycle; used for
 * isolated A/B perf comparisons (e.g. geometry-scene NEON vs scalar). */
static SDL_bool rs_scene_from_name(const char *name, SceneKind *out)
{
    static const struct { const char *name; SceneKind scene; } table[] = {
        {"fill", SCENE_FILL}, {"texture", SCENE_TEXTURE}, {"lines", SCENE_LINES},
        {"geometry", SCENE_GEOMETRY}, {"scaling", SCENE_SCALING},
        {"memory", SCENE_MEMORY}, {"pixels", SCENE_PIXELS},
    };
    for (size_t i = 0; i < SDL_arraysize(table); i++) {
        if (SDL_strcasecmp(name, table[i].name) == 0) {
            *out = table[i].scene;
            return SDL_TRUE;
        }
    }
    return SDL_FALSE;
}

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
        bench_loading_step(&loading, 0.1f, "Initialising state");
    }

    rs_print_system_info();

    RenderSuiteState state;
    rs_state_init(&state);

    const char *force_scene_name = SDL_getenv("RS_FORCE_SCENE");
    SceneKind forced_scene;
    if (force_scene_name && rs_scene_from_name(force_scene_name, &forced_scene)) {
        state.active_scene = forced_scene;
        state.auto_cycle = SDL_FALSE;
    }

    const char *bench_duration_str = SDL_getenv("RS_BENCH_DURATION_S");
    const double bench_duration_s = bench_duration_str ? SDL_atof(bench_duration_str) : 0.0;
    const char *bench_tag = SDL_getenv("RS_BENCH_TAG");

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

    BenchOverlay *overlay = bench_overlay_create(renderer, bench_logical_w(), 16, 12);
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

    bench_overlay_configure(overlay, g_rs_rows, (int)SDL_arraysize(g_rs_rows),
                            g_rs_keybinds, (int)SDL_arraysize(g_rs_keybinds));
    if (loading_active) {
        bench_loading_step(&loading, 0.8f, "Render suite ready");
        bench_loading_finish(&loading);
        loading_active = SDL_FALSE;
    }

    printf("SDL2 Render Suite initialised\n");

    double next_bench_log_ms = 0.0;

    SDL_bool running = SDL_TRUE;
    while (running) {
        const Uint64 frame_start_counter = SDL_GetPerformanceCounter();

        if (!rs_handle_input(&state, &metrics, overlay)) {
            break;
        }

        const double delta_seconds = bench_get_delta_seconds(&last_counter, perf_freq);

        metrics.draw_calls = 0;
        metrics.vertices_rendered = 0;
        metrics.triangles_rendered = 0;

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
        bench_frame_limit_wait(frame_start_counter);

        char scene_label[64];
        snprintf(scene_label, sizeof(scene_label), "Scene: %s | Auto %s",
                 g_rs_scene_names[state.active_scene], state.auto_cycle ? "ON" : "OFF");
        char stress_label[48];
        snprintf(stress_label, sizeof(stress_label), "Stress L%d x%.1f",
                 state.stress_level, rs_state_stress_factor(&state));
        char mode_label[48] = "";
        if (state.active_scene == SCENE_GEOMETRY) {
            const int mode_index = (state.geometry_render_mode >= 0) ?
                (state.geometry_render_mode % RS_GEOMETRY_RENDER_MODE_MAX) : 0;
            snprintf(mode_label, sizeof(mode_label), "Geometry Mode: %s", g_rs_geometry_mode_labels[mode_index]);
        }
        const char *custom_values[] = {scene_label, stress_label, mode_label};
        bench_overlay_update(overlay, &metrics, custom_values, (int)SDL_arraysize(custom_values));

        if (bench_tag && metrics.accumulated_frame_time_ms >= next_bench_log_ms) {
            printf("[BENCH] tag=%s scene=%d elapsed_s=%.1f frame=%llu fps=%.2f avg_fps=%.2f "
                   "min_fps=%.2f max_fps=%.2f frame_ms=%.3f tris=%llu verts=%llu\n",
                   bench_tag, (int)state.active_scene, metrics.accumulated_frame_time_ms / 1000.0,
                   (unsigned long long)metrics.frame_count, metrics.current_fps, metrics.avg_fps,
                   metrics.min_fps, metrics.max_fps, metrics.frame_time_ms,
                   (unsigned long long)metrics.triangles_rendered,
                   (unsigned long long)metrics.vertices_rendered);
            fflush(stdout);
            next_bench_log_ms = metrics.accumulated_frame_time_ms + 2000.0;
        }

        if (bench_duration_s > 0.0 && metrics.accumulated_frame_time_ms >= bench_duration_s * 1000.0) {
            running = SDL_FALSE;
        }
    }

    bench_driver_shutdown();

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
