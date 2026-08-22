#include "common/display_config.h"

#include <stdlib.h>

#include "common/types.h"

static int g_logical_w = BENCH_NATIVE_W;
static int g_logical_h = BENCH_NATIVE_H;

void bench_display_config_load(int *out_w, int *out_h)
{
    int w = BENCH_NATIVE_W;
    int h = BENCH_NATIVE_H;

    const char *w_str = SDL_getenv(BENCH_ENV_LOGICAL_W);
    const char *h_str = SDL_getenv(BENCH_ENV_LOGICAL_H);
    if (w_str && h_str) {
        int parsed_w = SDL_atoi(w_str);
        int parsed_h = SDL_atoi(h_str);
        if (parsed_w > 0 && parsed_h > 0) {
            w = parsed_w;
            h = parsed_h;
        }
    }

    if (out_w) {
        *out_w = w;
    }
    if (out_h) {
        *out_h = h;
    }
}

void bench_display_config_apply(SDL_Renderer *renderer, int w, int h)
{
    if (w <= 0 || h <= 0) {
        w = BENCH_NATIVE_W;
        h = BENCH_NATIVE_H;
    }
    g_logical_w = w;
    g_logical_h = h;
    if (renderer) {
        SDL_RenderSetLogicalSize(renderer, w, h);
    }
}

int bench_logical_w(void)
{
    return g_logical_w;
}

int bench_logical_h(void)
{
    return g_logical_h;
}
