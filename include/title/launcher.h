#ifndef TITLE_LAUNCHER_H
#define TITLE_LAUNCHER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "title/backend_status.h"
#include "title/battery_fill.h"
#include "title/battery_glow.h"
#include "title/fireflies.h"
#include "title/state.h"

/* sdl2_title's own SDL context; torn down before and rebuilt after every child suite launch. */
typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    TTF_Font *title_font;
    TTF_Font *ui_font;
    TTF_Font *small_font; /* footer's backend info line, LED labels, clock */
    TTF_Font *accent_font; /* clock, battery percentage, version stamp */
    TitleBackendStatus backend;
    TitleBatteryFill battery_fill;
    TitleBatteryGlow battery_glow;
    TitleFireflies fireflies;
    SDL_Texture *background;
} TitleContext;

SDL_bool title_context_init(TitleContext *ctx);
void title_context_shutdown(TitleContext *ctx);

/* Resolves the directory sdl2_title lives in, so sibling files (suite binaries, assets/) can be found. */
SDL_bool title_get_bin_dir(char *out_dir, size_t out_size);

typedef struct {
    int exit_code;
    SDL_bool crashed;
    int signal_number;
    SDL_bool exec_failed;
} TitleLaunchResult;

/* Bridges shared SDL context config to the launched suite; suite-specific controls are untouched. */
SDL_bool title_launch_suite(const TitleState *state,
                            const char *bin_name,
                            TitleContext *ctx,
                            TitleLaunchResult *out_result);

#endif /* TITLE_LAUNCHER_H */
