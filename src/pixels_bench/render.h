#ifndef PIXELS_BENCH_RENDER_H
#define PIXELS_BENCH_RENDER_H

#include "pixels_bench/state.h"

typedef enum {
    PIXEL_MODE_PLASMA = 0,
    PIXEL_MODE_MANDELBROT,
    PIXEL_MODE_CELLULAR,
    PIXEL_MODE_NOISE,
    PIXEL_MODE_DITHER,
    PIXEL_MODE_PALETTE,
    PIXEL_MODE_MAX
} PixelMode;

void pixels_render_init(PixelsBenchState *state, SDL_Renderer *renderer);
void pixels_render_cleanup(PixelsBenchState *state);

void pixels_render_scene(PixelsBenchState *state,
                         SDL_Renderer *renderer,
                         BenchMetrics *metrics,
                         double delta_seconds);

const char *pixels_render_mode_name(int mode);
const char *pixels_render_blend_name(PixelsBlendMode mode);
const char *pixels_render_upload_name(PixelsUploadPath path);

#endif /* PIXELS_BENCH_RENDER_H */
