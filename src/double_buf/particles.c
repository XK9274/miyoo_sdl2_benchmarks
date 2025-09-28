#include "double_buf/particles.h"

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

    static SDL_Point point_buffer[DB_MAX_PARTICLES];
    const SDL_Color *palette = db_state_particle_palette();
    int total_points = 0;
    int run_start = 0;
    int current_color_index = -1;

    for (int i = 0; i < state->particle_count; ++i) {
        BenchParticle *p = &state->particles[i];

        // Skip particles outside visible area
        if (p->x < -1.0f || p->x > DB_SCREEN_W + 1.0f ||
            p->y < state->top_margin - 1.0f || p->y > DB_SCREEN_H + 1.0f) {
            continue;
        }

        const Uint8 color_index = (i < DB_MAX_PARTICLES) ? state->particle_color_index[i] : 0;
        if (color_index >= DB_PARTICLE_PALETTE_SIZE) {
            continue;
        }

        if (current_color_index != -1 && color_index != current_color_index && total_points > run_start) {
            const SDL_Color color = palette[current_color_index % DB_PARTICLE_PALETTE_SIZE];
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
            SDL_RenderDrawPoints(renderer, &point_buffer[run_start], total_points - run_start);

            if (metrics) {
                metrics->draw_calls++;
                metrics->vertices_rendered += (total_points - run_start);
            }

            run_start = total_points;
        }

        current_color_index = color_index;
        if (total_points < DB_MAX_PARTICLES) {
            point_buffer[total_points].x = (int)p->x;
            point_buffer[total_points].y = (int)p->y;
            total_points++;
        }
    }

    if (total_points > run_start && current_color_index != -1) {
        const SDL_Color color = palette[current_color_index % DB_PARTICLE_PALETTE_SIZE];
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
        SDL_RenderDrawPoints(renderer, &point_buffer[run_start], total_points - run_start);

        if (metrics) {
            metrics->draw_calls++;
            metrics->vertices_rendered += (total_points - run_start);
        }
    }
}
