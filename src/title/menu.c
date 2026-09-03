#include "title/menu.h"

#include "title/config_panel.h"
#include "title/modal.h"
#include "title/render_util.h"
#include "title/statusbar.h"
#include "title/version.h"

#define TITLE_LIST_X 40
#define TITLE_LIST_VISIBLE_ROWS 9
#define TITLE_ROW_HEIGHT 20

#define TITLE_CONFIG_X 340
#define TITLE_CONFIG_ROW_HEIGHT 22

static void title_draw_row_highlight(SDL_Renderer *renderer, int x, int y, int w, int h, SDL_Color color)
{
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_Rect rect = {x, y, w, h};
    SDL_RenderFillRect(renderer, &rect);
}

/* Highlight box centered on text's vertical midpoint, padded by pad_v above/below. */
static void title_draw_row_highlight_for_text(SDL_Renderer *renderer, TTF_Font *font, const char *text,
                                              int x, int row_y, int w, int pad_v, SDL_Color color)
{
    int text_h = 0;
    TTF_SizeUTF8(font, (text && text[0]) ? text : " ", NULL, &text_h);
    title_draw_row_highlight(renderer, x, row_y - pad_v, w, text_h + pad_v * 2, color);
}

/* Smoothed FPS from the wall-clock gap between renders. */
static float title_track_fps(void)
{
    static Uint32 last_ticks = 0;
    static float smoothed_fps = 0.0f;

    const Uint32 now = SDL_GetTicks();
    if (last_ticks != 0) {
        const Uint32 delta = now - last_ticks;
        if (delta > 0) {
            const float instant_fps = 1000.0f / (float)delta;
            smoothed_fps = (smoothed_fps <= 0.0f) ? instant_fps : (smoothed_fps * 0.9f + instant_fps * 0.1f);
        }
    }
    last_ticks = now;
    return smoothed_fps;
}

