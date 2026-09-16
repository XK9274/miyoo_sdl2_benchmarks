#include "common/loading_screen.h"

#include <SDL2/SDL_log.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "common/overlay.h"
#include "common/geometry/core.h"
#include "common/gl_effect_library.h"

#define LOADING_GL_WIDTH 80
#define LOADING_GL_HEIGHT 60
#define LOADING_MESSAGE_MAX 95

static void bench_loading_reset(BenchLoadingScreen *screen)
{
    if (!screen) {
        return;
    }
    SDL_memset(screen, 0, sizeof(*screen));
}

static void bench_loading_destroy_gl(BenchLoadingScreen *screen)
{
    if (!screen) {
        return;
    }

    if (screen->gl_ready) {
        gl_effect_destroy_program(screen->gl_program);
        screen->gl_program = 0;
        gl_effect_target_destroy(&screen->gl_target);
        gl_effect_context_release();
        screen->gl_ready = SDL_FALSE;
        screen->gl_time_accum = 0.0f;
    }

    screen->gl_init_pending = SDL_FALSE;
    screen->gl_initializing = SDL_FALSE;
    screen->gl_first_frame_presented = SDL_FALSE;
}

static SDL_bool bench_loading_setup_gl(BenchLoadingScreen *screen)
{
    if (!screen || !screen->renderer) {
        return SDL_FALSE;
    }

    if (!gl_effect_context_acquire()) {
        return SDL_FALSE;
    }

    if (!gl_effect_target_create(&screen->gl_target, screen->renderer,
                                 LOADING_GL_WIDTH, LOADING_GL_HEIGHT)) {
        gl_effect_context_release();
        return SDL_FALSE;
    }

    screen->gl_program = gl_effect_compile_program(
        gl_effect_library_fragment_source(GL_EFFECT_LIBRARY_SOFT_WAVES));
    if (!screen->gl_program) {
        gl_effect_target_destroy(&screen->gl_target);
        gl_effect_context_release();
        return SDL_FALSE;
    }

    screen->gl_ready = SDL_TRUE;
    return SDL_TRUE;
}

static void bench_loading_try_initialize_gl(BenchLoadingScreen *screen)
{
    if (!screen || screen->style != BENCH_LOADING_STYLE_GL) {
        return;
    }

    if (!screen->gl_init_pending || screen->gl_ready || screen->gl_initializing) {
        if (screen->gl_ready) {
            screen->gl_init_pending = SDL_FALSE;
        }
        return;
    }

    screen->gl_initializing = SDL_TRUE;
    if (!bench_loading_setup_gl(screen)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "bench_loading: deferred GL setup failed, staying in rect style");
        bench_loading_destroy_gl(screen);
        screen->style = BENCH_LOADING_STYLE_RECT;
        screen->gl_init_pending = SDL_FALSE;
    } else {
        screen->gl_init_pending = SDL_FALSE;
    }
    screen->gl_initializing = SDL_FALSE;
}

static void bench_loading_update_gl(BenchLoadingScreen *screen)
{
    if (!screen->gl_ready) {
        return;
    }

    const Uint64 now = SDL_GetPerformanceCounter();
    double delta = 0.0;
    if (screen->last_counter != 0) {
        delta = (double)(now - screen->last_counter) / (double)screen->perf_freq;
    }
    screen->last_counter = now;
    screen->gl_time_accum += (float)delta;

    gl_effect_render(&screen->gl_target, screen->gl_program,
                     gl_effect_set_time_uniform, &screen->gl_time_accum);
}

/* BENCH_LOADING_STYLE_SHIP: solid 3D dart geometry, wingspan on X/Z (rotates with the spin), slight Y taper so it isn't flat. */
static const float ship_base[4][3] = {
    { 1.00f, -0.36f,  0.00f},  /* nose */
    {-0.55f,  0.00f, -0.62f},  /* left wingtip */
    {-0.55f,  0.00f,  0.62f},  /* right wingtip */
    {-0.30f, -0.36f,  0.00f},  /* tail */
};

static const int ship_faces[4][3] = {
    {0, 1, 2},
    {0, 1, 3},
    {0, 2, 3},
    {1, 2, 3},
};

static const int ship_edges[6][2] = {
    {0, 1}, {0, 2}, {1, 2},
    {0, 3}, {1, 3}, {2, 3},
};

