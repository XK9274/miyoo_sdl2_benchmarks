#ifndef PIXELS_BENCH_STATE_H
#define PIXELS_BENCH_STATE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "bench_common.h"

typedef struct {
    int stress_level;
    float top_margin;
    TTF_Font *font;

    SDL_Texture *pixel_texture;
    SDL_Surface *pixel_surface;
    void *pixel_buffer;
    float pixel_phase;
    int pixel_plasma_offset;
} PixelsBenchState;

void pixels_state_init(PixelsBenchState *state);
void pixels_state_update_layout(PixelsBenchState *state, BenchOverlay *overlay);
void pixels_state_destroy(PixelsBenchState *state, SDL_Renderer *renderer);

float pixels_state_stress_factor(const PixelsBenchState *state);

#endif /* PIXELS_BENCH_STATE_H */
