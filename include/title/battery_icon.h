#ifndef TITLE_BATTERY_ICON_H
#define TITLE_BATTERY_ICON_H

#include <SDL2/SDL.h>

#include "title/battery_fill.h"
#include "title/battery_glow.h"

/* Total on-screen width (body + nub) an icon of this height will occupy. */
int title_battery_icon_width(int height);

/* Vector battery outline + nub, with an animated GL fill and a charging-only ambient glow. */
void title_draw_battery_icon(SDL_Renderer *renderer, TitleBatteryFill *fill, TitleBatteryGlow *glow,
                             int x, int y, int height, int percent, SDL_bool charging);

#endif /* TITLE_BATTERY_ICON_H */
