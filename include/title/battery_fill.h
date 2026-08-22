#ifndef TITLE_BATTERY_FILL_H
#define TITLE_BATTERY_FILL_H

#include <SDL2/SDL.h>

#include "common/gl_effect.h"

/* Animated GLES2 fill shader (gradient + wavy edge + scrolling shimmer), rendered every frame. */
typedef struct {
    GLEffectTarget target;
    Uint32 program;
    SDL_bool ready;
} TitleBatteryFill;

/* Compiles the fill shader + offscreen target; ready stays SDL_FALSE if GL is unavailable. */
void title_battery_fill_init(TitleBatteryFill *fill, SDL_Renderer *renderer);
void title_battery_fill_shutdown(TitleBatteryFill *fill);

/* Renders the proportional fill into dst via GL, tinted by color; charging speeds up the shimmer. */
void title_battery_fill_render(TitleBatteryFill *fill, SDL_Renderer *renderer,
                               SDL_Rect dst, int percent, SDL_Color color, SDL_bool charging);

#endif /* TITLE_BATTERY_FILL_H */
