#ifndef TITLE_STATUSBAR_H
#define TITLE_STATUSBAR_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "title/backend_status.h"
#include "title/battery_fill.h"
#include "title/battery_glow.h"
#include "title/state.h"

#define TITLE_STATUSBAR_FOOTER_HEIGHT 34
#define TITLE_STATUSBAR_HEADER_HEIGHT 46

typedef struct {
    const char *text;
    SDL_Color color;
} TitleStatusSegment;

/* Run of segments centered as a block at center_x, joined by separator in sep_color. */
void title_draw_segmented_line(SDL_Renderer *renderer, TTF_Font *font,
                               const TitleStatusSegment *segments, int count,
                               int center_x, int y,
                               const char *separator, SDL_Color sep_color);

/* Small square LED: lighter outline, darker green/red center. */
void title_draw_led(SDL_Renderer *renderer, int x, int y, int size, SDL_bool ok);

/* ui_font sizes the keybind legend; small_font sizes the info line and LED row. */
void title_statusbar_render_footer(SDL_Renderer *renderer, TTF_Font *ui_font, TTF_Font *small_font,
                                   const TitleBackendStatus *backend, const TitleState *state);

/* Header bar: FPS (left), title (centered), clock+battery+percent (right) -- one aligned row. */
void title_statusbar_render_header(SDL_Renderer *renderer,
                                   TTF_Font *left_font, TTF_Font *title_font, TTF_Font *accent_font,
                                   TitleBatteryFill *battery_fill, TitleBatteryGlow *battery_glow,
                                   const TitleBackendStatus *backend,
                                   const char *left_text, const char *title_text);

/* Tagline (SDL/platform info), backend LEDs, then the version string -- stacked below top_y. */
void title_statusbar_render_status_header(SDL_Renderer *renderer, TTF_Font *small_font,
                                          const TitleBackendStatus *backend, int top_y,
                                          const char *version_text);

#endif /* TITLE_STATUSBAR_H */
