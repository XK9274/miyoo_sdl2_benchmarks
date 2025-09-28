#include "space_bench/state/internal.h"
#include "space_bench/state/constants.h"

#include <math.h>

void space_spawn_explosion(SpaceBenchState *state, float x, float y, float radius)
{
    for (int i = 0; i < SPACE_MAX_EXPLOSIONS; ++i) {
        SpaceExplosion *exp = &state->explosions[i];
        if (!exp->active) {
            exp->active = SDL_TRUE;
            exp->x = x;
            exp->y = y;
            exp->timer = 0.0f;
            exp->lifetime = space_rand_range(state, SPACE_EXPLOSION_LIFETIME * 0.65f, SPACE_EXPLOSION_LIFETIME * 1.35f);
            exp->radius = radius;
            break;
        }
    }

    for (int i = 0; i < SPACE_MAX_PARTICLES; ++i) {
        SpaceParticle *particle = &state->particles[i];
        if (!particle->active) {
            const float angle = space_rand_range(state, 0.0f, (float)(M_PI * 2.0));
            const float speed = space_rand_range(state, 60.0f, 160.0f);
            particle->active = SDL_TRUE;
            particle->x = x;
            particle->y = y;
            particle->vx = cosf(angle) * speed - state->scroll_speed * 0.5f;
            particle->vy = sinf(angle) * speed;
            particle->life = particle->max_life = space_rand_range(state, 0.4f, 0.7f);
            particle->r = 255;
            particle->g = (Uint8)space_rand_range(state, 160.0f, 255.0f);
            particle->b = (Uint8)space_rand_range(state, 80.0f, 200.0f);
            break;
        }
    }
}

void space_spawn_layer_explosion(SpaceBenchState *state, float x, float y, float radius, int particles)
{
    for (int i = 0; i < particles; ++i) {
        const float angle = (float)(M_PI * 2.0) * (float)i / (float)particles;
        const float dist = space_rand_range(state, radius * 0.3f, radius);
        const float ex = x + cosf(angle) * dist;
        const float ey = y + sinf(angle) * dist;
        space_spawn_explosion(state, ex, ey, 15.0f);
    }
}

void space_spawn_firing_particles(SpaceBenchState *state, float x, float y, SDL_bool is_laser)
{
    const int spawn_count = is_laser ? 2 : 4;
    for (int i = 0; i < spawn_count; ++i) {
        for (int p = 0; p < SPACE_MAX_PARTICLES; ++p) {
            SpaceParticle *particle = &state->particles[p];
            if (!particle->active) {
                particle->active = SDL_TRUE;
                particle->x = x + space_rand_range(state, -6.0f, 3.0f);
                particle->y = y + space_rand_range(state, -4.0f, 4.0f);
                particle->vx = space_rand_range(state, -30.0f, 14.0f) - state->scroll_speed * 0.22f;
                particle->vy = space_rand_range(state, -40.0f, 40.0f);
                particle->life = particle->max_life = is_laser ? 0.24f : 0.36f;
                particle->r = 255;
                particle->g = is_laser ? 190 : 200;
                particle->b = is_laser ? 255 : 140;
                break;
            }
        }
    }
}

void space_spawn_laser_charge_particles(SpaceBenchState *state, float x, float y, float charge_progress)
{
    const int spawn_count = (int)(charge_progress * 6.0f) + 2;
    for (int i = 0; i < spawn_count; ++i) {
        for (int p = 0; p < SPACE_MAX_PARTICLES; ++p) {
            SpaceParticle *particle = &state->particles[p];
            if (!particle->active) {
                particle->active = SDL_TRUE;
                // Spawn particles around the player and suck them towards the front
                const float angle = space_rand_range(state, 0.0f, (float)(M_PI * 2.0));
                const float dist = space_rand_range(state, 20.0f, 40.0f);
                particle->x = x + cosf(angle) * dist;
                particle->y = y + sinf(angle) * dist;

                // Calculate velocity towards front of ship with some attraction
                const float target_x = x + 24.0f; // Front of ship
                const float target_y = y;
                const float dx = target_x - particle->x;
                const float dy = target_y - particle->y;
                const float speed = 120.0f + charge_progress * 80.0f;
                const float len = SDL_max(1.0f, SDL_sqrtf(dx * dx + dy * dy));

                particle->vx = (dx / len) * speed - state->scroll_speed * 0.5f;
                particle->vy = (dy / len) * speed;
                particle->life = particle->max_life = 0.5f;
                particle->r = 100 + (Uint8)(charge_progress * 155.0f);
                particle->g = 150 + (Uint8)(charge_progress * 105.0f);
                particle->b = 255;
                break;
            }
        }
    }
}

void space_spawn_laser_firing_particles(SpaceBenchState *state, float x, float y)
{
    const int spawn_count = 8;
    for (int i = 0; i < spawn_count; ++i) {
        for (int p = 0; p < SPACE_MAX_PARTICLES; ++p) {
            SpaceParticle *particle = &state->particles[p];
            if (!particle->active) {
                particle->active = SDL_TRUE;
                particle->x = x + space_rand_range(state, -4.0f, 8.0f);
                particle->y = y + space_rand_range(state, -6.0f, 6.0f);
                // Fire particles forward with high velocity
                particle->vx = space_rand_range(state, 150.0f, 300.0f) - state->scroll_speed * 0.3f;
                particle->vy = space_rand_range(state, -50.0f, 50.0f);
                particle->life = particle->max_life = 0.4f;
                particle->r = 255;
                particle->g = 200;
                particle->b = 255;
                break;
            }
        }
    }
}

void space_update_particles(SpaceBenchState *state, float dt)
{
    (void)state;
    for (int i = 0; i < SPACE_MAX_PARTICLES; ++i) {
        SpaceParticle *particle = &state->particles[i];
        if (!particle->active) {
            continue;
        }
        particle->x += particle->vx * dt;
        particle->y += particle->vy * dt;
        particle->life -= dt;

        if (particle->life <= 0.0f) {
            particle->active = SDL_FALSE;
        }
    }
}

void space_spawn_enemy_missile_trail(SpaceBenchState *state,
                                     float x,
                                     float y,
                                     float vx,
                                     float vy)
{
    int cursor = state->particle_cursor % SPACE_MAX_PARTICLES;
    for (int attempt = 0; attempt < SPACE_MAX_PARTICLES; ++attempt) {
        const int idx = (cursor + attempt) % SPACE_MAX_PARTICLES;
        SpaceParticle *particle = &state->particles[idx];
        if (particle->active) {
            continue;
        }

        const float jitter_x = space_rand_range(state, -18.0f, 18.0f);
        const float jitter_y = space_rand_range(state, -12.0f, 12.0f);

        particle->active = SDL_TRUE;
        particle->x = x + jitter_x * 0.05f;
        particle->y = y + jitter_y * 0.05f;
        particle->vx = -vx * 0.12f + jitter_x * 0.6f - state->scroll_speed * 0.15f;
        particle->vy = -vy * 0.12f + jitter_y * 0.4f;
        particle->life = particle->max_life = 0.32f;
        particle->r = 255;
        particle->g = 180;
        particle->b = 100;

        state->particle_cursor = (idx + 1) % SPACE_MAX_PARTICLES;
        break;
    }
}
