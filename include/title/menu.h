#ifndef TITLE_MENU_H
#define TITLE_MENU_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "title/launcher.h"
#include "title/state.h"

void title_menu_render(TitleContext *ctx, const TitleState *state);

#endif /* TITLE_MENU_H */
