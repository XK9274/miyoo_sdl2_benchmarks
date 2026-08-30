#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include <stdio.h>

#include "bench_common.h"
#include "common/loading_screen.h"
#include "common/render3d/pipeline.h"
#include "obj_model_loader/input.h"
#include "obj_model_loader/overlay.h"
#include "obj_model_loader/state.h"

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
    if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) == 0) {
        printf("IMG_Init failed: %s\n", IMG_GetError());
    }

    SDL_Window *window = SDL_CreateWindow("SDL2 Obj Model Loader",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          BENCH_NATIVE_W, BENCH_NATIVE_H,
                                          SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        IMG_Quit();
        TTF_Quit();
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
        IMG_Quit();
        TTF_Quit();
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
        bench_loading_step(&loading, 0.15f, "Loading model");
    }

    ObjModelLoaderState state;
    obj_state_init(renderer, &state);

    BenchMetrics metrics;
    bench_reset_metrics(&metrics);

    BenchOverlay *overlay = bench_overlay_create(renderer, bench_logical_w(), 16, 12);
    if (!overlay) {
        printf("Overlay creation failed\n");
        if (loading_active) {
            bench_loading_abort(&loading);
        }
        obj_state_destroy(renderer, &state);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    obj_state_update_layout(&state, overlay);
    obj_overlay_submit(overlay, &state, &metrics);
    if (loading_active) {
        bench_loading_step(&loading, 1.0f, "Obj model loader ready");
        bench_loading_finish(&loading);
        loading_active = SDL_FALSE;
    }

    printf("SDL2 Obj Model Loader initialised (model: %s)\n", state.model_label);

    double next_status_refresh_ms = 0.0;

    SDL_bool running = SDL_TRUE;
    while (running) {
        const Uint64 frame_start_counter = SDL_GetPerformanceCounter();

        if (!obj_handle_input(&state, &metrics)) {
            break;
        }

        const double delta_seconds = bench_get_delta_seconds(&last_counter, perf_freq);

        metrics.draw_calls = 0;
        metrics.vertices_rendered = 0;
        metrics.triangles_rendered = 0;
        metrics.texture_switches = 0;

        obj_state_update_layout(&state, overlay);

        if (state.auto_rotate) {
            camera3d_auto_rotate(&state.camera, state.auto_rotate_radians_per_second, (float)delta_seconds);
        }

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer, 10, 12, 20, 255);
        SDL_RenderClear(renderer);
        metrics.draw_calls++;

        const int content_x = 0;
        const int content_y = (int)state.top_margin;
        const int content_width = bench_logical_w();
        const int content_height = SDL_max(1, bench_logical_h() - (int)state.top_margin);

        Render3DFrameParams params;
        params.view = camera3d_view_matrix(&state.camera);
        params.projection = camera3d_projection_matrix(&state.camera,
                                                        (float)content_width / (float)content_height);
        params.near_plane = state.camera.near_plane;
        params.light_direction = (Vec3){0.4f, -1.0f, 0.3f};
        params.ambient_floor = state.ambient_floor;
        params.viewport_x = content_x;
        params.viewport_y = content_y;
        params.viewport_width = content_width;
        params.viewport_height = content_height;
        params.wireframe = state.wireframe;

        /* No frustum side-plane clipping in the pipeline (only near-plane), so
         * this clip rect is the hard guarantee nothing draws over the overlay. */
        const SDL_Rect content_clip = {content_x, content_y, content_width, content_height};
        SDL_RenderSetClipRect(renderer, &content_clip);
        render3d_draw_mesh(renderer, &state.model.mesh, state.model.material_textures,
                           &params, state.scratch, &metrics);
        SDL_RenderSetClipRect(renderer, NULL);

        bench_overlay_present(overlay, renderer, &metrics, 0, 0);
        SDL_RenderPresent(renderer);

        bench_update_metrics(&metrics, delta_seconds * 1000.0);
        bench_frame_limit_wait(frame_start_counter);

        if (metrics.accumulated_frame_time_ms >= next_status_refresh_ms) {
            char status_fields[BENCH_STATUS_GRID_CELLS][BENCH_STATUS_FIELD_LEN];
            bench_driver_format_status_grid(status_fields);
            bench_overlay_set_status_grid(overlay, status_fields, (SDL_Color){255, 255, 255, 255});
            next_status_refresh_ms = metrics.accumulated_frame_time_ms + 150.0;
        }

        obj_overlay_submit(overlay, &state, &metrics);
    }

    bench_driver_shutdown();
    obj_state_destroy(renderer, &state);
    bench_overlay_destroy(overlay);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
    return 0;
}
