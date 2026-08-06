#include "space_bench/overlay.h"

#include <float.h>
#include <string.h>

typedef struct {
    char text[192];
    SDL_Texture *texture;
    int w;
    int h;
} HudLineCache;

static void draw_hud_line(SDL_Renderer *renderer,
                          TTF_Font *font,
                          HudLineCache *cache,
                          const char *text,
                          int y,
                          SDL_Color color)
{
    if (!text || text[0] == '\0') {
        return;
    }

    if (cache->texture == NULL || strcmp(cache->text, text) != 0) {
        if (cache->texture) {
            SDL_DestroyTexture(cache->texture);
            cache->texture = NULL;
        }

        if (font) {
            SDL_Surface *surface = TTF_RenderUTF8_Blended(font, text, color);
            if (surface) {
                cache->texture = SDL_CreateTextureFromSurface(renderer, surface);
                if (cache->texture) {
                    SDL_SetTextureBlendMode(cache->texture, SDL_BLENDMODE_BLEND);
                    cache->w = surface->w;
                    cache->h = surface->h;
                }
                SDL_FreeSurface(surface);
            }
        }

        strncpy(cache->text, text, sizeof(cache->text) - 1);
        cache->text[sizeof(cache->text) - 1] = '\0';
    }

    if (cache->texture) {
        SDL_Rect dst = {8, y, cache->w, cache->h};
        SDL_RenderCopy(renderer, cache->texture, NULL, &dst);
    }
}

void space_hud_render(SDL_Renderer *renderer,
                      const SpaceBenchState *state,
                      const BenchMetrics *metrics)
{
    if (!renderer || !state) {
        return;
    }

    static TTF_Font *font = NULL;
    static HudLineCache perf_cache;
    static HudLineCache status_cache;
    static char perf_line[64] = "FPS -- | Frame --";
    static int perf_update_counter = 0;
    if (!font) {
        font = bench_load_font(SPACE_HUD_LINE_HEIGHT - 4);
    }

    SDL_Rect strip = {0, 0, SPACE_SCREEN_W, SPACE_HUD_STRIP_HEIGHT};
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_RenderFillRect(renderer, &strip);

    const SDL_Color perf_color = {180, 220, 255, 255};
    const SDL_Color status_color = {220, 220, 220, 255};

    /* Only reformat the FPS text a few times a second -- it changes almost
     * every frame, which would defeat draw_hud_line's texture cache and
     * re-rasterize text at full frame rate for no visible benefit. */
    if (metrics && perf_update_counter <= 0) {
        SDL_snprintf(perf_line, sizeof(perf_line), "FPS %.1f | Frame %.2fms",
                     metrics->current_fps, metrics->frame_time_ms);
        perf_update_counter = 6;
    }
    if (perf_update_counter > 0) {
        perf_update_counter--;
    }
    draw_hud_line(renderer, font, &perf_cache, perf_line, 2, perf_color);

    const char *guidance = state->weapon_upgrades.guidance_active ? "ON" : "--";
    const char *thumper = state->weapon_upgrades.thumper_active ? "ON" : "--";
    char status_line[192];
    SDL_snprintf(status_line, sizeof(status_line),
                "Score %d | Enemies %d | Missed %d | Split %d | Guide %s | Drones %d | Thumper %s",
                state->score,
                state->total_enemies_killed,
                state->player_hits,
                state->weapon_upgrades.split_level,
                guidance,
                state->weapon_upgrades.drone_count,
                thumper);
    draw_hud_line(renderer, font, &status_cache, status_line, SPACE_HUD_LINE_HEIGHT + 4, status_color);
}
