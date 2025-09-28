#include "space_bench/state/internal.h"
#include "space_bench/state/constants.h"

#include <float.h>
#include <math.h>

void space_player_take_damage(SpaceBenchState *state, float amount)
{
    if (!state->player_alive || state->player_invulnerable > 0.0f) {
        return;
    }

    if (state->shield_active && state->shield_strength > 0.0f) {
        state->shield_strength -= amount;
        state->shield_pulse = 0.5f;
        if (state->shield_strength <= 0.0f) {
            state->shield_strength = 0.0f;
            state->shield_active = SDL_FALSE;
        }
        return;
    }

    state->player_health -= amount;
    if (state->player_health <= 0.0f) {
        state->player_health = 0.0f;
        state->player_alive = SDL_FALSE;
        state->game_state = SPACE_GAME_OVER;
        state->gameover_countdown = SPACE_GAMEOVER_COUNTDOWN_TIME;
    }
    state->player_invulnerable = SPACE_PLAYER_INVULN_TIME;
}

void space_player_take_beam_damage(SpaceBenchState *state, float amount)
{
    if (!state->player_alive || amount <= 0.0f) {
        return;
    }

    if (state->shield_active && state->shield_strength > 0.0f) {
        state->shield_strength -= amount;
        state->shield_pulse = 0.35f;
        if (state->shield_strength <= 0.0f) {
            state->shield_strength = 0.0f;
            state->shield_active = SDL_FALSE;
        }
        return;
    }

    state->player_health -= amount;
    if (state->player_health <= 0.0f) {
        state->player_health = 0.0f;
        state->player_alive = SDL_FALSE;
        state->game_state = SPACE_GAME_OVER;
        state->gameover_countdown = SPACE_GAMEOVER_COUNTDOWN_TIME;
    }
}

void space_update_drones(SpaceBenchState *state, float dt)
{
    for (int i = 0; i < state->weapon_upgrades.drone_count; ++i) {
        SpaceDrone *ship = &state->drones[i];
        SpaceTrail *trail = &state->drone_trails[i];
        if (!ship->active) {
            const float relax = SDL_min(1.0f, dt * 8.0f);
            ship->trail_offset_y += (0.0f - ship->trail_offset_y) * relax;
            ship->prev_y = state->player_y;
            space_trail_update(trail, dt, SPACE_DRONE_TRAIL_DECAY, state->scroll_speed);
            continue;
        }
        ship->angle += ship->angular_velocity * dt;
        if (ship->angle > (float)(M_PI * 2.0)) {
            ship->angle -= (float)(M_PI * 2.0);
        } else if (ship->angle < 0.0f) {
            ship->angle += (float)(M_PI * 2.0);
        }

        const float orbit_angle = ship->angle + state->player_roll;
        const float offset_x = -18.0f;
        const float offset_y = cosf(orbit_angle) * ship->radius;
        const float offset_z = sinf(orbit_angle) * ship->radius;

        ship->x = state->player_x + offset_x;
        ship->y = state->player_y + offset_y * 0.6f;
        ship->z = offset_z;

        const float drone_trail_offset = 12.0f;
        const float vertical_delta = ship->y - ship->prev_y;
        float desired_offset = vertical_delta * 6.5f;
        if (desired_offset > SPACE_DRONE_TRAIL_MAX_OFFSET) {
            desired_offset = SPACE_DRONE_TRAIL_MAX_OFFSET;
        } else if (desired_offset < -SPACE_DRONE_TRAIL_MAX_OFFSET) {
            desired_offset = -SPACE_DRONE_TRAIL_MAX_OFFSET;
        }
        const float smoothing = SDL_min(1.0f, dt * 12.0f);
        ship->trail_offset_y += (desired_offset - ship->trail_offset_y) * smoothing;

        const float drone_trail_x = ship->x - drone_trail_offset;
        const float drone_trail_y = ship->y - ship->trail_offset_y + offset_z * 0.08f;
        space_trail_push(trail, drone_trail_x, drone_trail_y);
        space_trail_update(trail, dt, SPACE_DRONE_TRAIL_DECAY, state->scroll_speed);
        ship->prev_y = ship->y;
    }
}