/* BENCH_LOADING_STYLE_SHIP: solid 3D dart, rotated/projected via common/geometry/core.c. Faces filled with the background color for a silhouette; edges drawn on top. */
static void bench_loading_render_ship(BenchLoadingScreen *screen,
                                      int renderer_w,
                                      int renderer_h)
{
    const Uint64 now = SDL_GetPerformanceCounter();
    double delta = 0.0;
    if (screen->last_counter != 0 && screen->perf_freq > 0) {
        delta = (double)(now - screen->last_counter) / (double)screen->perf_freq;
    }
    screen->last_counter = now;
    screen->ship_angle += (float)delta * 1.8f;  /* matches double-buffer suite's rotation speed */

    RotationCache rotation_cache = {.rotation = NAN};
    bench_update_rotation_cache(&rotation_cache, screen->ship_angle);

    const float origin_x = (float)renderer_w * 0.5f;
    const float origin_y = (float)renderer_h * 0.5f;
    const float size = SDL_min((float)renderer_w, (float)renderer_h) * 0.315f;  /* 0.42f, reduced 25% */

    BenchVertex vertices[4];
    for (int i = 0; i < 4; ++i) {
        bench_project_vertex(ship_base[i], &rotation_cache, origin_x, origin_y, size, &vertices[i]);
    }

    const SDL_Color fill_color = screen->bar_fill;
    const SDL_Color ship_edge_color = {
        (Uint8)(fill_color.r * 0.55f), (Uint8)(fill_color.g * 0.55f), (Uint8)(fill_color.b * 0.55f), 255
    };

    SDL_Vertex triangle_vertices[4 * 3];
    int triangle_count = 0;
    for (int face = 0; face < 4; ++face) {
        const int *indices = ship_faces[face];
        const int base = triangle_count * 3;
        bench_setup_sdl_vertex(&triangle_vertices[base + 0], &vertices[indices[0]], &fill_color);
        bench_setup_sdl_vertex(&triangle_vertices[base + 1], &vertices[indices[1]], &fill_color);
        bench_setup_sdl_vertex(&triangle_vertices[base + 2], &vertices[indices[2]], &fill_color);
        triangle_count++;
    }

    SDL_SetRenderDrawBlendMode(screen->renderer, SDL_BLENDMODE_BLEND);
    bench_render_triangle_batch(screen->renderer, triangle_vertices, triangle_count, NULL);

    bench_render_edge_batch(screen->renderer,
                            vertices,
                            ship_edges,
                            (int)(sizeof(ship_edges) / sizeof(ship_edges[0])),
                            &ship_edge_color,
                            1,
                            0,
                            NULL);
}

