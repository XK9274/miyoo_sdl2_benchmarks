#include "space_bench/render/internal.h"

#include <math.h>

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
    for (int i = 0; i < SPACE_STAR_COUNT; ++i) {
        const float sx = state->star_x[i];
        const float sy = state->star_y[i];
        if (sx < 0.0f || sx >= (float)SPACE_SCREEN_W || sy < state->play_area_top || sy >= state->play_area_bottom) {
            continue;
        }

        SDL_SetRenderDrawColor(renderer, 155, 175, 255, 200);
        SDL_RenderDrawPointF(renderer, sx, sy);

        if (metrics) {
            metrics->draw_calls++;
            metrics->vertices_rendered += 1;
        }
    }

    for (int i = 0; i < SPACE_SPEEDLINE_COUNT; ++i) {
        const float sx = state->speedline_x[i];
        const float sy = state->speedline_y[i];
        if (sx < -32.0f || sx >= (float)SPACE_SCREEN_W + 16.0f || sy < state->play_area_top || sy >= state->play_area_bottom) {
            continue;
        }

        SDL_SetRenderDrawColor(renderer, 120, 160, 255, 150);
        SDL_RenderDrawLineF(renderer,
                            sx,
                            sy,
                            sx + state->speedline_length[i],
                            sy);

        if (metrics) {
            metrics->draw_calls++;
            metrics->vertices_rendered += 2;
        }
    }
}
