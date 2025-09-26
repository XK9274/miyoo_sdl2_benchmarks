#include "double_buf/render.h"

#include "double_buf/particles.h"
#include "common/geometry.h"

void db_render_backdrop(DoubleBenchState *state,
                        SDL_Renderer *renderer,
                        BenchMetrics *metrics)
{
    if (!state) {
        return;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    if (!state->backdrop_grid) {
        SDL_SetRenderDrawColor(renderer, 8, 10, 20, 255);
        SDL_RenderClear(renderer);
        if (metrics) {
            metrics->draw_calls++;
        }
        return;
    }

    SDL_SetRenderDrawColor(renderer, 10, 12, 24, 255);
    SDL_RenderClear(renderer);
    if (metrics) {
        metrics->draw_calls++;
    }

    SDL_SetRenderDrawColor(renderer, 20, 26, 42, 120);
    for (int x = 0; x < DB_SCREEN_W; x += 16) {
        SDL_RenderDrawLine(renderer, x, (int)state->top_margin, x, DB_SCREEN_H);
        if (metrics) {
            metrics->draw_calls++;
        }
    }
    for (int y = (int)state->top_margin; y < DB_SCREEN_H; y += 16) {
        SDL_RenderDrawLine(renderer, 0, y, DB_SCREEN_W, y);
        if (metrics) {
            metrics->draw_calls++;
        }
    }
}

void db_render_cube_and_particles(DoubleBenchState *state,
                                  SDL_Renderer *renderer,
                                  BenchMetrics *metrics)
{
    if (!state) {
        return;
    }

    if (state->show_cube) {
        bench_render_cube(renderer,
                          metrics,
                          state->cube_rotation,
                          DB_SCREEN_W * 0.5f,
                          state->center_y,
                          50.0f,
                          state->render_mode);
    }

    db_particles_draw(state, renderer, metrics);
}