void space_spawn_bullet(SpaceBenchState *state, float x, float y, float vx, float vy)
{
    for (int i = 0; i < SPACE_MAX_BULLETS; ++i) {
        SpaceBullet *bullet = &state->bullets[i];
        if (!bullet->active) {
            bullet->active = SDL_TRUE;
            bullet->x = x;
            bullet->y = y;
            bullet->vx = vx;
            bullet->vy = vy;
            return;
        }
    }
}

void space_update_guided_bullets(SpaceBenchState *state, float dt)
{
    if (!state->weapon_upgrades.guidance_active) {
        return;
    }

    for (int i = 0; i < SPACE_MAX_BULLETS; ++i) {
        SpaceBullet *bullet = &state->bullets[i];
        if (!bullet->active) {
            continue;
        }

        const SpaceEnemy *closest_enemy = NULL;
        float closest_dist_sq = FLT_MAX;
        for (int e = 0; e < SPACE_MAX_ENEMIES; ++e) {
            const SpaceEnemy *enemy = &state->enemies[e];
            if (!enemy->active) {
                continue;
            }
            const float dx = enemy->x - bullet->x;
            if (dx < -16.0f) {
                continue;
            }
            const float dy = enemy->y - bullet->y;
            const float dist_sq = dx * dx + dy * dy;
            if (dist_sq < closest_dist_sq) {
                closest_dist_sq = dist_sq;
                closest_enemy = enemy;
            }
        }

        if (!closest_enemy) {
            continue;
        }

        const float dx = closest_enemy->x - bullet->x;
        const float dy = closest_enemy->y - bullet->y;
        const float len = SDL_max(8.0f, SDL_sqrtf(dx * dx + dy * dy));
        const float accel = 320.0f;
        const float ax = (dx / len) * accel;
        const float ay = (dy / len) * accel;

        bullet->vx = SDL_min(bullet->vx + ax * dt, SPACE_BULLET_SPEED * 2.2f);
        bullet->vy += ay * dt;
    }
}

void space_spawn_thumper_wave(SpaceBenchState *state)
{
    static SDL_bool lut_ready = SDL_FALSE;
    static float lut_cos[SPACE_THUMPER_SEGMENTS];
    static float lut_sin[SPACE_THUMPER_SEGMENTS];
    if (!lut_ready) {
        for (int i = 0; i < SPACE_THUMPER_SEGMENTS; ++i) {
            const float angle = (float)(M_PI * 2.0f) * (float)i / (float)SPACE_THUMPER_SEGMENTS;
            lut_cos[i] = cosf(angle);
            lut_sin[i] = sinf(angle);
        }
        lut_ready = SDL_TRUE;
    }

    const float speed = 180.0f;
    const float start_radius = state->player_radius + 6.0f;
    int cursor = state->particle_cursor % SPACE_MAX_PARTICLES;

    for (int i = 0; i < SPACE_THUMPER_SEGMENTS; ++i) {
        const float dir_x = lut_cos[i];
        const float dir_y = lut_sin[i];
        const float spawn_x = state->player_x + dir_x * start_radius;
        const float spawn_y = state->player_y + dir_y * start_radius;

        for (int search = 0; search < SPACE_MAX_PARTICLES; ++search) {
            const int idx = (cursor + search) % SPACE_MAX_PARTICLES;
            SpaceParticle *particle = &state->particles[idx];
            if (!particle->active) {
                particle->active = SDL_TRUE;
                particle->x = spawn_x;
                particle->y = spawn_y;
                particle->vx = dir_x * speed + state->scroll_speed * 0.35f;
                particle->vy = dir_y * speed;
                particle->life = particle->max_life = 0.45f;
                particle->r = 180;
                particle->g = 220;
                particle->b = 255;
                cursor = (idx + 1) % SPACE_MAX_PARTICLES;
                break;
            }
        }
    }

    state->particle_cursor = cursor;
}
