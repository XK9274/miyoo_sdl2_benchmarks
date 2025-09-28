#ifndef COMMON_LOADING_SCREEN_H
#define COMMON_LOADING_SCREEN_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

typedef enum {
    BENCH_LOADING_STYLE_RECT = 0,
    BENCH_LOADING_STYLE_GL = 1
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

    SDL_Texture *gl_stage_texture;
    Uint8 *gl_pixels;
    size_t gl_capacity;
    int gl_width;
    int gl_height;
    float gl_time_accum;
    SDL_bool gl_ready;
    SDL_bool gl_library_loaded;
    SDL_bool gl_library_owned;
    SDL_bool gl_transferred;
    SDL_Window *gl_window;
    SDL_GLContext gl_context;
    Uint32 gl_vbo;
    Uint32 gl_ibo;
    Uint32 gl_program;
    Uint32 gl_fbo;
    Uint32 gl_color_texture;
    int gl_uniform_time;
    int gl_uniform_progress;
} BenchLoadingScreen;

SDL_bool bench_loading_begin(BenchLoadingScreen *screen,
                             SDL_Window *window,
                             SDL_Renderer *renderer,
                             BenchLoadingStyle style);

void bench_loading_step(BenchLoadingScreen *screen,
                        float progress,
                        const char *label);

void bench_loading_mark_idle(BenchLoadingScreen *screen,
                             const char *label);

void bench_loading_finish(BenchLoadingScreen *screen);

void bench_loading_abort(BenchLoadingScreen *screen);

SDL_bool bench_loading_obtain_gl(BenchLoadingScreen *screen,
                                 SDL_Window **out_window,
                                 SDL_GLContext *out_context);

#endif /* COMMON_LOADING_SCREEN_H */
