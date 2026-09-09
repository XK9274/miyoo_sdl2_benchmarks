#include "double_buf/render.h"

#include <math.h>

#include "double_buf/particles.h"
#include "common/geometry/shapes.h"

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

static void db_render_fill_stress(DoubleBenchState *state,
                                  SDL_Renderer *renderer,
                                  BenchMetrics *metrics)
{
    if (state->fill_intensity <= 0) {
        return;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    const int start_y = (int)state->top_margin;
    const int region_height = DB_SCREEN_H - start_y > 1 ? DB_SCREEN_H - start_y : 1;
    const int columns = 6 + state->fill_intensity * 3;
    const int passes = state->fill_intensity > 6 ? 2 : 1;

    for (int pass = 0; pass < passes; ++pass) {
        const float pass_phase = state->fill_phase_units + (float)pass * 2.1f;

        for (int col = 0; col < columns; ++col) {
            const int col_x = (col * DB_SCREEN_W) / columns;
            const int next_x = ((col + 1) * DB_SCREEN_W) / columns;
            const int col_w = next_x - col_x > 1 ? next_x - col_x : 1;

            const float wave = sinf(pass_phase + (float)col * 0.35f) * 0.5f + 0.5f;
            const Uint8 r = (Uint8)(30.0f + wave * 60.0f);
            const Uint8 g = (Uint8)(24.0f + (1.0f - wave) * 50.0f);
            const Uint8 b = (Uint8)(40.0f + wave * 70.0f);

            SDL_SetRenderDrawColor(renderer, r, g, b, 255);
            SDL_Rect rect = {col_x, start_y, col_w, region_height};
            SDL_RenderFillRect(renderer, &rect);
            if (metrics) {
                metrics->draw_calls++;
                metrics->vertices_rendered += 4;
                metrics->triangles_rendered += 2;
            }
        }
    }
}

void db_render_scene(DoubleBenchState *state,
                     SDL_Renderer *renderer,
                     BenchMetrics *metrics)
{
    if (!state) {
        return;
    }

    db_render_fill_stress(state, renderer, metrics);

    if (state->show_shape) {
        bench_render_shape(state->shape_type,
                           renderer,
                           metrics,
                           state->cube_rotation,
                           DB_SCREEN_W * 0.5f,
                           state->center_y,
                           50.0f,
                           state->render_mode);
    }

    db_particles_draw(state, renderer, metrics);
}
