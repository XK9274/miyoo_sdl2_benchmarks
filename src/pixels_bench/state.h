#ifndef PIXELS_BENCH_STATE_H
#define PIXELS_BENCH_STATE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "bench_common.h"
#include "common/bench_stress.h"

typedef enum {
    PIXELS_BLEND_ALPHA = 0,
    PIXELS_BLEND_ADDITIVE,
    PIXELS_BLEND_COLORKEY,
    PIXELS_BLEND_MAX
} PixelsBlendMode;

typedef enum {
    PIXELS_UPLOAD_LOCK = 0,
    PIXELS_UPLOAD_UPDATE,
    PIXELS_UPLOAD_MAX
} PixelsUploadPath;

typedef struct {
    int stress_level;
    float top_margin;
    TTF_Font *font;
    BenchSinTable sin_table;

    int buffer_width;
    int buffer_height;

    SDL_Texture *pixel_texture;
    void *pixel_buffer;
    float pixel_phase;

    Uint8 *cellular_cells;
    Uint8 *cellular_new_cells;
    SDL_bool cellular_seeded;

    int forced_mode;                 /* -1 = auto-cycle, else locked mode index */
    int current_mode;
    SDL_bool neon_copy_enabled;
    PixelsBlendMode blend_mode;
    PixelsUploadPath upload_path;
} PixelsBenchState;

void pixels_state_init(PixelsBenchState *state);
void pixels_state_update_layout(PixelsBenchState *state, BenchOverlay *overlay);
void pixels_state_destroy(PixelsBenchState *state, SDL_Renderer *renderer);

float pixels_state_stress_factor(const PixelsBenchState *state);
float pixels_state_sin(const PixelsBenchState *state, float units);
float pixels_state_cos(const PixelsBenchState *state, float units);
float pixels_state_sin_rad(const PixelsBenchState *state, float radians);
float pixels_state_cos_rad(const PixelsBenchState *state, float radians);

#endif /* PIXELS_BENCH_STATE_H */
