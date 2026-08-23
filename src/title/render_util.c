#include "title/render_util.h"

#include <stdio.h>
#include <string.h>

/* Text-texture cache, keyed by (font, color, text); LRU eviction when full. */
#define TITLE_TEXT_CACHE_SIZE 64
#define TITLE_TEXT_CACHE_TEXT_LEN 160

typedef struct {
    SDL_bool used;
    TTF_Font *font;
    SDL_Color color;
    char text[TITLE_TEXT_CACHE_TEXT_LEN];
    SDL_Texture *texture;
    int w, h;
    Uint32 last_used_ticks;
} TitleTextCacheEntry;

static TitleTextCacheEntry g_text_cache[TITLE_TEXT_CACHE_SIZE];
static SDL_Renderer *g_text_cache_owner = NULL;

static SDL_bool title_color_eq(SDL_Color a, SDL_Color b)
{
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

static void title_text_cache_reset(void)
{
    /* Renderer changed (suite-launch context reinit) -- textures already gone with it. */
    for (int i = 0; i < TITLE_TEXT_CACHE_SIZE; i++) {
        g_text_cache[i].used = SDL_FALSE;
        g_text_cache[i].texture = NULL;
    }
}

/* Looks up (or bakes and caches) the glyph texture for (font, bake_color, text). */
static TitleTextCacheEntry *title_text_cache_get(SDL_Renderer *renderer, TTF_Font *font,
                                                  const char *text, SDL_Color bake_color)
{
    if (g_text_cache_owner != renderer) {
        title_text_cache_reset();
        g_text_cache_owner = renderer;
    }

    const Uint32 now = SDL_GetTicks();
    TitleTextCacheEntry *slot = NULL;
    TitleTextCacheEntry *empty = NULL;
    TitleTextCacheEntry *oldest = &g_text_cache[0];

    for (int i = 0; i < TITLE_TEXT_CACHE_SIZE; i++) {
        TitleTextCacheEntry *e = &g_text_cache[i];
        if (e->used && e->font == font && title_color_eq(e->color, bake_color) && strcmp(e->text, text) == 0) {
            slot = e;
            break;
        }
        if (!e->used) {
            if (!empty) {
                empty = e;
            }
        } else if (e->last_used_ticks < oldest->last_used_ticks) {
            oldest = e;
        }
    }

    if (!slot) {
        TitleTextCacheEntry *lru = empty ? empty : oldest;
        SDL_Surface *surface = TTF_RenderUTF8_Blended(font, text, bake_color);
        if (!surface) {
            return NULL;
        }
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
        const int w = surface->w;
        const int h = surface->h;
        SDL_FreeSurface(surface);
        if (!texture) {
            return NULL;
        }
        /* TTF_RenderUTF8_Blended supplies per-pixel alpha; preserve it on copy. */
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

        if (lru->texture) {
            SDL_DestroyTexture(lru->texture);
        }
        lru->used = SDL_TRUE;
        lru->font = font;
        lru->color = bake_color;
        strncpy(lru->text, text, sizeof(lru->text) - 1);
        lru->text[sizeof(lru->text) - 1] = '\0';
        lru->texture = texture;
        lru->w = w;
        lru->h = h;
        slot = lru;
    }

    slot->last_used_ticks = now;
    return slot;
}

void title_draw_text(SDL_Renderer *renderer, TTF_Font *font, const char *text,
                     int x, int y, SDL_Color color, SDL_bool center)
{
    if (!renderer || !font || !text || !text[0]) {
        return;
    }

    TitleTextCacheEntry *slot = title_text_cache_get(renderer, font, text, color);
    if (!slot) {
        return;
    }

    const SDL_Rect dst = {center ? x - slot->w / 2 : x, y, slot->w, slot->h};
    SDL_RenderCopy(renderer, slot->texture, NULL, &dst);
}

/* Same as title_draw_text, but bakes the glyph in white and tints it via texture color mod each call. */
void title_draw_text_tinted(SDL_Renderer *renderer, TTF_Font *font, const char *text,
                            int x, int y, SDL_Color tint, SDL_bool center)
{
    if (!renderer || !font || !text || !text[0]) {
        return;
    }

    const SDL_Color bake_white = {255, 255, 255, 255};
    TitleTextCacheEntry *slot = title_text_cache_get(renderer, font, text, bake_white);
    if (!slot) {
        return;
    }

    SDL_SetTextureColorMod(slot->texture, tint.r, tint.g, tint.b);
    SDL_SetTextureAlphaMod(slot->texture, tint.a);
    const SDL_Rect dst = {center ? x - slot->w / 2 : x, y, slot->w, slot->h};
    SDL_RenderCopy(renderer, slot->texture, NULL, &dst);
}

static SDL_Texture *title_get_dim_texture(SDL_Renderer *renderer)
{
    static SDL_Texture *tex = NULL;
    static SDL_Renderer *owner = NULL;

    if (!tex || owner != renderer) {
        /* Renderer changed (suite-launch context reinit) -- old texture already gone with it. */
        tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, 1, 1);
        if (tex) {
            Uint32 pixel = 0xFFFFFFFFu;
            SDL_UpdateTexture(tex, NULL, &pixel, sizeof(pixel));
            SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
        }
        owner = renderer;
    }
    return tex;
}

