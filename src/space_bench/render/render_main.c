#include "space_bench/render/internal.h"

#include <math.h>

void space_render_scene(SpaceBenchState *state,
                        SDL_Renderer *renderer,
                        BenchMetrics *metrics)
{
    if (!state || !renderer) {
        return;
    }

    const int overlay_height = (int)state->play_area_top;
    const int game_height = (int)(state->play_area_bottom - state->play_area_top);
    SDL_Rect game_clip = {0, overlay_height, SPACE_SCREEN_W, game_height};
    SDL_RenderSetClipRect(renderer, &game_clip);

    space_render_background(renderer, metrics);
    space_render_stars(state, renderer, metrics);
    space_render_upgrades(state, renderer, metrics);
    space_render_lasers(state, renderer, metrics);
    space_render_enemy_shots(state, renderer, metrics);
    space_render_bullets(state, renderer, metrics);
    space_render_particles(state, renderer, metrics);

    if (state->anomaly_pending && state->anomaly_warning_timer > 0.0f) {
        const float flash = 0.5f + 0.5f * sinf(state->anomaly_warning_phase * 3.2f);
        const float alpha = 140.0f + 100.0f * flash;
        const float warning_y = state->play_area_top + 8.0f;
        char warn_text[64];
        const float time_left = SDL_max(0.0f, state->anomaly_warning_timer);
        const int seconds_left = (int)ceilf(time_left);
        SDL_snprintf(warn_text, sizeof(warn_text), "ANOMALY DETECTED... %ds", seconds_left);

        TTF_Font *warn_font = bench_load_font(18);
        if (warn_font) {
            SDL_Color warn_color = {255, (Uint8)(140 + 80 * flash), (Uint8)(40 + 60 * flash), (Uint8)alpha};
            SDL_Surface *warn_surface = TTF_RenderUTF8_Blended(warn_font, warn_text, warn_color);
            if (warn_surface) {
                SDL_Texture *warn_texture = SDL_CreateTextureFromSurface(renderer, warn_surface);
                if (warn_texture) {
                    const SDL_Rect warn_rect = {
                        (int)(SPACE_SCREEN_W * 0.5f - warn_surface->w * 0.5f),
                        (int)warning_y,
                        warn_surface->w,
                        warn_surface->h
                    };
                    SDL_SetTextureAlphaMod(warn_texture, (Uint8)alpha);
                    SDL_RenderCopy(renderer, warn_texture, NULL, &warn_rect);
                    SDL_DestroyTexture(warn_texture);
                }
                SDL_FreeSurface(warn_surface);
            }
            TTF_CloseFont(warn_font);
        } else {
            SDL_SetRenderDrawColor(renderer, 255, 120, 60, (Uint8)alpha);
            const int bar_width = SPACE_SCREEN_W - 40;
            SDL_RenderDrawLine(renderer, 20, (int)warning_y, 20 + bar_width, (int)warning_y);
        }
    }

    space_render_trail(&state->player_trail,
                       renderer,
                       metrics,
                       (SDL_Color){120, 210, 255, 200},
                       (SDL_Color){255, 255, 200, 200});
    for (int i = 0; i < state->weapon_upgrades.drone_count; ++i) {
        space_render_trail(&state->drone_trails[i],
                           renderer,
                           metrics,
                           (SDL_Color){160, 120, 255, 200},
                           (SDL_Color){255, 200, 255, 200});
    }
    for (int i = 0; i < SPACE_MAX_ENEMIES; ++i) {
        space_render_trail(&state->enemy_trails[i],
                           renderer,
                           metrics,
                           (SDL_Color){255, 140, 120, 200},
                           (SDL_Color){255, 200, 160, 160});
    }

    space_render_enemies(state, renderer, metrics);
    space_render_anomaly(state, renderer, metrics);
    space_render_drones(state, renderer, metrics);
    space_render_explosions(state, renderer, metrics);
    space_render_player(state, renderer, metrics);

    if (state->game_state == SPACE_GAME_OVER) {
        space_render_game_over(state, renderer, metrics);
    }

    SDL_RenderSetClipRect(renderer, NULL);
}
