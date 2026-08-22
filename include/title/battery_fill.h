#ifndef TITLE_BATTERY_FILL_H
#define TITLE_BATTERY_FILL_H

#include <SDL2/SDL.h>

/* Animated battery fill from a small pre-rendered texture + SDL geometry -- no GL. */
typedef struct {
    SDL_Texture *pattern;
    int pattern_w, pattern_h;
    SDL_bool ready;
} TitleBatteryFill;

/* Builds the gradient texture once (pure CPU pixel buffer, no GL). */
void title_battery_fill_init(TitleBatteryFill *fill, SDL_Renderer *renderer);
void title_battery_fill_shutdown(TitleBatteryFill *fill);

/* Stretches the proportional fill into dst, tinted by color, with a gentle pulse; sweeps a highlight band when charging. */
void title_battery_fill_render(TitleBatteryFill *fill, SDL_Renderer *renderer,
                               SDL_Rect dst, int percent, SDL_Color color, SDL_bool charging);

#endif /* TITLE_BATTERY_FILL_H */