static void bench_loading_render_message(BenchLoadingScreen *screen,
                                         int renderer_w,
                                         int renderer_h)
{
    if (!screen->font || screen->message[0] == '\0') {
        return;
    }

    SDL_Surface *surface = TTF_RenderUTF8_Blended(screen->font,
                                                  screen->message,
                                                  screen->text_color);
    if (!surface) {
        return;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(screen->renderer, surface);
    if (!texture) {
        SDL_FreeSurface(surface);
        return;
    }

    SDL_Rect dst;
    dst.w = surface->w;
    dst.h = surface->h;
    dst.x = (renderer_w - dst.w) / 2;
    dst.y = (int)((float)renderer_h * 0.75f);

    SDL_FreeSurface(surface);

    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_RenderCopy(screen->renderer, texture, NULL, &dst);
    SDL_DestroyTexture(texture);
}

static void bench_loading_render_bar(BenchLoadingScreen *screen,
                                     int renderer_w,
                                     int renderer_h)
{
    const int bar_width = renderer_w / 2;
    const int bar_height = 20;
    SDL_Rect outline = {
        (renderer_w - bar_width) / 2,
        (int)((float)renderer_h * 0.75f) + 76,
        bar_width,
        bar_height
    };

    SDL_SetRenderDrawBlendMode(screen->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(screen->renderer,
                           screen->bar_outline.r,
                           screen->bar_outline.g,
                           screen->bar_outline.b,
                           screen->bar_outline.a);
    SDL_RenderFillRect(screen->renderer, &outline);

    SDL_Rect inner = {
        outline.x + 2,
        outline.y + 2,
        outline.w - 4,
        outline.h - 4
    };
    SDL_SetRenderDrawColor(screen->renderer, 12, 18, 32, 200);
    SDL_RenderFillRect(screen->renderer, &inner);

    SDL_Rect fill = inner;
    fill.w = (int)((float)fill.w * SDL_clamp(screen->progress, 0.0f, 1.0f));
    SDL_SetRenderDrawColor(screen->renderer,
                           screen->bar_fill.r,
                           screen->bar_fill.g,
                           screen->bar_fill.b,
                           screen->bar_fill.a);
    SDL_RenderFillRect(screen->renderer, &fill);
}

static void bench_loading_render_percent(BenchLoadingScreen *screen,
                                         int renderer_w,
                                         int renderer_h)
{
    if (!screen->font) {
        return;
    }

    char buffer[32];
    SDL_snprintf(buffer, sizeof(buffer), "%d%%", (int)(screen->progress * 100.0f));

    SDL_Surface *surface = TTF_RenderUTF8_Blended(screen->font, buffer, screen->text_color);
    if (!surface) {
        return;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(screen->renderer, surface);
    if (!texture) {
        SDL_FreeSurface(surface);
        return;
    }

    SDL_Rect dst;
    dst.w = surface->w;
    dst.h = surface->h;
    dst.x = (renderer_w - dst.w) / 2;
    dst.y = (int)((float)renderer_h * 0.75f) + 38;

    SDL_FreeSurface(surface);

    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_RenderCopy(screen->renderer, texture, NULL, &dst);
    SDL_DestroyTexture(texture);
}

static void bench_loading_present(BenchLoadingScreen *screen)
{
    if (!screen || !screen->renderer) {
        return;
    }

    if (screen->state_mutex) {
        SDL_LockMutex(screen->state_mutex);
    }

    int w = 0;
    int h = 0;
    SDL_GetRendererOutputSize(screen->renderer, &w, &h);

    SDL_SetRenderDrawBlendMode(screen->renderer, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(screen->renderer,
                           screen->background.r,
                           screen->background.g,
                           screen->background.b,
                           screen->background.a);
    SDL_RenderClear(screen->renderer);

    if (screen->style == BENCH_LOADING_STYLE_GL && screen->gl_ready && screen->gl_target.screen_texture) {
        bench_loading_update_gl(screen);
        SDL_Rect dst = {0, 0, w, h};
        SDL_RenderCopy(screen->renderer, screen->gl_target.screen_texture, NULL, &dst);
    } else if (screen->style == BENCH_LOADING_STYLE_SHIP) {
        bench_loading_render_ship(screen, w, h);
    }

    bench_loading_render_bar(screen, w, h);
    bench_loading_render_percent(screen, w, h);
    bench_loading_render_message(screen, w, h);

    SDL_RenderPresent(screen->renderer);

    if (screen->style == BENCH_LOADING_STYLE_GL && screen->gl_init_pending) {
        if (!screen->gl_first_frame_presented) {
            screen->gl_first_frame_presented = SDL_TRUE;
        } else {
            bench_loading_try_initialize_gl(screen);
        }
    }

    if (screen->state_mutex) {
        SDL_UnlockMutex(screen->state_mutex);
    }
}

/* BENCH_LOADING_STYLE_SHIP only: keeps the ship spinning at a steady cadence
   regardless of how often (or infrequently) the caller's loading work calls
   bench_loading_step. */
static int bench_loading_render_thread_fn(void *userdata)
{
    BenchLoadingScreen *screen = (BenchLoadingScreen *)userdata;

    for (;;) {
        SDL_LockMutex(screen->state_mutex);
        const SDL_bool keep_running = screen->render_thread_running;
        SDL_UnlockMutex(screen->state_mutex);
        if (!keep_running) {
            break;
        }

        bench_loading_present(screen);
        SDL_Delay(16);
    }

    return 0;
}

SDL_bool bench_loading_begin(BenchLoadingScreen *screen,
                             SDL_Window *window,
                             SDL_Renderer *renderer,
                             BenchLoadingStyle style)
{
    if (!screen || !window || !renderer) {
        return SDL_FALSE;
    }

    bench_loading_reset(screen);

    screen->window = window;
    screen->renderer = renderer;
    screen->style = style;
    screen->active = SDL_TRUE;
    screen->progress = 0.0f;
    screen->background = (SDL_Color){8, 10, 18, 255};
    screen->bar_outline = (SDL_Color){28, 42, 62, 255};
    screen->bar_fill = (SDL_Color){0, 210, 180, 255};
    screen->text_color = (SDL_Color){220, 230, 255, 255};
    screen->start_ticks = SDL_GetTicks64();
    screen->last_ticks = screen->start_ticks;
    screen->perf_freq = SDL_GetPerformanceFrequency();
    screen->last_counter = SDL_GetPerformanceCounter();

    screen->font = bench_load_font(24);
    screen->owns_font = (screen->font != NULL);

    if (style == BENCH_LOADING_STYLE_GL) {
        screen->gl_init_pending = SDL_TRUE;
        screen->gl_first_frame_presented = SDL_FALSE;
    }

    if (style == BENCH_LOADING_STYLE_SHIP) {
        screen->state_mutex = SDL_CreateMutex();
        if (screen->state_mutex) {
            screen->render_thread_running = SDL_TRUE;
            screen->render_thread = SDL_CreateThread(bench_loading_render_thread_fn, "bench_loading_ship", screen);
            if (!screen->render_thread) {
                screen->render_thread_running = SDL_FALSE;
                SDL_DestroyMutex(screen->state_mutex);
                screen->state_mutex = NULL;
            }
        }
    }

    if (!screen->render_thread) {
        bench_loading_present(screen);
    }
    return SDL_TRUE;
}

void bench_loading_set_colors(BenchLoadingScreen *screen,
                              SDL_Color text_color,
                              SDL_Color bar_fill_color)
{
    if (!screen || !screen->active) {
        return;
    }

    if (screen->state_mutex) {
        SDL_LockMutex(screen->state_mutex);
    }
    screen->text_color = text_color;
    screen->bar_fill = bar_fill_color;
    if (screen->state_mutex) {
        SDL_UnlockMutex(screen->state_mutex);
    }
}

void bench_loading_step(BenchLoadingScreen *screen,
                        float progress,
                        const char *label)
{
    if (!screen || !screen->active) {
        return;
    }

    if (screen->state_mutex) {
        SDL_LockMutex(screen->state_mutex);
    }
    screen->progress = SDL_clamp(progress, 0.0f, 1.0f);
    if (label && label[0] != '\0') {
        SDL_strlcpy(screen->message, label, sizeof(screen->message));
    }
    if (screen->state_mutex) {
        SDL_UnlockMutex(screen->state_mutex);
    }

    if (!screen->render_thread) {
        bench_loading_present(screen);
    }
    SDL_PumpEvents();
}

void bench_loading_mark_idle(BenchLoadingScreen *screen,
                             const char *label)
{
    if (!screen || !screen->active) {
        return;
    }

    if (screen->state_mutex) {
        SDL_LockMutex(screen->state_mutex);
    }
    screen->progress = 1.0f;
    if (label && label[0] != '\0') {
        SDL_strlcpy(screen->message, label, sizeof(screen->message));
    } else {
        SDL_strlcpy(screen->message,
                    "GL modules idle - awaiting additional workloads",
                    sizeof(screen->message));
    }
    if (screen->state_mutex) {
        SDL_UnlockMutex(screen->state_mutex);
    }

    if (!screen->render_thread) {
        bench_loading_present(screen);
    }
}

/* Stops and joins the ship's render thread (if any) so nothing is still
   touching screen->font/renderer/gl_* when the caller tears them down. */
static void bench_loading_stop_render_thread(BenchLoadingScreen *screen)
{
    if (!screen->render_thread) {
        return;
    }

    SDL_LockMutex(screen->state_mutex);
    screen->render_thread_running = SDL_FALSE;
    SDL_UnlockMutex(screen->state_mutex);

    SDL_WaitThread(screen->render_thread, NULL);
    screen->render_thread = NULL;

    SDL_DestroyMutex(screen->state_mutex);
    screen->state_mutex = NULL;
}

void bench_loading_finish(BenchLoadingScreen *screen)
{
    if (!screen || !screen->active) {
        return;
    }

    bench_loading_stop_render_thread(screen);

    screen->progress = 1.0f;
    if (screen->message[0] == '\0') {
        SDL_strlcpy(screen->message, "Loading complete", sizeof(screen->message));
    }
    bench_loading_present(screen);

    if (screen->owns_font && screen->font) {
        TTF_CloseFont(screen->font);
    }
    screen->font = NULL;
    screen->owns_font = SDL_FALSE;

    bench_loading_destroy_gl(screen);
    screen->active = SDL_FALSE;
}

void bench_loading_abort(BenchLoadingScreen *screen)
{
    if (!screen || !screen->active) {
        return;
    }

    bench_loading_stop_render_thread(screen);

    if (screen->owns_font && screen->font) {
        TTF_CloseFont(screen->font);
    }
    screen->font = NULL;
    screen->owns_font = SDL_FALSE;

    bench_loading_destroy_gl(screen);
    screen->active = SDL_FALSE;
}
