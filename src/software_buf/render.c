#include "software_buf/render.h"

#include "software_buf/particles.h"
#include "common/geometry.h"

static void draw_stress_grid(SoftwareBenchState *state,
                             SDL_Renderer *renderer,
                             BenchMetrics *metrics)
{
    if (!state->stress_grid) {
        return;
    }

    SDL_SetRenderDrawColor(renderer, 15, 25, 45, 160);
    for (int x = 0; x < SB_SCREEN_W; x += 12) {
        SDL_RenderDrawLine(renderer, x, (int)state->top_margin, x, SB_SCREEN_H);
        if (metrics) {
            metrics->draw_calls++;
        }
    }
    for (int y = (int)state->top_margin; y < SB_SCREEN_H; y += 12) {
        SDL_RenderDrawLine(renderer, 0, y, SB_SCREEN_W, y);
        if (metrics) {
            metrics->draw_calls++;
        }
    }
}

void sb_render_scene(SDL_Renderer *renderer,
                     SDL_Texture *backbuffer,
                     SoftwareBenchState *state,
                     BenchMetrics *metrics)
{
    SDL_SetRenderTarget(renderer, backbuffer);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(renderer, 12, 18, 32, 255);
    SDL_RenderClear(renderer);
    if (metrics) {
        metrics->draw_calls++;
    }

    draw_stress_grid(state, renderer, metrics);

    if (state->show_cube) {
        if (state->shape_type == 0) {
            bench_render_cube(renderer,
                              metrics,
                              state->cube_rotation,
                              SB_SCREEN_W * 0.5f,
                              state->center_y,
                              50.0f,
                              state->render_mode);
        } else {
            bench_render_octahedron(renderer,
                                    metrics,
                                    state->cube_rotation,
                                    SB_SCREEN_W * 0.5f,
                                    state->center_y,
                                    50.0f,
                                    state->render_mode);
        }
    }

    sb_particles_draw(state, renderer, metrics);
}
