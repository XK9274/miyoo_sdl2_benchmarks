#include "space_bench/render/internal.h"

#include <math.h>
#include <string.h>

void space_render_game_over(const SpaceBenchState *state,
                                   SDL_Renderer *renderer,
                                   BenchMetrics *metrics)
{
    // Render stars behind overlay
    space_render_stars(state, renderer, metrics);

    // Semi-transparent black overlay
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 140);  // Lighter overlay to show stars
    SDL_Rect overlay_rect = {0, (int)state->play_area_top, SPACE_SCREEN_W, (int)(state->play_area_bottom - state->play_area_top)};
    SDL_RenderFillRect(renderer, &overlay_rect);

    // Calculate center positions
    const float center_x = SPACE_SCREEN_W * 0.5f;
    const float center_y = (state->play_area_top + state->play_area_bottom) * 0.5f;

    // Try to load font for text rendering
    TTF_Font *font = bench_load_font(24);
    TTF_Font *small_font = bench_load_font(16);

    if (font) {
        // Render "GAME OVER" with TTF
        SDL_Color text_color = {255, 80, 80, 255};
        SDL_Surface *text_surface = TTF_RenderUTF8_Blended(font, "GAME OVER", text_color);
        if (text_surface) {
            SDL_Texture *text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
            if (text_texture) {
                SDL_Rect text_rect = {
                    (int)(center_x - text_surface->w * 0.5f),
                    (int)(center_y - 60.0f),
                    text_surface->w,
                    text_surface->h
                };
                SDL_RenderCopy(renderer, text_texture, NULL, &text_rect);
                SDL_DestroyTexture(text_texture);
            }
            SDL_FreeSurface(text_surface);
        }

        // Show score and kills
        SDL_Color stats_color = {255, 255, 255, 255};
        char stats_text[64];
        SDL_snprintf(stats_text, sizeof(stats_text), "Score: %d  Enemies Killed: %d", state->score, state->total_enemies_killed);

        SDL_Surface *stats_surface = TTF_RenderUTF8_Blended(small_font ? small_font : font, stats_text, stats_color);
        if (stats_surface) {
            SDL_Texture *stats_texture = SDL_CreateTextureFromSurface(renderer, stats_surface);
            if (stats_texture) {
                SDL_Rect stats_rect = {
                    (int)(center_x - stats_surface->w * 0.5f),
                    (int)(center_y - 20.0f),
                    stats_surface->w,
                    stats_surface->h
                };
                SDL_RenderCopy(renderer, stats_texture, NULL, &stats_rect);
                SDL_DestroyTexture(stats_texture);
            }
            SDL_FreeSurface(stats_surface);
        }

        // Show countdown if active
        if (state->gameover_countdown > 0.0f) {
            SDL_Color countdown_color = {255, 255, 100, 200};
            const int countdown_seconds = (int)(state->gameover_countdown) + 1;
            char countdown_text[32];
            SDL_snprintf(countdown_text, sizeof(countdown_text), "Select in %d", countdown_seconds);

            SDL_Surface *countdown_surface = TTF_RenderUTF8_Blended(small_font ? small_font : font, countdown_text, countdown_color);
            if (countdown_surface) {
                SDL_Texture *countdown_texture = SDL_CreateTextureFromSurface(renderer, countdown_surface);
                if (countdown_texture) {
                    SDL_Rect countdown_rect = {
                        (int)(center_x - countdown_surface->w * 0.5f),
                        (int)(center_y + 60.0f),
                        countdown_surface->w,
                        countdown_surface->h
                    };
                    SDL_RenderCopy(renderer, countdown_texture, NULL, &countdown_rect);
                    SDL_DestroyTexture(countdown_texture);
                }
                SDL_FreeSurface(countdown_surface);
            }
        }

        // Retry option
        SDL_Color retry_color = (state->gameover_selected == SPACE_GAMEOVER_RETRY) ?
                               (SDL_Color){255, 255, 120, 255} : (SDL_Color){180, 180, 180, 255};
        SDL_Surface *retry_surface = TTF_RenderUTF8_Blended(small_font ? small_font : font, "RETRY", retry_color);
        if (retry_surface) {
            SDL_Texture *retry_texture = SDL_CreateTextureFromSurface(renderer, retry_surface);
            if (retry_texture) {
                SDL_Rect retry_rect = {
                    (int)(center_x - 100.0f - retry_surface->w * 0.5f),
                    (int)(center_y + 20.0f),
                    retry_surface->w,
                    retry_surface->h
                };
                SDL_RenderCopy(renderer, retry_texture, NULL, &retry_rect);
                SDL_DestroyTexture(retry_texture);
            }
            SDL_FreeSurface(retry_surface);
        }

        // Quit option
        SDL_Color quit_color = (state->gameover_selected == SPACE_GAMEOVER_QUIT) ?
                              (SDL_Color){255, 255, 120, 255} : (SDL_Color){180, 180, 180, 255};
        SDL_Surface *quit_surface = TTF_RenderUTF8_Blended(small_font ? small_font : font, "QUIT", quit_color);
        if (quit_surface) {
            SDL_Texture *quit_texture = SDL_CreateTextureFromSurface(renderer, quit_surface);
            if (quit_texture) {
                SDL_Rect quit_rect = {
                    (int)(center_x + 100.0f - quit_surface->w * 0.5f),
                    (int)(center_y + 20.0f),
                    quit_surface->w,
                    quit_surface->h
                };
                SDL_RenderCopy(renderer, quit_texture, NULL, &quit_rect);
                SDL_DestroyTexture(quit_texture);
            }
            SDL_FreeSurface(quit_surface);
        }

        TTF_CloseFont(font);
        if (small_font) TTF_CloseFont(small_font);
    } else {
        // Fallback to rectangle rendering if no font available
        const float char_width = 8.0f;
        const float char_height = 16.0f;
        SDL_SetRenderDrawColor(renderer, 255, 80, 80, 255);
        const char* game_over_text = "GAME OVER";
        const float text_width = strlen(game_over_text) * (char_width + 2.0f) - 2.0f;
        const float text_start_x = center_x - text_width * 0.5f;
        for (int i = 0; game_over_text[i] != '\0'; ++i) {
            if (game_over_text[i] != ' ') {
                SDL_FRect char_rect = {text_start_x + i * (char_width + 2.0f), center_y - 60.0f, char_width, char_height};
                SDL_RenderFillRectF(renderer, &char_rect);
            }
        }
    }

    if (metrics) {
        metrics->draw_calls += 4;
    }
}
