#include "title/battery_icon.h"

int title_battery_icon_width(int height)
{
    if (height <= 0) {
        return 0;
    }
    const int body_w = (height * 9) / 5; /* 1.8x */
    const int nub_w = SDL_max(2, height / 5);
    return body_w + nub_w + 1;
}

void title_draw_battery_icon(SDL_Renderer *renderer, TitleBatteryFill *fill, TitleBatteryGlow *glow,
                             int x, int y, int height, int percent, SDL_bool charging)
{
    if (!renderer || height <= 0) {
        return;
    }
    if (percent < 0) {
        percent = 0;
    } else if (percent > 100) {
        percent = 100;
    }

    const int body_w = (height * 9) / 5;
    const int nub_w = SDL_max(2, height / 5);
    const int nub_h = SDL_max(2, height / 2);

    const SDL_Color outline = {170, 174, 184, 255};
    const SDL_Color bg = {40, 42, 48, 255};
    const SDL_Color fill_charging = {225, 190, 40, 255}; /* yellow */
    const SDL_Color fill_ok = {60, 170, 90, 255};
    const SDL_Color fill_low = {190, 70, 60, 255};
    const SDL_Color fill_color = charging ? fill_charging : ((percent <= 15) ? fill_low : fill_ok);

    if (charging && glow) {
        const int total_w = body_w + nub_w + 1;
        const int diameter = (int)(total_w * 2.2f);
        title_battery_glow_render(glow, renderer, x + total_w / 2, y + height / 2, diameter, fill_color);
    }

    const SDL_Rect body = {x, y, body_w, height};
    SDL_SetRenderDrawColor(renderer, outline.r, outline.g, outline.b, outline.a);
    SDL_RenderDrawRect(renderer, &body);

    const SDL_Rect nub = {x + body_w + 1, y + (height - nub_h) / 2, nub_w, nub_h};
    SDL_RenderFillRect(renderer, &nub);

    const int pad = 2;
    const SDL_Rect inner_bg = {body.x + pad, body.y + pad,
                               SDL_max(0, body_w - pad * 2), SDL_max(0, height - pad * 2)};
    SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, bg.a);
    SDL_RenderFillRect(renderer, &inner_bg);

    if (fill && fill->ready) {
        title_battery_fill_render(fill, renderer, inner_bg, percent, fill_color, charging);
        return;
    }

    /* Fill texture unavailable -- flat fill fallback. */
    const int fill_w = (inner_bg.w * percent) / 100;
    if (fill_w > 0) {
        const SDL_Rect fill_rect = {inner_bg.x, inner_bg.y, fill_w, inner_bg.h};
        SDL_SetRenderDrawColor(renderer, fill_color.r, fill_color.g, fill_color.b, fill_color.a);
        SDL_RenderFillRect(renderer, &fill_rect);
    }
}
