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
