#ifndef TITLE_BATTERY_GLOW_H
#define TITLE_BATTERY_GLOW_H

#include <SDL2/SDL.h>

#include "common/gl_effect.h"

/* Soft radial-falloff sprite rendered once via GL; per-frame draw is a plain tinted blit, no GL cost. */
typedef struct {
    GLEffectTarget target;
    Uint32 program;
    SDL_bool ready;
} TitleBatteryGlow;

void title_battery_glow_init(TitleBatteryGlow *glow, SDL_Renderer *renderer);
void title_battery_glow_shutdown(TitleBatteryGlow *glow);

/* Draws a pulsing, additively-blended glow of the given color, centered at (center_x, center_y). */
void title_battery_glow_render(TitleBatteryGlow *glow, SDL_Renderer *renderer,
                               int center_x, int center_y, int diameter, SDL_Color color);

#endif /* TITLE_BATTERY_GLOW_H */
