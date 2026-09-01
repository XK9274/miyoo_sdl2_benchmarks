#ifndef COMMON_OVERLAY_H
#define COMMON_OVERLAY_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "common/types.h"

typedef struct BenchOverlay BenchOverlay;

TTF_Font *bench_load_font(int size);

BenchOverlay *bench_overlay_create(SDL_Renderer *renderer,
                                   int width,
                                   int line_height,
                                   int max_rows);
void bench_overlay_destroy(BenchOverlay *overlay);
void bench_overlay_request_stop(BenchOverlay *overlay);

/* Toggles the collapsed (top-left FPS/frame-time-only) view for overlays
 * configured via bench_overlay_configure (common/overlay_rows.h). */
void bench_overlay_toggle_collapsed(BenchOverlay *overlay);

void bench_overlay_submit(BenchOverlay *overlay,
                          const BenchOverlayLine *lines,
                          int line_count,
                          SDL_Color background);

#define BENCH_STATUS_GRID_COLS 4
#define BENCH_STATUS_GRID_ROWS 2
#define BENCH_STATUS_GRID_CELLS (BENCH_STATUS_GRID_COLS * BENCH_STATUS_GRID_ROWS)
#define BENCH_STATUS_FIELD_LEN 40

/* Sets the full-width status strip above the metrics/controls grid, laid out as a row-major BENCH_STATUS_GRID_ROWS x BENCH_STATUS_GRID_COLS grid, each cell independently clipped. Pass "" for unused cells. */
void bench_overlay_set_status_grid(BenchOverlay *overlay,
                                   const char fields[BENCH_STATUS_GRID_CELLS][BENCH_STATUS_FIELD_LEN],
                                   SDL_Color color);
void bench_overlay_present(BenchOverlay *overlay,
                           SDL_Renderer *renderer,
                           BenchMetrics *metrics,
                           int x,
                           int y);
int bench_overlay_height(const BenchOverlay *overlay);

#endif /* COMMON_OVERLAY_H */
