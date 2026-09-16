#ifndef COMMON_LOADING_SCREEN_H
#define COMMON_LOADING_SCREEN_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

typedef enum {
    BENCH_LOADING_STYLE_RECT = 0,
    BENCH_LOADING_STYLE_SHIP = 1
} BenchLoadingStyle;

typedef struct BenchLoadingScreen {
    SDL_Window *window;
    SDL_Renderer *renderer;
    BenchLoadingStyle style;
    SDL_bool active;
    float progress;
    Uint64 start_ticks;
    Uint64 last_ticks;
    Uint64 perf_freq;
    Uint64 last_counter;
    char message[96];

    SDL_Color background;
    SDL_Color bar_outline;
    SDL_Color bar_fill;
    SDL_Color text_color;

    TTF_Font *font;
    SDL_bool owns_font;

    float ship_angle; /* BENCH_LOADING_STYLE_SHIP: current Y-axis spin, radians */

    /* BENCH_LOADING_STYLE_SHIP only: renders on its own thread so the ship
       keeps spinning continuously instead of only advancing when the
       caller's loading work happens to call bench_loading_step. Guards
       progress/message, which the caller's thread also writes. */
    SDL_Thread *render_thread;
    SDL_mutex *state_mutex;
    SDL_bool render_thread_running;
} BenchLoadingScreen;

SDL_bool bench_loading_begin(BenchLoadingScreen *screen,
                             SDL_Window *window,
                             SDL_Renderer *renderer,
                             BenchLoadingStyle style);

/* Overrides the default text/bar-fill colors set by bench_loading_begin.
   Safe to call any time after bench_loading_begin; takes effect on the
   next present. */
void bench_loading_set_colors(BenchLoadingScreen *screen,
                              SDL_Color text_color,
                              SDL_Color bar_fill_color);

void bench_loading_step(BenchLoadingScreen *screen,
                        float progress,
                        const char *label);

void bench_loading_mark_idle(BenchLoadingScreen *screen,
                             const char *label);

void bench_loading_finish(BenchLoadingScreen *screen);

void bench_loading_abort(BenchLoadingScreen *screen);

#endif /* COMMON_LOADING_SCREEN_H */
