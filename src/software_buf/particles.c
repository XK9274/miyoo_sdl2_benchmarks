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

    static SDL_FPoint point_buffer[SB_MAX_PARTICLES];
    const SDL_Color *palette = sb_state_particle_palette();
    const int alpha_bucket_size = SB_ALPHA_BUCKETS ? 256 / SB_ALPHA_BUCKETS : 256;
    int total_points = 0;
    int run_start = 0;
    int current_color_index = -1;
    int current_bucket = -1;

    for (int i = 0; i < state->particle_count; ++i) {
        BenchParticle *p = &state->particles[i];
        const Uint8 palette_index = (i < SB_MAX_PARTICLES) ? state->particle_color_index[i] : 0;
        if (palette_index >= SB_PARTICLE_PALETTE_SIZE) {
            continue;
        }

        Uint8 alpha = (Uint8)(p->a * p->life);
        int bucket = alpha_bucket_size ? (alpha / alpha_bucket_size) : 0;
        if (bucket >= SB_ALPHA_BUCKETS) {
            bucket = SB_ALPHA_BUCKETS - 1;
        }

        if (current_color_index != -1 &&
            (palette_index != current_color_index || bucket != current_bucket) &&
            total_points > run_start) {
            const SDL_Color color = palette[current_color_index % SB_PARTICLE_PALETTE_SIZE];
            Uint8 flush_alpha = 255;
            if (alpha_bucket_size > 0) {
                int base = current_bucket * alpha_bucket_size + alpha_bucket_size / 2;
                if (base > 255) {
                    base = 255;
                }
                flush_alpha = (Uint8)base;
            }

            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, flush_alpha);
            SDL_RenderDrawPointsF(renderer, &point_buffer[run_start], total_points - run_start);

            if (metrics) {
                metrics->draw_calls++;
                metrics->vertices_rendered += (total_points - run_start);
            }

            run_start = total_points;
        }

        current_color_index = palette_index;
        current_bucket = bucket;

        if (total_points < SB_MAX_PARTICLES) {
            point_buffer[total_points].x = p->x;
            point_buffer[total_points].y = p->y;
            total_points++;
        }
    }

    if (total_points > run_start && current_color_index != -1 && current_bucket != -1) {
        const SDL_Color color = palette[current_color_index % SB_PARTICLE_PALETTE_SIZE];
        Uint8 flush_alpha = 255;
        if (alpha_bucket_size > 0) {
            int base = current_bucket * alpha_bucket_size + alpha_bucket_size / 2;
            if (base > 255) {
                base = 255;
            }
            flush_alpha = (Uint8)base;
        }

        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, flush_alpha);
        SDL_RenderDrawPointsF(renderer, &point_buffer[run_start], total_points - run_start);

        if (metrics) {
            metrics->draw_calls++;
            metrics->vertices_rendered += (total_points - run_start);
        }
    }
}
