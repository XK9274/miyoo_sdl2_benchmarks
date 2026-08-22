#ifndef TITLE_RENDER_UTIL_H
#define TITLE_RENDER_UTIL_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

/* Blits text at (x, y); x is the center if center is SDL_TRUE, else the left edge. */
void title_draw_text(SDL_Renderer *renderer, TTF_Font *font, const char *text,
                     int x, int y, SDL_Color color, SDL_bool center);

/* Same as title_draw_text, but the glyph is baked once (white) and tinted via texture
 * color/alpha mod each call -- cheap even when tint changes every frame (e.g. a pulse). */
void title_draw_text_tinted(SDL_Renderer *renderer, TTF_Font *font, const char *text,
                            int x, int y, SDL_Color tint, SDL_bool center);

/* Alpha-blended fill via a 1x1 texture blit -- SDL_RenderFillRect ignores alpha on this renderer.
 * TODO: fix at the source in the sdl2_miyoo driver's QuickFill path; this is an app-side workaround. */
void title_draw_dim_rect(SDL_Renderer *renderer, SDL_Rect rect, SDL_Color color);

/* Groupbox-style outline with a caption label cut into the border, dimming what's behind it.
 * When active, the border and label pulse brighter to mark this as the currently focused panel. */
void title_draw_panel_frame(SDL_Renderer *renderer, TTF_Font *font, const char *label, SDL_Rect box,
                            SDL_bool active);

#define TITLE_WRAP_MAX_LINES 8
#define TITLE_WRAP_LINE_LEN 128

/* Greedy word-wrap of text into lines no wider than max_width under font; returns the line count. */
int title_wrap_text(TTF_Font *font, const char *text, int max_width,
                    char lines[][TITLE_WRAP_LINE_LEN], int max_lines);

#endif /* TITLE_RENDER_UTIL_H */
