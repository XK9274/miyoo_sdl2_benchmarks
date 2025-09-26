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
void bench_overlay_present(BenchOverlay *overlay,
                           SDL_Renderer *renderer,
                           BenchMetrics *metrics,
                           int x,
                           int y);
int bench_overlay_height(const BenchOverlay *overlay);

// Old overlay builder functions removed - use overlay_grid.h instead

#endif /* COMMON_OVERLAY_H */
