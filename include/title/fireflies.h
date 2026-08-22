#ifndef TITLE_FIREFLIES_H
#define TITLE_FIREFLIES_H

#include <SDL2/SDL.h>

#include "common/gl_effect.h"

#define TITLE_FIREFLY_COUNT 8

typedef struct {
    float x, y;
    float vx, vy;
    float phase;
    float hue_mix;
} TitleFirefly;

typedef struct {
    GLEffectTarget target;
    Uint32 program;
    SDL_bool ready;
    TitleFirefly flies[TITLE_FIREFLY_COUNT];
} TitleFireflies;

void title_fireflies_init(TitleFireflies *fx, SDL_Renderer *renderer);
void title_fireflies_shutdown(TitleFireflies *fx);

/* Advances positions by dt seconds; wraps around the screen edges with a small offscreen margin. */
void title_fireflies_update(TitleFireflies *fx, float dt);

/* Renders this frame's fireflies and composites them full-screen. */
void title_fireflies_render(SDL_Renderer *renderer, TitleFireflies *fx);

#endif /* TITLE_FIREFLIES_H */
