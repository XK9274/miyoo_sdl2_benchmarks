#include "software_buf/particles.h"

void sb_particles_update(SoftwareBenchState *state, double dt)
{
    if (!state || !state->show_particles) {
        return;
    }

    const float speed = state->particle_speed;
    for (int i = 0; i < state->particle_count; ++i) {
        BenchParticle *p = &state->particles[i];
        p->x += p->dx * (float)(dt * speed);
        p->y += p->dy * (float)(dt * speed);
        p->life -= (float)(dt * 0.45f);

        if (p->x < -4.0f || p->x > SB_SCREEN_W + 4.0f ||
            p->y < state->top_margin - 4.0f || p->y > SB_SCREEN_H + 4.0f ||
            p->life <= 0.0f) {
            sb_state_respawn_particle(state, p);
        }
    }
}

void sb_particles_draw(SoftwareBenchState *state,
                       SDL_Renderer *renderer,
                       BenchMetrics *metrics)
{
    if (!state || !state->show_particles) {
        return;
    }

    SDL_FPoint points[SB_MAX_PARTICLES];
    int point_count = 0;
    Uint32 current_color = 0;
    int batch_start = 0;

    for (int i = 0; i < state->particle_count; ++i) {
        BenchParticle *p = &state->particles[i];
        Uint8 alpha = (Uint8)(p->a * p->life);
        Uint32 particle_color = ((Uint32)p->r << 24) | ((Uint32)p->g << 16) |
                               ((Uint32)p->b << 8) | alpha;

        // If color changed and we have points to draw, flush the batch
        if (particle_color != current_color && point_count > 0) {
            Uint8 r = (current_color >> 24) & 0xFF;
            Uint8 g = (current_color >> 16) & 0xFF;
            Uint8 b = (current_color >> 8) & 0xFF;
            Uint8 a = current_color & 0xFF;

            SDL_SetRenderDrawColor(renderer, r, g, b, a);
            SDL_RenderDrawPointsF(renderer, &points[batch_start], point_count - batch_start);

            if (metrics) {
                metrics->draw_calls++;
                metrics->vertices_rendered += (point_count - batch_start);
            }

            batch_start = point_count;
        }

        current_color = particle_color;
        points[point_count].x = p->x;
        points[point_count].y = p->y;
        point_count++;
    }

    // Draw the final batch
    if (point_count > batch_start) {
        Uint8 r = (current_color >> 24) & 0xFF;
        Uint8 g = (current_color >> 16) & 0xFF;
        Uint8 b = (current_color >> 8) & 0xFF;
        Uint8 a = current_color & 0xFF;

        SDL_SetRenderDrawColor(renderer, r, g, b, a);
        SDL_RenderDrawPointsF(renderer, &points[batch_start], point_count - batch_start);

        if (metrics) {
            metrics->draw_calls++;
            metrics->vertices_rendered += (point_count - batch_start);
        }
    }
}
