#ifndef TITLE_LAUNCHER_H
#define TITLE_LAUNCHER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "title/state.h"

/* sdl2_title's own SDL context. Torn down before, and rebuilt after, every
 * child suite launch -- the MMIYOO driver's video/render/joystick/haptic
 * backends are singleton, single-process resources. */
typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    TTF_Font *title_font;
    TTF_Font *ui_font;
} TitleContext;

SDL_bool title_context_init(TitleContext *ctx);
void title_context_shutdown(TitleContext *ctx);

typedef struct {
    int exit_code;
    SDL_bool crashed;
    int signal_number;
    SDL_bool exec_failed;
} TitleLaunchResult;

/* Config the title screen exposes to a launched suite. Suite-specific
 * controls (RS_FORCE_SCENE, RS_BENCH_DURATION_S, stress levels, etc.) are
 * NOT touched here -- suites keep their own controls. */
SDL_bool title_launch_suite(const TitleState *state,
                            const char *bin_name,
                            TitleContext *ctx,
                            TitleLaunchResult *out_result);

#endif /* TITLE_LAUNCHER_H */
