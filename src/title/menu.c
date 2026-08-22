#include "title/menu.h"

#include "title/config_panel.h"

#define TITLE_LIST_X 40
#define TITLE_LIST_TOP 70
#define TITLE_ROW_HEIGHT 26

#define TITLE_CONFIG_X 340
#define TITLE_CONFIG_TOP 70
#define TITLE_CONFIG_ROW_HEIGHT 34

static void title_draw_text(SDL_Renderer *renderer, TTF_Font *font, const char *text,
                            int x, int y, SDL_Color color, SDL_bool center)
{
    if (!font || !text || !text[0]) {
        return;
    }

    SDL_Surface *surface = TTF_RenderUTF8_Blended(font, text, color);
    if (!surface) {
        return;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture) {
        SDL_Rect dst = {center ? x - surface->w / 2 : x, y, surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, NULL, &dst);
        SDL_DestroyTexture(texture);
    }
    SDL_FreeSurface(surface);
}

static void title_draw_row_highlight(SDL_Renderer *renderer, int x, int y, int w, int h)
{
    SDL_SetRenderDrawColor(renderer, 50, 90, 160, 255);
    SDL_Rect rect = {x, y, w, h};
    SDL_RenderFillRect(renderer, &rect);
}

void title_menu_render(SDL_Renderer *renderer, TTF_Font *title_font, TTF_Font *ui_font, const TitleState *state)
{
    if (!renderer || !state) {
        return;
    }

    SDL_SetRenderDrawColor(renderer, 10, 12, 20, 255);
    SDL_RenderClear(renderer);

    const SDL_Color white = {235, 235, 240, 255};
    const SDL_Color dim = {150, 155, 170, 255};
    const SDL_Color highlight_text = {255, 255, 255, 255};
    const SDL_Color error_color = {255, 90, 90, 255};

    title_draw_text(renderer, title_font, "SDL2 Demo Suites", BENCH_NATIVE_W / 2, 12, white, SDL_TRUE);

    /* Suite list */
    for (int i = 0; i < TITLE_SUITE_COUNT; i++) {
        const int row_y = TITLE_LIST_TOP + i * TITLE_ROW_HEIGHT;
        const SDL_bool selected = (state->focus == TITLE_FOCUS_LIST) && (state->selected_suite == i);
        if (selected) {
            title_draw_row_highlight(renderer, TITLE_LIST_X - 8, row_y - 4, 260, TITLE_ROW_HEIGHT);
        }
        title_draw_text(renderer, ui_font, state->suites[i].label, TITLE_LIST_X, row_y,
                        selected ? highlight_text : white, SDL_FALSE);
    }

    /* Config panel */
    for (int row = 0; row < TITLE_CONFIG_COUNT; row++) {
        const int row_y = TITLE_CONFIG_TOP + row * TITLE_CONFIG_ROW_HEIGHT;
        const SDL_bool selected = (state->focus == TITLE_FOCUS_CONFIG) && (state->config_row == row);
        if (selected) {
            title_draw_row_highlight(renderer, TITLE_CONFIG_X - 8, row_y - 4, 240, TITLE_CONFIG_ROW_HEIGHT - 6);
        }

        char value[64];
        title_config_row_value_text(state, (TitleConfigRow)row, value, sizeof(value));

        title_draw_text(renderer, ui_font, title_config_row_label((TitleConfigRow)row),
                        TITLE_CONFIG_X, row_y, selected ? highlight_text : dim, SDL_FALSE);

        char value_line[80];
        SDL_snprintf(value_line, sizeof(value_line), selected ? "< %s >" : "%s", value);
        title_draw_text(renderer, ui_font, value_line, TITLE_CONFIG_X, row_y + 16,
                        selected ? highlight_text : white, SDL_FALSE);
    }

    if (state->mode == TITLE_MODE_CHILD_ERROR) {
        SDL_SetRenderDrawColor(renderer, 60, 15, 15, 230);
        SDL_Rect banner = {20, BENCH_NATIVE_H - 60, BENCH_NATIVE_W - 40, 36};
        SDL_RenderFillRect(renderer, &banner);
        title_draw_text(renderer, ui_font, state->error_message, banner.x + 12, banner.y + 8, error_color, SDL_FALSE);
    } else {
        title_draw_text(renderer, ui_font,
                        "Up/Down: Navigate  L1/R1: Switch Panel  Left/Right: Change  A: Launch  Exit: Quit",
                        BENCH_NATIVE_W / 2, BENCH_NATIVE_H - 24, dim, SDL_TRUE);
    }

    SDL_RenderPresent(renderer);
}
