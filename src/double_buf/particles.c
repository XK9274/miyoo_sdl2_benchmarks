#include "double_buf/particles.h"

// Pre-allocated buffer to avoid stack allocation every frame
static SDL_Point g_point_buffer[DB_MAX_PARTICLES];

void db_particles_update(DoubleBenchState *state, double dt)
{
    if (!state || !state->show_particles) {
        return;
    }

    const float speed = state->particle_speed;
    for (int i = 0; i < state->particle_count; ++i) {
        BenchParticle *p = &state->particles[i];
        p->x += p->dx * (float)(dt * speed);
        p->y += p->dy * (float)(dt * speed);
        p->life -= (float)(dt * 0.4f);

        if (p->x < -6.0f || p->x > DB_SCREEN_W + 6.0f ||
            p->y < state->top_margin - 6.0f || p->y > DB_SCREEN_H + 6.0f ||
            p->life <= 0.0f) {
            db_state_respawn_particle(state, p);
        }
    }
}

void db_particles_draw(DoubleBenchState *state,
                       SDL_Renderer *renderer,
                       BenchMetrics *metrics)
{
    if (!state || !state->show_particles) {
        return;
    }

    // Disable alpha blending for better performance
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    SDL_Point *points = g_point_buffer;
    int point_count = 0;
    Uint32 current_color = 0;
    int batch_start = 0;

    for (int i = 0; i < state->particle_count; ++i) {
        BenchParticle *p = &state->particles[i];

        // Skip particles outside visible area
        if (p->x < -1.0f || p->x > DB_SCREEN_W + 1.0f ||
            p->y < state->top_margin - 1.0f || p->y > DB_SCREEN_H + 1.0f) {
            continue;
        }

        // Use full opacity since blending is disabled
        Uint32 particle_color = ((Uint32)p->r << 24) | ((Uint32)p->g << 16) |
                               ((Uint32)p->b << 8) | 255;

        // If color changed and we have points to draw, flush the batch
        if (particle_color != current_color && point_count > 0) {
            Uint8 r = (current_color >> 24) & 0xFF;
            Uint8 g = (current_color >> 16) & 0xFF;
            Uint8 b = (current_color >> 8) & 0xFF;
            Uint8 a = current_color & 0xFF;

            SDL_SetRenderDrawColor(renderer, r, g, b, a);
            SDL_RenderDrawPoints(renderer, &points[batch_start], point_count - batch_start);

            if (metrics) {
                metrics->draw_calls++;
                metrics->vertices_rendered += (point_count - batch_start);
            }

            batch_start = point_count;
        }

        current_color = particle_color;
        points[point_count].x = (int)p->x;
        points[point_count].y = (int)p->y;
        point_count++;
    }

    // Draw the final batch
    if (point_count > batch_start) {
        Uint8 r = (current_color >> 24) & 0xFF;
        Uint8 g = (current_color >> 16) & 0xFF;
        Uint8 b = (current_color >> 8) & 0xFF;
        Uint8 a = current_color & 0xFF;

        SDL_SetRenderDrawColor(renderer, r, g, b, a);
        SDL_RenderDrawPoints(renderer, &points[batch_start], point_count - batch_start);

        if (metrics) {
            metrics->draw_calls++;
            metrics->vertices_rendered += (point_count - batch_start);
        }
    }
}
