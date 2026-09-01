#define _POSIX_C_SOURCE 200809L

#include "common/overlay.h"
#include "common/overlay_internal.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <SDL2/SDL_mutex.h>
#include <SDL2/SDL_thread.h>

#include "common/asset_path.h"
#include "common/backend_probe.h"
#include "common/display_config.h"
#include "common/driver_support.h"
#include "common/memory_opt.h"
#include "common/metrics.h"

static const char *g_font_paths[] = {
    "/customer/app/Exo-2-Bold-Italic.ttf",
    "/customer/app/Helvetica-Neue-2.ttf",
    "/mnt/SDCARD/.tmp_update/lib/parasyte/python2.7/site-packages/pygame/examples/data/sans.ttf",
    "/mnt/SDCARD/.tmp_update/lib/parasyte/python2.7/site-packages/pygame/freesansbold.ttf",
    NULL
};

#define OVERLAY_PANEL_BG_R 0x17
#define OVERLAY_PANEL_BG_G 0x17
#define OVERLAY_PANEL_BG_B 0x17
#define OVERLAY_PANEL_BG_A 165
#define OVERLAY_MIN_EFFECTIVE_ROWS 24
#define OVERLAY_CHART_ROW_SPAN 4   /* two 2-row-tall charts */
#define OVERLAY_HEADER_ROW_SPAN 1
#define OVERLAY_BACKENDS_ROW_SPAN 8 /* Render, Audio, Input, Power, Threads, Display, Build, CPU Freq */
#define OVERLAY_FONT_SIZE_MARGIN 4

typedef struct {
    BenchOverlay *overlay;
    int font_size;
} BenchOverlayThreadArgs;

