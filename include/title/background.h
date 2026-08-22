#ifndef TITLE_BACKGROUND_H
#define TITLE_BACKGROUND_H

#include <SDL2/SDL.h>

/* Loads assets/title_bg.bmp into a texture; NULL on failure (caller falls back to a flat clear). */
SDL_Texture *title_background_load(SDL_Renderer *renderer);

#endif /* TITLE_BACKGROUND_H */
