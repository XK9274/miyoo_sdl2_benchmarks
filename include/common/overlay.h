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

void bench_overlay_submit(BenchOverlay *overlay,
                          const BenchOverlayLine *lines,
                          int line_count,
                          SDL_Color background);

#define BENCH_STATUS_GRID_COLS 4
#define BENCH_STATUS_GRID_ROWS 2
#define BENCH_STATUS_GRID_CELLS (BENCH_STATUS_GRID_COLS * BENCH_STATUS_GRID_ROWS)
#define BENCH_STATUS_FIELD_LEN 40

/* Sets the full-width status strip drawn above the metrics/controls grid,
 * e.g. driver/system status from driver_support.h, laid out as a fixed
 * BENCH_STATUS_GRID_ROWS x BENCH_STATUS_GRID_COLS grid (row-major, cell 0 is
 * top-left). Each cell is independently clipped to its own bounds -- text
 * that doesn't fit is truncated at the cell edge, never drawn into a
 * neighboring cell. Pass an empty string ("") for unused cells. */
void bench_overlay_set_status_grid(BenchOverlay *overlay,
                                   const char fields[BENCH_STATUS_GRID_CELLS][BENCH_STATUS_FIELD_LEN],
                                   SDL_Color color);
void bench_overlay_present(BenchOverlay *overlay,
                           SDL_Renderer *renderer,
                           BenchMetrics *metrics,
                           int x,
                           int y);
int bench_overlay_height(const BenchOverlay *overlay);

// Old overlay builder functions removed - use overlay_grid.h instead

#endif /* COMMON_OVERLAY_H */