void title_draw_dim_rect(SDL_Renderer *renderer, SDL_Rect rect, SDL_Color color)
{
    SDL_Texture *tex = title_get_dim_texture(renderer);
    if (!tex) {
        return;
    }
    SDL_SetTextureColorMod(tex, color.r, color.g, color.b);
    SDL_SetTextureAlphaMod(tex, color.a);
    SDL_RenderCopy(renderer, tex, NULL, &rect);
}

static Uint8 title_lerp_u8(Uint8 a, Uint8 b, float t)
{
    return (Uint8)(a + (b - a) * t);
}

void title_draw_panel_frame(SDL_Renderer *renderer, TTF_Font *font, const char *label, SDL_Rect box,
                            SDL_bool active)
{
    if (!renderer) {
        return;
    }

    const SDL_Color border = {70, 74, 84, 255};
    const SDL_Color border_active = {130, 175, 255, 255};
    const SDL_Color label_color = {150, 155, 168, 255};
    const SDL_Color label_active = {225, 235, 255, 255};
    const SDL_Color dim = {0, 0, 0, 100};

    float pulse = 0.0f;
    if (active) {
        const float time = SDL_GetTicks() / 1000.0f;
        pulse = 0.4f + 0.6f * (0.5f + 0.5f * SDL_sinf(time * 3.0f));
    }

    const SDL_Color border_draw = active
        ? (SDL_Color){title_lerp_u8(border.r, border_active.r, pulse),
                      title_lerp_u8(border.g, border_active.g, pulse),
                      title_lerp_u8(border.b, border_active.b, pulse), 255}
        : border;

    /* Same continuous pulse as the border, cheap since the label is tinted rather than re-rasterized. */
    const SDL_Color label_draw = active
        ? (SDL_Color){title_lerp_u8(label_color.r, label_active.r, pulse),
                      title_lerp_u8(label_color.g, label_active.g, pulse),
                      title_lerp_u8(label_color.b, label_active.b, pulse), 255}
        : label_color;

    title_draw_dim_rect(renderer, box, dim);

    int label_w = 0, label_h = 0;
    if (font && label && label[0]) {
        TTF_SizeUTF8(font, label, &label_w, &label_h);
    }

    const int label_x = box.x + 10;
    const int gap_pad = 4;
    const int right = box.x + box.w - 1;
    const int bottom = box.y + box.h - 1;

    SDL_SetRenderDrawColor(renderer, border_draw.r, border_draw.g, border_draw.b, border_draw.a);

    if (label_w > 0) {
        const int gap_start = SDL_clamp(label_x - gap_pad, box.x, right);
        const int gap_end = SDL_clamp(label_x + label_w + gap_pad, box.x, right);
        SDL_RenderDrawLine(renderer, box.x, box.y, gap_start, box.y);
        SDL_RenderDrawLine(renderer, gap_end, box.y, right, box.y);
    } else {
        SDL_RenderDrawLine(renderer, box.x, box.y, right, box.y);
    }
    SDL_RenderDrawLine(renderer, box.x, box.y, box.x, bottom);
    SDL_RenderDrawLine(renderer, right, box.y, right, bottom);
    SDL_RenderDrawLine(renderer, box.x, bottom, right, bottom);

    if (!font || !label || !label[0]) {
        return;
    }

    title_draw_text_tinted(renderer, font, label, label_x, box.y - label_h / 2, label_draw, SDL_FALSE);
}

int title_wrap_text(TTF_Font *font, const char *text, int max_width,
                    char lines[][TITLE_WRAP_LINE_LEN], int max_lines)
{
    if (!font || !text || max_lines <= 0) {
        return 0;
    }

    int line_count = 0;
    char word[64];
    char current[TITLE_WRAP_LINE_LEN];
    current[0] = '\0';

    const char *p = text;
    while (*p && line_count < max_lines) {
        while (*p == ' ') {
            p++;
        }
        size_t word_len = 0;
        while (p[word_len] && p[word_len] != ' ' && word_len < sizeof(word) - 1) {
            word_len++;
        }
        if (word_len == 0) {
            break;
        }
        memcpy(word, p, word_len);
        word[word_len] = '\0';
        p += word_len;

        char candidate[TITLE_WRAP_LINE_LEN];
        if (current[0]) {
            snprintf(candidate, sizeof(candidate), "%s %s", current, word);
        } else {
            snprintf(candidate, sizeof(candidate), "%s", word);
        }

        int candidate_w = 0;
        TTF_SizeUTF8(font, candidate, &candidate_w, NULL);

        if (candidate_w > max_width && current[0]) {
            strncpy(lines[line_count], current, TITLE_WRAP_LINE_LEN - 1);
            lines[line_count][TITLE_WRAP_LINE_LEN - 1] = '\0';
            line_count++;
            strncpy(current, word, sizeof(current) - 1);
            current[sizeof(current) - 1] = '\0';
        } else {
            strncpy(current, candidate, sizeof(current) - 1);
            current[sizeof(current) - 1] = '\0';
        }
    }

    if (current[0] && line_count < max_lines) {
        strncpy(lines[line_count], current, TITLE_WRAP_LINE_LEN - 1);
        lines[line_count][TITLE_WRAP_LINE_LEN - 1] = '\0';
        line_count++;
    }

    return line_count;
}
