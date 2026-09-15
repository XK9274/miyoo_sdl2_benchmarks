#ifndef TITLE_FIREFLIES_H
#define TITLE_FIREFLIES_H

#include <SDL2/SDL.h>

#include "common/gl_effect.h"

#define TITLE_FIREFLY_COUNT 32

typedef struct {
    float x, y;
    float vx, vy;
    float phase;
    float hue_mix;
    SDL_bool far; /* half the flies: half draw size, half brightness -- reads as further away */

    float charge_mix;   /* 0 = normal palette, 1 = full charge colour */
    float pop_delay;    /* ripple stagger after a charging edge, seconds */
    float pop_elapsed;  /* seconds since the last charging edge; <0 = no pop in progress */
    SDL_bool fading;    /* easing charge_mix back to 0 after unplug */
} TitleFirefly;

typedef struct {
    GLEffectTarget target;
    Uint32 program;
    SDL_bool ready;
    SDL_bool was_charging;
    TitleFirefly flies[TITLE_FIREFLY_COUNT];
} TitleFireflies;

void title_fireflies_init(TitleFireflies *fx, SDL_Renderer *renderer);
void title_fireflies_shutdown(TitleFireflies *fx);

/* Advances positions by dt seconds; wraps around the screen edges with a small offscreen margin. */
void title_fireflies_update(TitleFireflies *fx, float dt);

/* Renders this frame's fireflies and composites them full-screen. */
void title_fireflies_render(SDL_Renderer *renderer, TitleFireflies *fx);

#endif /* TITLE_FIREFLIES_H */
