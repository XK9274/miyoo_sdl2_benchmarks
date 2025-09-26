#include "common/overlay.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <SDL2/SDL_mutex.h>
#include <SDL2/SDL_thread.h>

#include "common/metrics.h"

static const char *g_font_paths[] = {
    "/customer/app/Exo-2-Bold-Italic.ttf",
    "/customer/app/Helvetica-Neue-2.ttf",
    "/mnt/SDCARD/.tmp_update/lib/parasyte/python2.7/site-packages/pygame/examples/data/sans.ttf",
    "/mnt/SDCARD/.tmp_update/lib/parasyte/python2.7/site-packages/pygame/freesansbold.ttf",
    NULL
};

struct BenchOverlay {
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    int width;
    int height;
    int line_height;
    int max_rows;
    int max_lines;
    SDL_Color background;

    SDL_mutex *mutex;
    SDL_cond *cond;
    SDL_Thread *thread;
    SDL_bool running;
    SDL_bool dirty;
    SDL_bool has_pixels;

    int refresh_divisor;
    int refresh_counter;

    BenchOverlayLine pending_lines[BENCH_OVERLAY_MAX_LINES];
    int line_count;

    Uint8 *pixel_buffer;
    Uint8 *visible_buffer;
    size_t buffer_bytes;
    int pitch;
};

typedef struct {
    BenchOverlay *overlay;
    int font_size;
} BenchOverlayThreadArgs;

TTF_Font *bench_load_font(int size)
{
    for (int i = 0; g_font_paths[i] != NULL; ++i) {
        if (access(g_font_paths[i], F_OK) == 0) {
            TTF_Font *candidate = TTF_OpenFont(g_font_paths[i], size);
            if (candidate) {
                SDL_Log("Loaded font: %s", g_font_paths[i]);
                return candidate;
            }
        }
    }
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "bench_load_font: no fonts available");
    return NULL;
}

static void bench_overlay_free_texture_locked(BenchOverlay *overlay)
{
    if (overlay->texture) {
        SDL_DestroyTexture(overlay->texture);
        overlay->texture = NULL;
    }
}

static int bench_overlay_thread(void *userdata)
{
    BenchOverlayThreadArgs *args = (BenchOverlayThreadArgs *)userdata;
    BenchOverlay *overlay = args->overlay;
    const int font_size = args->font_size;
    free(args);

    TTF_Font *font = bench_load_font(font_size);
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0,
                                                         overlay->width,
                                                         overlay->height,
                                                         32,
                                                         SDL_PIXELFORMAT_RGBA32);
    if (!surface) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "bench_overlay_thread: failed to create surface (%s)",
                     SDL_GetError());
    }

    while (overlay->running) {
        SDL_LockMutex(overlay->mutex);
        while (!overlay->dirty && overlay->running) {
            SDL_CondWait(overlay->cond, overlay->mutex);
        }
        if (!overlay->running) {
            SDL_UnlockMutex(overlay->mutex);
            break;
        }

        BenchOverlayLine lines[BENCH_OVERLAY_MAX_LINES];
        SDL_Color background = overlay->background;
        const int line_count = SDL_min(overlay->line_count, overlay->max_lines);
        if (line_count > 0) {
            SDL_memcpy(lines, overlay->pending_lines, (size_t)line_count * sizeof(BenchOverlayLine));
        }
        overlay->dirty = SDL_FALSE;
        SDL_UnlockMutex(overlay->mutex);

        if (!surface) {
            continue;
        }

        SDL_FillRect(surface, NULL,
                     SDL_MapRGBA(surface->format,
                                 background.r,
                                 background.g,
                                 background.b,
                                 background.a));

        if (font) {
            const int line_adv = overlay->line_height;
            const int column_width = overlay->width / 2;
            const int divider_x = column_width;
            int left_y = 4;
            int right_y = 4;

            for (int i = 0; i < line_count; ++i) {
                if (lines[i].text[0] == '\0') {
                    if (lines[i].column == 1) {
                        right_y += line_adv;
                    } else {
                        left_y += line_adv;
                    }
                    continue;
                }

                SDL_Surface *line_surface = TTF_RenderUTF8_Blended(font,
                                                                   lines[i].text,
                                                                   lines[i].color);
                if (!line_surface) {
                    if (lines[i].column == 1) {
                        right_y += line_adv;
                    } else {
                        left_y += line_adv;
                    }
                    continue;
                }

                int x_offset = 8;
                int y_pos;

                if (lines[i].column == 1) {
                    // Right column
                    x_offset = divider_x + 8;
                    y_pos = right_y;
                    right_y += line_adv;
                } else {
                    // Left column
                    x_offset = 8;
                    y_pos = left_y;
                    left_y += line_adv;
                }

                // Apply alignment within column
                const int available_width = column_width - 16;
                if (lines[i].alignment == 1) {
                    // Center align
                    x_offset += (available_width - line_surface->w) / 2;
                } else if (lines[i].alignment == 2) {
                    // Right align
                    x_offset += available_width - line_surface->w;
                }

                SDL_Rect dst = {x_offset, y_pos, line_surface->w, line_surface->h};
                SDL_SetSurfaceBlendMode(line_surface, SDL_BLENDMODE_BLEND);
                SDL_BlitSurface(line_surface, NULL, surface, &dst);
                SDL_FreeSurface(line_surface);
            }

            // Draw column divider
            SDL_Rect divider = {divider_x - 1, 4, 2, overlay->height - 8};
            SDL_FillRect(surface, &divider,
                         SDL_MapRGBA(surface->format, 60, 80, 120, 180));
        }

        SDL_LockMutex(overlay->mutex);
        const size_t bytes = (size_t)surface->pitch * (size_t)surface->h;
        overlay->pitch = surface->pitch;
        overlay->buffer_bytes = bytes;
        if (!overlay->pixel_buffer) {
            overlay->pixel_buffer = (Uint8 *)SDL_malloc(bytes);
        }
        if (!overlay->visible_buffer) {
            overlay->visible_buffer = (Uint8 *)SDL_malloc(bytes);
        }
        if (overlay->pixel_buffer && overlay->visible_buffer) {
            SDL_memcpy(overlay->pixel_buffer, surface->pixels, bytes);
            overlay->has_pixels = SDL_TRUE;
        }
        SDL_UnlockMutex(overlay->mutex);
    }

    if (font) {
        TTF_CloseFont(font);
    }
    if (surface) {
        SDL_FreeSurface(surface);
    }
    return 0;
}

