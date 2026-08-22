#ifndef COMMON_DISPLAY_CONFIG_H
#define COMMON_DISPLAY_CONFIG_H

#include <SDL2/SDL.h>

/* Set by sdl2_title before exec'ing a suite; read once at suite startup. */
#define BENCH_ENV_LOGICAL_W "BENCH_LOGICAL_W"
#define BENCH_ENV_LOGICAL_H "BENCH_LOGICAL_H"

/* Reads BENCH_LOGICAL_W/H; falls back to BENCH_NATIVE_W/H if unset or invalid. */
void bench_display_config_load(int *out_w, int *out_h);

/* Applies SDL_RenderSetLogicalSize(renderer, w, h) and caches w/h for the accessors below. */
void bench_display_config_apply(SDL_Renderer *renderer, int w, int h);

int bench_logical_w(void);
int bench_logical_h(void);

#endif /* COMMON_DISPLAY_CONFIG_H */
