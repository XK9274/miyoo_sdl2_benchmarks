#include "space_bench/render/internal.h"

void space_render_background(SDL_Renderer *renderer, BenchMetrics *metrics)
{
    SDL_SetRenderDrawColor(renderer, SPACE_BACKGROUND_R, SPACE_BACKGROUND_G, SPACE_BACKGROUND_B, 255);
    SDL_RenderClear(renderer);
    if (metrics) {
        metrics->draw_calls++;
        metrics->vertices_rendered += 4;
    }
}

void space_render_stars(const SpaceBenchState *state,
                        SDL_Renderer *renderer,
                        BenchMetrics *metrics)
{
    SDL_FPoint star_bins[4][SPACE_STAR_COUNT];
    int star_counts[4] = {0, 0, 0, 0};

    for (int i = 0; i < SPACE_STAR_COUNT; ++i) {
        const Uint8 brightness = state->star_brightness[i];
        int bucket = brightness / 64;
        if (bucket > 3) bucket = 3;
        star_bins[bucket][star_counts[bucket]++] = (SDL_FPoint){state->star_x[i], state->star_y[i]};
    }

    for (int bucket = 0; bucket < 4; ++bucket) {
        if (star_counts[bucket] == 0) {
            continue;
        }
        const Uint8 brightness = (Uint8)(40 + bucket * 60);
        SDL_SetRenderDrawColor(renderer, brightness, brightness, brightness, 255);
        SDL_RenderDrawPointsF(renderer, star_bins[bucket], star_counts[bucket]);
        if (metrics) {
            metrics->draw_calls++;
            metrics->vertices_rendered += star_counts[bucket];
        }
    }
}
