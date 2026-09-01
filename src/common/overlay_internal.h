#ifndef COMMON_OVERLAY_INTERNAL_H
#define COMMON_OVERLAY_INTERNAL_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "common/overlay.h"
#include "common/overlay_debug_stats.h"
#include "common/overlay_rows.h"
#include "common/rolling_chart.h"
#include "common/types.h"

#define OVERLAY_EDGE_PAD 8

/* Full BenchOverlay layout, shared across the render thread and the row
 * registry's configure/update entry points -- not a public header. */
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

    char status_fields[BENCH_STATUS_GRID_CELLS][BENCH_STATUS_FIELD_LEN];
    SDL_Color status_color;
    SDL_bool has_status;

    /* Set once bench_overlay_configure is called; selects the new left-third
     * panel render path over the legacy two-column band above. */
    SDL_bool row_registry_configured;
    OverlayRowSpec configured_rows[OVERLAY_ROWS_MAX];
    int configured_row_count;
    OverlayKeybind configured_keybinds[OVERLAY_KEYBINDS_MAX];
    int configured_keybind_count;

    BenchMetrics latest_metrics;
    SDL_bool has_metrics;
    char custom_values[OVERLAY_ROWS_MAX][OVERLAY_CUSTOM_VALUE_LEN];
    int custom_value_count;
    OverlayDebugStats debug_stats;

    SDL_bool collapsed;
    RollingChart fps_chart;
    RollingChart frametime_chart;

    Uint8 *pixel_buffer;
    Uint8 *visible_buffer;
    size_t buffer_bytes;
    int pitch;
};

/* Shared aligned/clipped text blit used by every panel section. */
void overlay_draw_text_line(SDL_Surface *surface, TTF_Font *font, SDL_Rect bounds,
                            int alignment, SDL_Color color, const char *text);

/* Renders the suite-configured data rows / keybinds sections of the panel.
 * Both return the y offset immediately after the section they drew. */
int overlay_rows_render_data(const BenchOverlay *snap, SDL_Surface *surface, TTF_Font *font,
                             int panel_w, int y, int row_height);
int overlay_rows_render_keybinds(const BenchOverlay *snap, SDL_Surface *surface, TTF_Font *font,
                                 int panel_w, int y, int row_height);

#endif /* COMMON_OVERLAY_INTERNAL_H */
