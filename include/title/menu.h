#ifndef TITLE_MENU_H
#define TITLE_MENU_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "title/state.h"

void title_menu_render(SDL_Renderer *renderer,
                       TTF_Font *title_font,
                       TTF_Font *ui_font,
                       const TitleState *state);

#endif /* TITLE_MENU_H */
