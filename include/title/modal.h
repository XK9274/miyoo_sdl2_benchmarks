#ifndef TITLE_MODAL_H
#define TITLE_MODAL_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

/* Full-screen 50%-opaque dim, then a box sized to fit title+body text, centered on screen. */
void title_draw_modal(SDL_Renderer *renderer, TTF_Font *title_font, TTF_Font *body_font,
                      const char *title, const char *body);

#endif /* TITLE_MODAL_H */
