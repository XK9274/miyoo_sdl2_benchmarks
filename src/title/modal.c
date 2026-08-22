#include "title/modal.h"

#include "common/types.h"
#include "title/render_util.h"

#define TITLE_MODAL_MAX_BODY_WIDTH 460
#define TITLE_MODAL_PAD_X 20
#define TITLE_MODAL_PAD_Y 16
#define TITLE_MODAL_TITLE_GAP 10
#define TITLE_MODAL_LINE_GAP 4

void title_draw_modal(SDL_Renderer *renderer, TTF_Font *title_font, TTF_Font *body_font,
                      const char *title, const char *body)
{
    if (!renderer) {
        return;
    }

    /* 50%-opaque full-screen dim behind the box. */
    const SDL_Rect full_screen = {0, 0, BENCH_NATIVE_W, BENCH_NATIVE_H};
    const SDL_Color dim = {0, 0, 0, 128};
    title_draw_dim_rect(renderer, full_screen, dim);

    char lines[TITLE_WRAP_MAX_LINES][TITLE_WRAP_LINE_LEN];
    const int line_count = title_wrap_text(body_font, body, TITLE_MODAL_MAX_BODY_WIDTH,
                                           lines, TITLE_WRAP_MAX_LINES);

    int title_w = 0, title_h = 0;
    TTF_SizeUTF8(title_font, title, &title_w, &title_h);

    int line_w_max = 0, line_h = 0;
    for (int i = 0; i < line_count; i++) {
        int w = 0;
        TTF_SizeUTF8(body_font, lines[i], &w, &line_h);
        if (w > line_w_max) {
            line_w_max = w;
        }
    }

    const int content_w = SDL_max(title_w, line_w_max);
    const int box_w = content_w + TITLE_MODAL_PAD_X * 2;
    const int box_h = TITLE_MODAL_PAD_Y * 2 + title_h + TITLE_MODAL_TITLE_GAP
                     + line_count * (line_h + TITLE_MODAL_LINE_GAP);

    const SDL_Rect box = {(BENCH_NATIVE_W - box_w) / 2, (BENCH_NATIVE_H - box_h) / 2, box_w, box_h};

    const SDL_Color box_bg = {28, 30, 36, 235};
    const SDL_Color border = {90, 95, 108, 255};
    const SDL_Color title_color = {235, 235, 240, 255};
    const SDL_Color body_color = {200, 204, 214, 255};

    title_draw_dim_rect(renderer, box, box_bg);

    SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, border.a);
    SDL_RenderDrawRect(renderer, &box);

    const int center_x = box.x + box_w / 2;
    int y = box.y + TITLE_MODAL_PAD_Y;
    title_draw_text(renderer, title_font, title, center_x, y, title_color, SDL_TRUE);
    y += title_h + TITLE_MODAL_TITLE_GAP;

    for (int i = 0; i < line_count; i++) {
        title_draw_text(renderer, body_font, lines[i], center_x, y, body_color, SDL_TRUE);
        y += line_h + TITLE_MODAL_LINE_GAP;
    }
}