void title_menu_render(TitleContext *ctx, const TitleState *state)
{
    if (!ctx || !ctx->renderer || !state) {
        return;
    }

    SDL_Renderer *renderer = ctx->renderer;
    TTF_Font *title_font = ctx->title_font;
    TTF_Font *ui_font = ctx->ui_font;
    TTF_Font *accent_font = ctx->accent_font ? ctx->accent_font : ui_font;

    SDL_SetRenderDrawColor(renderer, 0x17, 0x17, 0x17, 255);
    SDL_RenderClear(renderer);
    if (ctx->background) {
        SDL_RenderCopy(renderer, ctx->background, NULL, NULL);
    }
    title_fireflies_render(renderer, &ctx->fireflies);

    const SDL_Color white = {235, 235, 240, 255};
    const SDL_Color highlight_text = {255, 255, 255, 255};
    const SDL_Color quit_color = {235, 150, 150, 255};
    const SDL_Color error_color = {255, 90, 90, 255};
    const SDL_Color highlight_focus = {50, 90, 160, 255};
    const SDL_Color highlight_editing = {170, 110, 30, 255};
    const SDL_Color disabled_color = {115, 118, 126, 255};

    char fps_text[16];
    SDL_snprintf(fps_text, sizeof(fps_text), "%.0f FPS", title_track_fps());

    title_statusbar_render_header(renderer, accent_font, title_font, accent_font,
                                  &ctx->battery_fill, &ctx->battery_glow, &ctx->backend, fps_text, "SDL2 Demo Suites");
    title_statusbar_render_status_header(renderer, ctx->small_font, &ctx->backend,
                                         TITLE_STATUSBAR_HEADER_HEIGHT - 6, TITLE_VERSION_STRING);

    /* Panels are centered as a matched-height pair around the screen's vertical midpoint. */
    const int box_pad_top = 16;
    const int box_pad_bottom = 8;

    const int list_content_h = TITLE_LIST_VISIBLE_ROWS * TITLE_ROW_HEIGHT;
    const int config_content_h = TITLE_CONFIG_COUNT * TITLE_CONFIG_ROW_HEIGHT;
    const int shared_content_h = SDL_max(list_content_h, config_content_h);
    const int box_h = shared_content_h + box_pad_top + box_pad_bottom;
    const int box_y = BENCH_NATIVE_H / 2 - box_h / 2;

    const int list_top = box_y + box_pad_top;
    const int config_top = box_y + box_pad_top + (shared_content_h - config_content_h) / 2;
    const int footer_top = BENCH_NATIVE_H - TITLE_STATUSBAR_FOOTER_HEIGHT;

    const TitleCategory *category = &state->categories[state->selected_category];
    const SDL_bool is_quit_category = title_state_quit_selected(state);

    char list_title[64];
    if (category->entry_count > TITLE_LIST_VISIBLE_ROWS) {
        SDL_snprintf(list_title, sizeof(list_title), "< %s (%d/%d) >",
                     category->label, state->selected_entry + 1, category->entry_count);
    } else {
        SDL_snprintf(list_title, sizeof(list_title), "< %s >", category->label);
    }

    const SDL_Rect list_box = {TITLE_LIST_X - 16, box_y, 276, box_h};
    const SDL_Rect config_box = {TITLE_CONFIG_X - 16, box_y, 256, box_h};
    title_draw_panel_frame(renderer, ui_font, list_title, list_box, state->focus == TITLE_FOCUS_LIST);
    title_draw_panel_frame(renderer, ui_font, "Config", config_box, state->focus == TITLE_FOCUS_CONFIG);

    /* Keep the selected entry visible with the smallest possible scroll. */
    int scroll = state->selected_entry - TITLE_LIST_VISIBLE_ROWS + 1;
    if (scroll < 0) {
        scroll = 0;
    }
    const int max_scroll = category->entry_count - TITLE_LIST_VISIBLE_ROWS;
    if (max_scroll > 0 && scroll > max_scroll) {
        scroll = max_scroll;
    } else if (max_scroll <= 0) {
        scroll = 0;
    }

    const int visible_count = SDL_min(TITLE_LIST_VISIBLE_ROWS, category->entry_count - scroll);
    for (int row = 0; row < visible_count; row++) {
        const int i = scroll + row;
        const int row_y = list_top + row * TITLE_ROW_HEIGHT;
        const SDL_bool selected = (state->focus == TITLE_FOCUS_LIST) && (state->selected_entry == i);
        const char *label = category->entries[i].label;

        if (selected) {
            title_draw_row_highlight_for_text(renderer, ui_font, label, TITLE_LIST_X - 8, row_y, 260, 4, highlight_focus);
        }

        SDL_Color color = white;
        if (selected) {
            color = highlight_text;
        } else if (is_quit_category) {
            color = quit_color;
        }
        title_draw_text(renderer, ui_font, label, TITLE_LIST_X, row_y, color, SDL_FALSE);
    }

    /* "Label: value", wrapped in <> only while actively editing that row. */
    for (int row = 0; row < TITLE_CONFIG_COUNT; row++) {
        const int row_y = config_top + row * TITLE_CONFIG_ROW_HEIGHT;
        const SDL_bool selected = (state->focus == TITLE_FOCUS_CONFIG) && (state->config_row == row);
        const SDL_bool disabled = title_config_row_disabled((TitleConfigRow)row);
        const SDL_bool editing = selected && state->editing && !disabled;

        char value[64];
        title_config_row_value_text(state, (TitleConfigRow)row, value, sizeof(value));

        char row_line[112];
        SDL_snprintf(row_line, sizeof(row_line), editing ? "%s: < %s >" : "%s: %s",
                    title_config_row_label((TitleConfigRow)row), value);

        if (selected) {
            title_draw_row_highlight_for_text(renderer, ui_font, row_line, TITLE_CONFIG_X - 8, row_y, 240, 4,
                                              editing ? highlight_editing : highlight_focus);
        }

        SDL_Color row_color = white;
        if (disabled) {
            row_color = disabled_color;
        } else if (selected) {
            row_color = highlight_text;
        }
        title_draw_text(renderer, ui_font, row_line, TITLE_CONFIG_X, row_y, row_color, SDL_FALSE);
    }

    if (state->mode == TITLE_MODE_CHILD_ERROR) {
        SDL_SetRenderDrawColor(renderer, 60, 15, 15, 230);
        SDL_Rect banner = {20, footer_top - 44, BENCH_NATIVE_W - 40, 36};
        SDL_RenderFillRect(renderer, &banner);
        title_draw_text(renderer, ui_font, state->error_message, banner.x + 12, banner.y + 8, error_color, SDL_FALSE);
    }

    title_statusbar_render_footer(renderer, ui_font, ctx->small_font, &ctx->backend, state);

    if (state->mode == TITLE_MODE_INFO_MODAL) {
        const TitleSuiteEntry *entry = &state->categories[state->info_modal_category].entries[state->info_modal_entry];
        title_draw_modal(renderer, accent_font, ui_font, entry->label, entry->info);
    }

    SDL_RenderPresent(renderer);
}
