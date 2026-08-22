#include "title/battery_fill.h"

#include "title/render_util.h"

#define TITLE_BATTERY_FILL_W 32
#define TITLE_BATTERY_FILL_H 16

void title_battery_fill_init(TitleBatteryFill *fill, SDL_Renderer *renderer)
{
    if (!fill) {
        return;
    }
    SDL_zerop(fill);
    if (!renderer) {
        return;
    }

    const int w = TITLE_BATTERY_FILL_W;
    const int h = TITLE_BATTERY_FILL_H;
    Uint8 *pixels = (Uint8 *)SDL_malloc((size_t)w * h * 4);
    if (!pixels) {
        return;
    }

    /* Plain white brightness gradient; SDL_SetTextureColorMod tints it at draw time. */
    for (int y = 0; y < h; y++) {
        const float t = (h > 1) ? (float)y / (float)(h - 1) : 0.0f;
        const float factor = 1.0f - t * 0.35f; /* brighter at top, darker at bottom */
        const Uint8 v = (Uint8)(factor * 255.0f);
        for (int x = 0; x < w; x++) {
            Uint8 *p = pixels + (y * w + x) * 4;
            p[0] = v;
            p[1] = v;
            p[2] = v;
            p[3] = 255;
        }
    }

    fill->pattern = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, w, h);
    if (fill->pattern) {
        SDL_UpdateTexture(fill->pattern, NULL, pixels, w * 4);
        SDL_SetTextureBlendMode(fill->pattern, SDL_BLENDMODE_BLEND);
        fill->pattern_w = w;
        fill->pattern_h = h;
        fill->ready = SDL_TRUE;
    }

    SDL_free(pixels);
}

void title_battery_fill_shutdown(TitleBatteryFill *fill)
{
    if (!fill || !fill->pattern) {
        return;
    }
    SDL_DestroyTexture(fill->pattern);
    fill->pattern = NULL;
    fill->ready = SDL_FALSE;
}

void title_battery_fill_render(TitleBatteryFill *fill, SDL_Renderer *renderer,
                               SDL_Rect dst, int percent, SDL_Color color, SDL_bool charging)
{
    if (!fill || !fill->ready || !renderer) {
        return;
    }
    if (percent < 0) {
        percent = 0;
    } else if (percent > 100) {
        percent = 100;
    }

    const int fill_w = (dst.w * percent) / 100;
    if (fill_w <= 0) {
        return;
    }

    const float time = SDL_GetTicks() / 1000.0f;
    const float pulse = 0.85f + 0.15f * SDL_sinf(time * 2.0f);

    SDL_SetTextureColorMod(fill->pattern, color.r, color.g, color.b);
    SDL_SetTextureAlphaMod(fill->pattern, (Uint8)(pulse * 255.0f));

    const SDL_Rect fill_rect = {dst.x, dst.y, fill_w, dst.h};
    SDL_RenderCopy(renderer, fill->pattern, NULL, &fill_rect);

    if (charging && fill_w > 4) {
        /* Subtle left-to-right (ping-pong) highlight band, same hue as the fill. */
        const int band_w = SDL_max(3, fill_w / 5);
        const float period = SDL_fmodf(time * 0.6f, 2.0f);
        const float progress = (period <= 1.0f) ? period : (2.0f - period);
        const int band_x = fill_rect.x + (int)(progress * (float)(fill_w - band_w));

        const SDL_Rect band_rect = {band_x, fill_rect.y, band_w, fill_rect.h};
        const SDL_Color band_color = {
            (Uint8)SDL_min(255, color.r + 60), (Uint8)SDL_min(255, color.g + 40),
            (Uint8)SDL_min(255, color.b + 60), 110,
        };
        title_draw_dim_rect(renderer, band_rect, band_color);
    }
}