TTF_Font *bench_load_font(int size)
{
    char bundled_path[512];
    if (bench_resolve_asset_path("ThaleahFat.ttf", bundled_path, sizeof(bundled_path))) {
        TTF_Font *bundled = TTF_OpenFont(bundled_path, size);
        if (bundled) {
            SDL_Log("Loaded font: %s", bundled_path);
            return bundled;
        }
    }

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

void overlay_draw_text_line(SDL_Surface *surface, TTF_Font *font, SDL_Rect bounds,
                            int alignment, SDL_Color color, const char *text)
{
    if (!surface || !font || !text || text[0] == '\0') {
        return;
    }

    SDL_Surface *line_surface = TTF_RenderUTF8_Blended(font, text, color);
    if (!line_surface) {
        return;
    }

    int x = bounds.x;
    if (alignment == 1) {
        x += (bounds.w - line_surface->w) / 2;
    } else if (alignment == 2) {
        x += bounds.w - line_surface->w;
    }
    const int y = bounds.y + (bounds.h - line_surface->h) / 2;

    SDL_Rect dst = {x, y, line_surface->w, line_surface->h};
    SDL_SetClipRect(surface, &bounds);
    SDL_SetSurfaceBlendMode(line_surface, SDL_BLENDMODE_BLEND);
    SDL_BlitSurface(line_surface, NULL, surface, &dst);
    SDL_SetClipRect(surface, NULL);
    SDL_FreeSurface(line_surface);
}

/* Hand-drawn zigzag lightning bolt -- bitmap fonts can't be relied on to
 * carry a real glyph for this. */
static void overlay_draw_charge_glyph(SDL_Surface *surface, int x, int y, int size, SDL_Color color)
{
    const Uint32 pixel = SDL_MapRGBA(surface->format, color.r, color.g, color.b, 255);
    const SDL_Point pts[] = {
        {x + size / 2, y},
        {x, y + size / 2},
        {x + size / 3, y + size / 2},
        {x, y + size},
        {x + size, y + size / 3},
        {x + size * 2 / 3, y + size / 3},
    };
    for (int i = 0; i + 1 < (int)SDL_arraysize(pts); ++i) {
        SDL_Rect seg = {SDL_min(pts[i].x, pts[i + 1].x),
                        SDL_min(pts[i].y, pts[i + 1].y),
                        SDL_max(1, abs(pts[i + 1].x - pts[i].x)),
                        SDL_max(1, abs(pts[i + 1].y - pts[i].y))};
        SDL_FillRect(surface, &seg, pixel);
    }
}

static void overlay_draw_divider(SDL_Surface *surface, int x, int y, int w)
{
    SDL_Rect divider = {x, y, w, 1};
    SDL_FillRect(surface, &divider, SDL_MapRGBA(surface->format, 255, 255, 255, 36));
}

static int overlay_effective_rows(const BenchOverlay *snap)
{
    const int total = OVERLAY_HEADER_ROW_SPAN + snap->configured_row_count +
                      OVERLAY_CHART_ROW_SPAN + OVERLAY_BACKENDS_ROW_SPAN +
                      snap->configured_keybind_count;
    return SDL_max(total, OVERLAY_MIN_EFFECTIVE_ROWS);
}

static void overlay_render_header(SDL_Surface *surface, TTF_Font *font, int panel_w, int row_height)
{
    BenchDriverStatus status;
    bench_driver_get_status(&status);

    const SDL_Rect bounds = {OVERLAY_EDGE_PAD, 0, panel_w - 2 * OVERLAY_EDGE_PAD, row_height};
    const SDL_Color text_color = {243, 241, 234, 255};

    char battery_text[32];
    if (status.battery_percent >= 0) {
        snprintf(battery_text, sizeof(battery_text), "%d%%", status.battery_percent);
    } else {
        snprintf(battery_text, sizeof(battery_text), "n/a");
    }
    overlay_draw_text_line(surface, font, bounds, 0, text_color, battery_text);

    if (status.charging) {
        const int glyph_size = SDL_max(row_height - OVERLAY_FONT_SIZE_MARGIN, 4);
        const SDL_Color bolt_color = {244, 211, 94, 255};
        overlay_draw_charge_glyph(surface, OVERLAY_EDGE_PAD + 34, (row_height - glyph_size) / 2,
                                  glyph_size, bolt_color);
    }

    const time_t now = time(NULL);
    struct tm tm_now;
    localtime_r(&now, &tm_now);
    char clock_text[16];
    strftime(clock_text, sizeof(clock_text), "%H:%M:%S", &tm_now);
    overlay_draw_text_line(surface, font, bounds, 2, text_color, clock_text);
}

static int overlay_render_backends_block(SDL_Renderer *renderer, SDL_Surface *surface,
                                         TTF_Font *font, int panel_w, int y, int row_height)
{
    overlay_draw_divider(surface, OVERLAY_EDGE_PAD, y, panel_w - 2 * OVERLAY_EDGE_PAD);
    y += row_height / 4;

    const int text_w = panel_w - 2 * OVERLAY_EDGE_PAD;
    const SDL_Color label_color = {201, 198, 188, 255};
    const SDL_Color value_color = {243, 241, 234, 255};
    char line[96];

    SDL_RendererInfo info;
    const char *render_name = (renderer && SDL_GetRendererInfo(renderer, &info) == 0) ? info.name : "unknown";
    snprintf(line, sizeof(line), "Render  %s", render_name);
    overlay_draw_text_line(surface, font, (SDL_Rect){OVERLAY_EDGE_PAD, y, text_w, row_height}, 0, value_color, line);
    y += row_height;

    const char *audio_driver = SDL_GetCurrentAudioDriver();
    snprintf(line, sizeof(line), "Audio   %s", audio_driver ? audio_driver : "off");
    overlay_draw_text_line(surface, font, (SDL_Rect){OVERLAY_EDGE_PAD, y, text_w, row_height}, 0, value_color, line);
    y += row_height;

    BenchDriverStatus status;
    bench_driver_get_status(&status);
    if (status.joystick_attached) {
        snprintf(line, sizeof(line), "Input   Joystick (%s)", status.joystick_name);
    } else {
        snprintf(line, sizeof(line), "Input   Keyboard");
    }
    overlay_draw_text_line(surface, font, (SDL_Rect){OVERLAY_EDGE_PAD, y, text_w, row_height}, 0, value_color, line);
    y += row_height;

    const char *power_label = "Unknown";
    if (status.power_info_valid) {
        switch (status.power_state) {
            case SDL_POWERSTATE_ON_BATTERY: power_label = "Battery"; break;
            case SDL_POWERSTATE_CHARGING:   power_label = "Charging"; break;
            case SDL_POWERSTATE_CHARGED:    power_label = "Charged"; break;
            case SDL_POWERSTATE_NO_BATTERY: power_label = "No battery"; break;
            default: break;
        }
    }
    snprintf(line, sizeof(line), "Power   %s", power_label);
    overlay_draw_text_line(surface, font, (SDL_Rect){OVERLAY_EDGE_PAD, y, text_w, row_height}, 0, value_color, line);
    y += row_height;

    snprintf(line, sizeof(line), "Threads %u", (unsigned)bench_backend_probe_thread_count());
    overlay_draw_text_line(surface, font, (SDL_Rect){OVERLAY_EDGE_PAD, y, text_w, row_height}, 0, value_color, line);
    y += row_height;

    snprintf(line, sizeof(line), "Display %dx%d %s", status.display_w, status.display_h,
             status.vsync_verified_active ? "vsync" : "no-vsync");
    overlay_draw_text_line(surface, font, (SDL_Rect){OVERLAY_EDGE_PAD, y, text_w, row_height}, 0, value_color, line);
    y += row_height;

    SDL_version v;
    SDL_GetVersion(&v);
#ifdef DEBUG_BUILD
    const char *build_tag = "DEBUG";
#else
    const char *build_tag = "RELEASE";
#endif
    snprintf(line, sizeof(line), "Build   SDL %d.%d.%d %s", v.major, v.minor, v.patch, build_tag);
    overlay_draw_text_line(surface, font, (SDL_Rect){OVERLAY_EDGE_PAD, y, text_w, row_height}, 0, value_color, line);
    y += row_height;

    const Uint32 cpu_freq = bench_backend_probe_cpu_freq_mhz();
    if (cpu_freq > 0) {
        snprintf(line, sizeof(line), "CPU Freq %u MHz", (unsigned)cpu_freq);
        overlay_draw_text_line(surface, font, (SDL_Rect){OVERLAY_EDGE_PAD, y, text_w, row_height}, 0, label_color, line);
    }
    y += row_height;

    return y;
}

static SDL_Rect overlay_chart_rect(int panel_w, int y, int row_height, int index)
{
    const SDL_Rect rect = {OVERLAY_EDGE_PAD, y + index * (2 * row_height),
                           panel_w - 2 * OVERLAY_EDGE_PAD, 2 * row_height};
    return rect;
}

static void overlay_render_charts(const BenchOverlay *snap, SDL_Surface *surface, TTF_Font *font,
                                  int panel_w, int charts_y, int row_height)
{
    const SDL_Rect fps_rect = overlay_chart_rect(panel_w, charts_y, row_height, 0);
    const SDL_Rect frame_rect = overlay_chart_rect(panel_w, charts_y, row_height, 1);

    const SDL_Color fps_color = {139, 224, 139, 255};
    const SDL_Color frame_color = {111, 211, 255, 255};

    rolling_chart_render(&snap->fps_chart, surface, fps_rect, fps_color);
    rolling_chart_render(&snap->frametime_chart, surface, frame_rect, frame_color);

    char fps_text[32];
    snprintf(fps_text, sizeof(fps_text), "%.1f FPS", snap->latest_metrics.current_fps);
    overlay_draw_text_line(surface, font, (SDL_Rect){fps_rect.x, fps_rect.y, fps_rect.w, row_height},
                           0, fps_color, fps_text);

    char frame_text[32];
    snprintf(frame_text, sizeof(frame_text), "%.2f ms", snap->latest_metrics.frame_time_ms);
    overlay_draw_text_line(surface, font, (SDL_Rect){frame_rect.x, frame_rect.y, frame_rect.w, row_height},
                           0, frame_color, frame_text);
}

static void overlay_render_new_panel(const BenchOverlay *snap, SDL_Surface *surface,
                                     TTF_Font *font, int row_height)
{
    SDL_FillRect(surface, NULL,
                 SDL_MapRGBA(surface->format, OVERLAY_PANEL_BG_R, OVERLAY_PANEL_BG_G,
                            OVERLAY_PANEL_BG_B, OVERLAY_PANEL_BG_A));

    const int panel_w = snap->width;
    int y = 0;

    overlay_render_header(surface, font, panel_w, row_height);
    y += row_height;

    overlay_render_charts(snap, surface, font, panel_w, y, row_height);
    y += OVERLAY_CHART_ROW_SPAN * row_height;

    y = overlay_rows_render_data(snap, surface, font, panel_w, y, row_height);

    y = overlay_render_backends_block(snap->renderer, surface, font, panel_w, y, row_height);

    overlay_rows_render_keybinds(snap, surface, font, panel_w, y, row_height);
}

static void overlay_render_collapsed(const BenchOverlay *snap, SDL_Surface *surface,
                                     TTF_Font *font, int row_height)
{
    const int panel_w = snap->width;
    const SDL_Rect bg_rect = {0, 0, panel_w, row_height + OVERLAY_CHART_ROW_SPAN * row_height};
    SDL_FillRect(surface, &bg_rect,
                 SDL_MapRGBA(surface->format, OVERLAY_PANEL_BG_R, OVERLAY_PANEL_BG_G,
                            OVERLAY_PANEL_BG_B, OVERLAY_PANEL_BG_A));
    overlay_render_charts(snap, surface, font, panel_w, row_height, row_height);
}

static void overlay_render_legacy(const BenchOverlay *snap, SDL_Surface *surface, TTF_Font *font)
{
    if (!font) {
        return;
    }

    const int line_adv = snap->line_height;
    const int column_width = snap->width / 2;
    const int divider_x = column_width;
    const int status_band_height = snap->line_height * 2;
    int left_y = status_band_height + 4;
    int right_y = status_band_height + 4;

    if (snap->has_status) {
        const int cell_w = snap->width / BENCH_STATUS_GRID_COLS;
        const int cell_h = snap->line_height;
        const int cell_pad = 4;

        for (int cell = 0; cell < BENCH_STATUS_GRID_CELLS; ++cell) {
            if (snap->status_fields[cell][0] == '\0') {
                continue;
            }
            const int col = cell % BENCH_STATUS_GRID_COLS;
            const int row = cell / BENCH_STATUS_GRID_COLS;
            const SDL_Rect cell_rect = {col * cell_w + cell_pad, row * cell_h, cell_w - cell_pad, cell_h};
            overlay_draw_text_line(surface, font, cell_rect, 0, snap->status_color, snap->status_fields[cell]);
        }

        SDL_Rect status_divider = {4, status_band_height, snap->width - 8, 1};
        SDL_FillRect(surface, &status_divider, SDL_MapRGBA(surface->format, 60, 80, 120, 140));
    }

    for (int i = 0; i < snap->line_count; ++i) {
        if (snap->pending_lines[i].text[0] == '\0') {
            if (snap->pending_lines[i].column == 1) {
                right_y += line_adv;
            } else {
                left_y += line_adv;
            }
            continue;
        }

        const int x_offset = (snap->pending_lines[i].column == 1) ? (divider_x + 8) : 8;
        const int y_pos = (snap->pending_lines[i].column == 1) ? right_y : left_y;
        const int available_width = column_width - 16;
        const SDL_Rect bounds = {x_offset, y_pos, available_width, line_adv};
        overlay_draw_text_line(surface, font, bounds, snap->pending_lines[i].alignment,
                               snap->pending_lines[i].color, snap->pending_lines[i].text);

        if (snap->pending_lines[i].column == 1) {
            right_y += line_adv;
        } else {
            left_y += line_adv;
        }
    }

    SDL_Rect divider = {divider_x - 1, status_band_height + 4, 2, snap->height - status_band_height - 8};
    SDL_FillRect(surface, &divider, SDL_MapRGBA(surface->format, 60, 80, 120, 180));
}

static int bench_overlay_thread(void *userdata)
{
    BenchOverlayThreadArgs *args = (BenchOverlayThreadArgs *)userdata;
    BenchOverlay *overlay = args->overlay;
    const int legacy_font_size = args->font_size;
    free(args);

    TTF_Font *font = NULL;
    int font_size = 0;
    SDL_Surface *surface = NULL;
    int surface_w = 0;
    int surface_h = 0;

    while (overlay->running) {
        SDL_LockMutex(overlay->mutex);
        while (!overlay->dirty && overlay->running) {
            SDL_CondWait(overlay->cond, overlay->mutex);
        }
        if (!overlay->running) {
            SDL_UnlockMutex(overlay->mutex);
            break;
        }

        BenchOverlay snap = *overlay;
        overlay->dirty = SDL_FALSE;
        SDL_UnlockMutex(overlay->mutex);

        if (surface_w != snap.width || surface_h != snap.height) {
            if (surface) {
                SDL_FreeSurface(surface);
            }
            surface = SDL_CreateRGBSurfaceWithFormat(0, snap.width, snap.height, 32, SDL_PIXELFORMAT_RGBA32);
            surface_w = snap.width;
            surface_h = snap.height;
        }
        if (!surface) {
            continue;
        }

        int desired_font_size;
        int row_height = snap.line_height;
        if (snap.row_registry_configured) {
            const int effective_rows = overlay_effective_rows(&snap);
            row_height = snap.height / effective_rows;
            desired_font_size = SDL_max(row_height - OVERLAY_FONT_SIZE_MARGIN, 6);
        } else {
            desired_font_size = legacy_font_size;
        }
        if (desired_font_size != font_size || !font) {
            if (font) {
                TTF_CloseFont(font);
            }
            font = bench_load_font(desired_font_size);
            font_size = desired_font_size;
        }

        SDL_FillRect(surface, NULL, SDL_MapRGBA(surface->format, 0, 0, 0, 0));

        if (snap.row_registry_configured) {
            if (snap.collapsed) {
                overlay_render_collapsed(&snap, surface, font, row_height);
            } else {
                overlay_render_new_panel(&snap, surface, font, row_height);
            }
        } else {
            SDL_FillRect(surface, NULL,
                         SDL_MapRGBA(surface->format, snap.background.r, snap.background.g,
                                    snap.background.b, snap.background.a));
            overlay_render_legacy(&snap, surface, font);
        }

        SDL_LockMutex(overlay->mutex);
        const size_t bytes = (size_t)surface->pitch * (size_t)surface->h;
        if (overlay->buffer_bytes != bytes) {
            SDL_free(overlay->pixel_buffer);
            SDL_free(overlay->visible_buffer);
            overlay->pixel_buffer = (Uint8 *)SDL_malloc(bytes);
            overlay->visible_buffer = (Uint8 *)SDL_malloc(bytes);
            overlay->buffer_bytes = bytes;
        }
        overlay->pitch = surface->pitch;
        if (overlay->pixel_buffer && overlay->visible_buffer) {
            rs_memcpy(overlay->pixel_buffer, surface->pixels, bytes);
            overlay->has_pixels = SDL_TRUE;
        }
        overlay->width = snap.width;
        overlay->height = snap.height;
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
    overlay->height = overlay->line_height * overlay->max_rows + overlay->line_height * 2;
    overlay->background = (SDL_Color){0, 0, 0, 255};
    overlay->running = SDL_TRUE;

    rolling_chart_init(&overlay->fps_chart);
    rolling_chart_init(&overlay->frametime_chart);

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
        rs_memcpy(overlay->pending_lines,
                  lines,
                  (size_t)overlay->line_count * sizeof(BenchOverlayLine));
    }
    overlay->background = background;
    overlay->dirty = SDL_TRUE;
    SDL_CondSignal(overlay->cond);
    SDL_UnlockMutex(overlay->mutex);
}

void bench_overlay_set_status_grid(BenchOverlay *overlay,
                                   const char fields[BENCH_STATUS_GRID_CELLS][BENCH_STATUS_FIELD_LEN],
                                   SDL_Color color)
{
    if (!overlay) {
        return;
    }

    SDL_LockMutex(overlay->mutex);
    SDL_bool any_set = SDL_FALSE;
    SDL_bool changed = SDL_FALSE;
    for (int i = 0; i < BENCH_STATUS_GRID_CELLS; ++i) {
        const char *field = fields ? fields[i] : "";
        if (field[0] != '\0') {
            any_set = SDL_TRUE;
        }
        if (strncmp(overlay->status_fields[i], field, BENCH_STATUS_FIELD_LEN - 1) != 0) {
            changed = SDL_TRUE;
        }
        strncpy(overlay->status_fields[i], field, BENCH_STATUS_FIELD_LEN - 1);
        overlay->status_fields[i][BENCH_STATUS_FIELD_LEN - 1] = '\0';
    }
    overlay->has_status = any_set;
    overlay->status_color = color;
    if (changed) {
        overlay->dirty = SDL_TRUE;
        SDL_CondSignal(overlay->cond);
    }
    SDL_UnlockMutex(overlay->mutex);
}

void bench_overlay_toggle_collapsed(BenchOverlay *overlay)
{
    if (!overlay) {
        return;
    }
    SDL_LockMutex(overlay->mutex);
    overlay->collapsed = !overlay->collapsed;
    overlay->dirty = SDL_TRUE;
    SDL_CondSignal(overlay->cond);
    SDL_UnlockMutex(overlay->mutex);
}

void bench_overlay_present(BenchOverlay *overlay,
                           SDL_Renderer *renderer,
                           BenchMetrics *metrics,
                           int x,
                           int y)
{
    (void)metrics;
    if (!overlay || !renderer) {
        return;
    }

    Uint8 *pixels = NULL;
    int pitch = 0;
    int tex_w;
    int tex_h;

    SDL_LockMutex(overlay->mutex);
    if (overlay->has_pixels && overlay->pixel_buffer && overlay->visible_buffer) {
        if (overlay->buffer_bytes > 0) {
            rs_memcpy(overlay->visible_buffer,
                      overlay->pixel_buffer,
                      overlay->buffer_bytes);
            pixels = overlay->visible_buffer;
            pitch = overlay->pitch;
        }
        overlay->has_pixels = SDL_FALSE;
    }
    tex_w = overlay->width;
    tex_h = overlay->height;
    SDL_UnlockMutex(overlay->mutex);

    if (pixels) {
        int cur_tex_w = 0, cur_tex_h = 0;
        if (overlay->texture) {
            SDL_QueryTexture(overlay->texture, NULL, NULL, &cur_tex_w, &cur_tex_h);
        }
        if (!overlay->texture || renderer != overlay->renderer ||
            cur_tex_w != tex_w || cur_tex_h != tex_h) {
            bench_overlay_free_texture_locked(overlay);
            overlay->texture = SDL_CreateTexture(renderer,
                                                 SDL_PIXELFORMAT_RGBA8888,
                                                 SDL_TEXTUREACCESS_STREAMING,
                                                 tex_w,
                                                 tex_h);
            if (overlay->texture) {
                SDL_SetTextureBlendMode(overlay->texture, SDL_BLENDMODE_BLEND);
            }
            overlay->renderer = renderer;
        }

        if (overlay->texture) {
            const int used_pitch = (pitch > 0) ? pitch : tex_w * 4;
            SDL_UpdateTexture(overlay->texture,
                              NULL,
                              pixels,
                              used_pitch);
        }
    }

    if (overlay->texture) {
        SDL_Rect dst = {x, y, tex_w, tex_h};
        SDL_RenderCopy(renderer, overlay->texture, NULL, &dst);
    }
}

int bench_overlay_height(const BenchOverlay *overlay)
{
    if (!overlay) {
        return 0;
    }
    return overlay->height;
}