BenchOverlay *bench_overlay_create(SDL_Renderer *renderer,
                                   int width,
                                   int line_height,
                                   int max_rows)
{
    if (!renderer || width <= 0 || line_height <= 0 || max_rows <= 0) {
        return NULL;
    }

    BenchOverlay *overlay = (BenchOverlay *)SDL_calloc(1, sizeof(BenchOverlay));
    if (!overlay) {
        return NULL;
    }

    overlay->renderer = renderer;
    overlay->width = width;
    overlay->line_height = line_height;

    const int requested_rows = (max_rows > 0) ? max_rows : 1;
    const int clamped_rows = SDL_min(requested_rows, BENCH_OVERLAY_MAX_LINES);
    overlay->max_rows = clamped_rows;
    overlay->max_lines = SDL_min(clamped_rows * 2, BENCH_OVERLAY_MAX_LINES);
    if (overlay->max_lines <= 0) {
        overlay->max_lines = SDL_min(2, BENCH_OVERLAY_MAX_LINES);
    }
    overlay->height = overlay->line_height * overlay->max_rows;
    overlay->background = (SDL_Color){0, 0, 0, 255};
    overlay->running = SDL_TRUE;
    overlay->refresh_divisor = 10;
    if (overlay->refresh_divisor < 1) {
        overlay->refresh_divisor = 1;
    }
    overlay->refresh_counter = overlay->refresh_divisor - 1;

    overlay->mutex = SDL_CreateMutex();
    overlay->cond = SDL_CreateCond();
    if (!overlay->mutex || !overlay->cond) {
        bench_overlay_destroy(overlay);
        return NULL;
    }

    BenchOverlayThreadArgs *args = (BenchOverlayThreadArgs *)SDL_calloc(1, sizeof(BenchOverlayThreadArgs));
    if (!args) {
        bench_overlay_destroy(overlay);
        return NULL;
    }
    args->overlay = overlay;
    args->font_size = line_height - 5;
    overlay->thread = SDL_CreateThread(bench_overlay_thread, "bench_overlay", args);
    if (!overlay->thread) {
        SDL_free(args);
        bench_overlay_destroy(overlay);
        return NULL;
    }

    return overlay;
}

void bench_overlay_request_stop(BenchOverlay *overlay)
{
    if (!overlay) {
        return;
    }
    if (!overlay->mutex) {
        overlay->running = SDL_FALSE;
        if (overlay->cond) {
            SDL_CondSignal(overlay->cond);
        }
        return;
    }
    SDL_LockMutex(overlay->mutex);
    overlay->running = SDL_FALSE;
    if (overlay->cond) {
        SDL_CondSignal(overlay->cond);
    }
    SDL_UnlockMutex(overlay->mutex);
}

void bench_overlay_destroy(BenchOverlay *overlay)
{
    if (!overlay) {
        return;
    }

    bench_overlay_request_stop(overlay);
    if (overlay->thread) {
        SDL_WaitThread(overlay->thread, NULL);
    }

    if (overlay->pixel_buffer) {
        SDL_free(overlay->pixel_buffer);
        overlay->pixel_buffer = NULL;
    }
    if (overlay->visible_buffer) {
        SDL_free(overlay->visible_buffer);
        overlay->visible_buffer = NULL;
    }

    bench_overlay_free_texture_locked(overlay);

    if (overlay->cond) {
        SDL_DestroyCond(overlay->cond);
    }
    if (overlay->mutex) {
        SDL_DestroyMutex(overlay->mutex);
    }

    SDL_free(overlay);
}

void bench_overlay_submit(BenchOverlay *overlay,
                          const BenchOverlayLine *lines,
                          int line_count,
                          SDL_Color background)
{
    if (!overlay || !lines || line_count <= 0) {
        return;
    }

    SDL_LockMutex(overlay->mutex);
    overlay->line_count = SDL_min(line_count, overlay->max_lines);
    if (overlay->line_count > 0) {
        SDL_memcpy(overlay->pending_lines,
                   lines,
                   (size_t)overlay->line_count * sizeof(BenchOverlayLine));
    }
    overlay->background = background;
    if (overlay->refresh_divisor <= 1) {
        overlay->dirty = SDL_TRUE;
        SDL_CondSignal(overlay->cond);
    } else {
        overlay->refresh_counter++;
        if (overlay->refresh_counter >= overlay->refresh_divisor) {
            overlay->refresh_counter = 0;
            overlay->dirty = SDL_TRUE;
            SDL_CondSignal(overlay->cond);
        }
    }
    SDL_UnlockMutex(overlay->mutex);
}

void bench_overlay_present(BenchOverlay *overlay,
                           SDL_Renderer *renderer,
                           BenchMetrics *metrics,
                           int x,
                           int y)
{
    if (!overlay || !renderer) {
        return;
    }

    Uint8 *pixels = NULL;
    int pitch = 0;

    SDL_LockMutex(overlay->mutex);
    if (overlay->has_pixels && overlay->pixel_buffer && overlay->visible_buffer) {
        if (overlay->buffer_bytes > 0) {
            SDL_memcpy(overlay->visible_buffer,
                       overlay->pixel_buffer,
                       overlay->buffer_bytes);
            pixels = overlay->visible_buffer;
            pitch = overlay->pitch;
        }
        overlay->has_pixels = SDL_FALSE;
    }
    SDL_UnlockMutex(overlay->mutex);

    if (pixels) {
        if (!overlay->texture || renderer != overlay->renderer) {
            bench_overlay_free_texture_locked(overlay);
            overlay->texture = SDL_CreateTexture(renderer,
                                                 SDL_PIXELFORMAT_RGBA8888,
                                                 SDL_TEXTUREACCESS_STREAMING,
                                                 overlay->width,
                                                 overlay->height);
            if (overlay->texture) {
                SDL_SetTextureBlendMode(overlay->texture, SDL_BLENDMODE_BLEND);
            }
            overlay->renderer = renderer;
        }

        if (overlay->texture) {
            const int used_pitch = (pitch > 0) ? pitch : overlay->width * 4;
            SDL_UpdateTexture(overlay->texture,
                              NULL,
                              pixels,
                              used_pitch);
        }
    }

    if (overlay->texture) {
        SDL_Rect dst = {x, y, overlay->width, overlay->height};
        SDL_RenderCopy(renderer, overlay->texture, NULL, &dst);
        if (metrics) {
            metrics->draw_calls++;
            metrics->vertices_rendered += 4;
            metrics->triangles_rendered += 2;
        }
    }
}

int bench_overlay_height(const BenchOverlay *overlay)
{
    if (!overlay) {
        return 0;
    }
    return overlay->height;
}

// Old broken overlay builder functions removed - use overlay_grid.h instead
