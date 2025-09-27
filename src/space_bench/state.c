#include "space_bench/state.h"

#include "common/geometry/shapes.h"

#include <float.h>
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

#define SPACE_PLAYER_BASE_SPEED 240.0f
#define SPACE_BULLET_SPEED 420.0f
#define SPACE_LASER_DURATION 0.18f
#define SPACE_ENEMY_BASE_SPEED 90.0f
#define SPACE_SCROLL_BASE 140.0f
#define SPACE_SCROLL_TARGET_MAX 260.0f
#define SPACE_EXPLOSION_LIFETIME 0.45f
#define SPACE_PLAYER_ROLL_SPEED 1.75f
#define SPACE_ENEMY_ROLL_SPEED_MIN 0.8f
#define SPACE_ENEMY_ROLL_SPEED_MAX 2.2f
#define SPACE_PLAYER_MAX_HEALTH 100.0f
#define SPACE_PLAYER_INVULN_TIME 1.1f
#define SPACE_UPGRADE_DROP_CHANCE 0.22f
#define SPACE_UPGRADE_SPEED 120.0f
#define SPACE_UPGRADE_OSC_SPEED 2.4f
#define SPACE_UPGRADE_OSC_AMPLITUDE 8.0f
#define SPACE_ENEMY_SHOT_SPEED 260.0f
#define SPACE_ENEMY_MISSILE_SPEED 120.0f
#define SPACE_ENEMY_FIRE_BASE 1.4f
#define SPACE_ENEMY_FIRE_VARIANCE 1.6f
#define SPACE_TRAIL_DECAY_RATE 0.8f
#define SPACE_DRONE_TRAIL_DECAY 0.9f
#define SPACE_DRONE_BASE_RADIUS 32.0f
#define SPACE_DRONE_RADIUS_STEP 8.0f
#define SPACE_DRONE_ANGULAR_SPEED 2.8f
#define SPACE_ENEMY_TRAIL_DECAY 3.8f
#define SPACE_ENEMY_TRAIL_EMIT_INTERVAL 0.05f
#define SPACE_PLAYER_TRAIL_MAX_OFFSET 14.0f
#define SPACE_DRONE_TRAIL_MAX_OFFSET 10.0f
#define SPACE_ANOMALY_TRIGGER_KILLS 60
#define SPACE_ANOMALY_TRIGGER_SCORE 900
#define SPACE_SHIELD_MAX_STRENGTH 50.0f
#define SPACE_GAMEOVER_COUNTDOWN_TIME 1.0f

static void space_spawn_explosion(SpaceBenchState *state, float x, float y, float radius);
static SpaceUpgradeType space_random_upgrade(SpaceBenchState *state);

static Uint32 space_rand_u32(SpaceBenchState *state)
{
    Uint32 x = state->rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    state->rng_state = (x == 0) ? 0x1234567u : x;
    return state->rng_state;
}

static float space_rand_float(SpaceBenchState *state)
{
    return (float)(space_rand_u32(state) & 0xFFFFFFu) / (float)0x1000000u;
}

static float space_rand_range(SpaceBenchState *state, float min_val, float max_val)
{
    return min_val + (max_val - min_val) * space_rand_float(state);
}

static float space_distance_sq_to_segment(float px,
                                          float py,
                                          float x1,
                                          float y1,
                                          float x2,
                                          float y2)
{
    const float dx = x2 - x1;
    const float dy = y2 - y1;
    const float len_sq = dx * dx + dy * dy;
    if (len_sq <= 0.0001f) {
        const float ddx = px - x1;
        const float ddy = py - y1;
        return ddx * ddx + ddy * ddy;
    }

    const float t = SDL_clamp(((px - x1) * dx + (py - y1) * dy) / len_sq, 0.0f, 1.0f);
    const float proj_x = x1 + t * dx;
    const float proj_y = y1 + t * dy;
    const float ddx = px - proj_x;
    const float ddy = py - proj_y;
    return ddx * ddx + ddy * ddy;
}

static void space_trail_reset(SpaceTrail *trail)
{
    if (!trail) {
        return;
    }
    trail->count = 0;
}

static void space_trail_push(SpaceTrail *trail, float x, float y)
{
    if (!trail) {
        return;
    }
    if (trail->count >= SPACE_TRAIL_POINTS) {
        SDL_memmove(&trail->points[0], &trail->points[1], sizeof(SpaceTrailPoint) * (SPACE_TRAIL_POINTS - 1));
        trail->count = SPACE_TRAIL_POINTS - 1;
    }
    SpaceTrailPoint *point = &trail->points[trail->count++];
    point->x = x;
    point->y = y;
    point->alpha = 1.0f;
}

static void space_trail_update(SpaceTrail *trail, float dt, float decay_rate, float scroll_speed)
{
    if (!trail || trail->count == 0) {
        return;
    }

    const float decay = dt * decay_rate;
    const float backward_movement = scroll_speed * dt;
    int write_idx = 0;
    for (int i = 0; i < trail->count; ++i) {
        SpaceTrailPoint *p = &trail->points[i];
        p->alpha -= decay;
        // Move trail points backward to show forward motion
        p->x -= backward_movement;
        if (p->alpha <= 0.0f || p->x < -50.0f) {
            continue;
        }
        trail->points[write_idx++] = *p;
    }
    trail->count = write_idx;
}

static void space_spawn_upgrade(SpaceBenchState *state,
                                SpaceUpgradeType type,
                                float x,
                                float y,
                                float vx)
{
    for (int i = 0; i < SPACE_MAX_UPGRADES; ++i) {
        SpaceUpgrade *upgrade = &state->upgrades[i];
        if (!upgrade->active) {
            upgrade->active = SDL_TRUE;
            upgrade->type = type;
            upgrade->x = x;
            upgrade->y = y;
            upgrade->vx = vx;
            upgrade->vy = 0.0f;
            upgrade->phase = space_rand_range(state, 0.0f, (float)(M_PI * 2.0));
            if (type == SPACE_UPGRADE_THUMPER) {
                state->thumper_dropped = SDL_TRUE;
            }
            return;
        }
    }
}

static void space_try_drop_upgrade(SpaceBenchState *state, float x, float y, float vx)
{
    if (space_rand_float(state) <= SPACE_UPGRADE_DROP_CHANCE) {
        const SpaceUpgradeType type = space_random_upgrade(state);
        space_spawn_upgrade(state, type, x, y, vx);
    }
}

static SpaceUpgradeType space_random_upgrade(SpaceBenchState *state)
{
    const SDL_bool can_drop_thumper = (!state->weapon_upgrades.thumper_active &&
                                       !state->thumper_dropped &&
                                       (state->anomaly_pending || state->anomaly.active));

    if (can_drop_thumper && space_rand_float(state) < 0.18f) {
        return SPACE_UPGRADE_THUMPER;
    }

    for (int attempt = 0; attempt < 4; ++attempt) {
        const int choice = (int)(space_rand_float(state) * SPACE_UPGRADE_COUNT) % SPACE_UPGRADE_COUNT;
        if (choice == SPACE_UPGRADE_THUMPER && !can_drop_thumper) {
            continue;
        }
        if (choice == SPACE_UPGRADE_THUMPER && state->weapon_upgrades.thumper_active) {
            continue;
        }
        return (SpaceUpgradeType)choice;
    }

    return SPACE_UPGRADE_SPLIT_CANNON;
}

static void space_apply_upgrade(SpaceBenchState *state, SpaceUpgradeType type)
{
    switch (type) {
        case SPACE_UPGRADE_SPLIT_CANNON:
            state->weapon_upgrades.split_level = SDL_min(2, state->weapon_upgrades.split_level + 1);
            break;
        case SPACE_UPGRADE_GUIDANCE:
            state->weapon_upgrades.guidance_active = SDL_TRUE;
            break;
        case SPACE_UPGRADE_DRONES:
            if (state->weapon_upgrades.drone_count < SPACE_MAX_DRONES) {
                int index = state->weapon_upgrades.drone_count++;
                SpaceDrone *ship = &state->drones[index];
                ship->active = SDL_TRUE;
                ship->radius = SPACE_DRONE_BASE_RADIUS + index * SPACE_DRONE_RADIUS_STEP;
                ship->angle = space_rand_range(state, 0.0f, (float)(M_PI * 2.0));
                ship->angular_velocity = SPACE_DRONE_ANGULAR_SPEED * (index % 2 == 0 ? 1.0f : -1.0f);
                ship->x = state->player_x;
                ship->y = state->player_y;
                ship->z = 0.0f;
                ship->prev_y = ship->y;
                ship->trail_offset_y = 0.0f;
                space_trail_reset(&state->drone_trails[index]);
            }
            break;
        case SPACE_UPGRADE_BEAM_FOCUS:
            state->weapon_upgrades.beam_scale = SDL_min(1.8f, state->weapon_upgrades.beam_scale + 0.18f);
            break;
        case SPACE_UPGRADE_SHIELD:
            state->shield_strength = SPACE_SHIELD_MAX_STRENGTH;
            state->shield_max_strength = SPACE_SHIELD_MAX_STRENGTH;
            state->shield_active = SDL_TRUE;
            state->shield_pulse = 0.0f;
            break;
        case SPACE_UPGRADE_THUMPER:
            state->weapon_upgrades.thumper_active = SDL_TRUE;
            state->weapon_upgrades.thumper_pulse_timer = 0.0f;
            state->weapon_upgrades.thumper_wave_timer = 0.0f;
            state->thumper_dropped = SDL_TRUE;
            break;
        case SPACE_UPGRADE_COUNT:
        default:
            break;
    }
}

static void space_handle_upgrade_pickups(SpaceBenchState *state)
{
    for (int i = 0; i < SPACE_MAX_UPGRADES; ++i) {
        SpaceUpgrade *upgrade = &state->upgrades[i];
        if (!upgrade->active) {
            continue;
        }
        const float dx = upgrade->x - state->player_x;
        const float dy = upgrade->y - state->player_y;
        const float dist_sq = dx * dx + dy * dy;
        const float pickup_radius = (state->player_radius + 20.0f);
        if (dist_sq <= pickup_radius * pickup_radius) {
            upgrade->active = SDL_FALSE;
            space_apply_upgrade(state, upgrade->type);
            state->score += 40;
        }
    }
}

static void space_player_take_damage(SpaceBenchState *state, float amount)
{
    if (!state->player_alive || state->player_invulnerable > 0.0f) {
        return;
    }

    // Shield absorbs damage first
    if (state->shield_active && state->shield_strength > 0.0f) {
        state->shield_strength -= amount;
        state->shield_pulse = 0.5f;  // Visual feedback
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

static void space_player_take_beam_damage(SpaceBenchState *state, float amount)
{
    if (!state->player_alive || amount <= 0.0f) {
        return;
    }

    // Shields still absorb first without triggering invulnerability cooldown
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

static void space_update_enemy_shots(SpaceBenchState *state, float dt)
{
    for (int i = 0; i < SPACE_MAX_ENEMY_SHOTS; ++i) {
        SpaceEnemyShot *shot = &state->enemy_shots[i];
        if (!shot->active) {
            continue;
        }
        shot->x += shot->vx * dt;
        shot->y += shot->vy * dt;
        shot->life -= dt;

        // Update missile trail - emit points from rear
        if (shot->is_missile) {
            shot->trail_timer += dt;
            if (shot->trail_timer >= 0.05f) {  // Emit point every 50ms
                shot->trail_timer = 0.0f;

                // Calculate rear position of missile
                const float missile_length = 8.0f;
                const float speed = SDL_sqrtf(shot->vx * shot->vx + shot->vy * shot->vy);
                if (speed > 0.0f) {
                    const float rear_x = shot->x - (shot->vx / speed) * missile_length;
                    const float rear_y = shot->y - (shot->vy / speed) * missile_length;

                    // Shift trail points
                    if (shot->trail_count >= 16) {
                        for (int t = 0; t < 15; ++t) {
                            shot->trail_points[t][0] = shot->trail_points[t + 1][0];
                            shot->trail_points[t][1] = shot->trail_points[t + 1][1];
                        }
                        shot->trail_count = 15;
                    }

                    // Add new point at rear of missile
                    shot->trail_points[shot->trail_count][0] = rear_x;
                    shot->trail_points[shot->trail_count][1] = rear_y;
                    shot->trail_count++;
                }
            }
        }

        if (shot->life <= 0.0f || shot->x < -40.0f || shot->x > SPACE_SCREEN_W + 40.0f ||
            shot->y < state->play_area_top - 40.0f || shot->y > state->play_area_bottom + 40.0f) {
            shot->active = SDL_FALSE;
            shot->damage = 0.0f;
            continue;
        }

        if (shot->is_missile && state->player_laser.is_firing) {
            const float beam_left = state->player_laser.origin_x;
            const float beam_top = state->player_laser.origin_y - 2.5f;
            const float beam_bottom = state->player_laser.origin_y + 2.5f;
            if (shot->x + 3.0f > beam_left &&
                shot->y + 3.0f > beam_top &&
                shot->y - 3.0f < beam_bottom) {
                shot->active = SDL_FALSE;
                shot->damage = 0.0f;
                space_spawn_explosion(state, shot->x, shot->y, 18.0f);
                state->score += 10;
                continue;
            }
        }

        const float dx = shot->x - state->player_x;
        const float dy = shot->y - state->player_y;
        const float hit_radius = state->player_radius + 6.0f;
        const float dist_sq = dx * dx + dy * dy;

        if (state->weapon_upgrades.thumper_active && dist_sq <= hit_radius * hit_radius) {
            const float len = SDL_max(1.0f, SDL_sqrtf(dist_sq));
            float dir_x = (len > 0.0f) ? dx / len : 1.0f;
            float dir_y = (len > 0.0f) ? dy / len : 0.0f;
            if (fabsf(dir_x) < 0.2f) {
                dir_x = (dir_x >= 0.0f) ? 0.2f : -0.2f;
            }
            const float deflect_speed = (shot->is_missile ? 280.0f : 240.0f) + state->scroll_speed * 0.35f;
            shot->vx = dir_x * deflect_speed;
            shot->vy = dir_y * (deflect_speed * 0.75f);
            shot->x = state->player_x + dir_x * (state->player_radius + 8.0f);
            shot->y = state->player_y + dir_y * (state->player_radius + 8.0f);
            shot->life = SDL_min(shot->life + 0.45f, 3.5f);
            shot->damage = SDL_max(1.0f, shot->damage * 0.5f);
            shot->is_missile = SDL_FALSE;
            space_spawn_explosion(state, shot->x, shot->y, 12.0f);
            state->weapon_upgrades.thumper_pulse_timer = 0.0f;
            continue;
        }

        if (dx * dx + dy * dy <= hit_radius * hit_radius) {
            shot->active = SDL_FALSE;
            const float damage = (shot->damage > 0.0f) ? shot->damage : (shot->is_missile ? 12.0f : 5.0f);
            shot->damage = 0.0f;
            space_player_take_damage(state, damage);
        }
    }
}

static void space_fire_enemy_shot(SpaceBenchState *state, const SpaceEnemy *enemy)
{
    for (int i = 0; i < SPACE_MAX_ENEMY_SHOTS; ++i) {
        SpaceEnemyShot *shot = &state->enemy_shots[i];
        if (!shot->active) {
            shot->active = SDL_TRUE;
            shot->x = enemy->x - enemy->radius * 0.4f;
            shot->y = enemy->y;
            shot->vx = -(state->scroll_speed + enemy->vx + SPACE_ENEMY_SHOT_SPEED);
            shot->vy = enemy->vy * 0.5f;
            shot->life = 3.0f;
            shot->is_missile = SDL_FALSE;
            shot->trail_count = 0;
            shot->damage = 5.0f;
            return;
        }
    }
}

static void space_fire_anomaly_missile(SpaceBenchState *state, float x, float y, float dx, float dy)
{
    for (int i = 0; i < SPACE_MAX_ENEMY_SHOTS; ++i) {
        SpaceEnemyShot *shot = &state->enemy_shots[i];
        if (!shot->active) {
            shot->active = SDL_TRUE;
            shot->x = x;
            shot->y = y;
            shot->vx = dx * SPACE_ENEMY_MISSILE_SPEED;
            shot->vy = dy * SPACE_ENEMY_MISSILE_SPEED;
            shot->life = 5.0f;
            shot->is_missile = SDL_TRUE;
            shot->trail_count = 0;
            shot->trail_timer = 0.0f;
            shot->damage = 12.0f;
            return;
        }
    }
}

static void space_spawn_layer_explosion(SpaceBenchState *state, float x, float y, float radius, int particles)
{
    // Create multiple small explosions for layer destruction
    for (int i = 0; i < particles; ++i) {
        const float angle = (float)(M_PI * 2.0) * (float)i / (float)particles;
        const float dist = space_rand_range(state, radius * 0.3f, radius);
        const float ex = x + cosf(angle) * dist;
        const float ey = y + sinf(angle) * dist;
        space_spawn_explosion(state, ex, ey, 15.0f);
    }
}

static void space_spawn_thumper_wave(SpaceBenchState *state)
{
    const int ring_count = 28;
    const float speed = 180.0f;
    const float start_radius = state->player_radius + 6.0f;
    for (int i = 0; i < ring_count; ++i) {
        const float angle = (float)(M_PI * 2.0f) * (float)i / (float)ring_count;
        for (int p = 0; p < SPACE_MAX_PARTICLES; ++p) {
            SpaceParticle *particle = &state->particles[p];
            if (!particle->active) {
                const float dir_x = cosf(angle);
                const float dir_y = sinf(angle);
                particle->active = SDL_TRUE;
                particle->x = state->player_x + dir_x * start_radius;
                particle->y = state->player_y + dir_y * start_radius;
                particle->vx = dir_x * speed + state->scroll_speed * 0.35f;
                particle->vy = dir_y * speed;
                particle->life = particle->max_life = 0.55f;
                particle->r = 180;
                particle->g = 220;
                particle->b = 255;
                break;
            }
        }
    }
}

static void space_handle_bullet_missile_collisions(SpaceBenchState *state)
{
    for (int b = 0; b < SPACE_MAX_BULLETS; ++b) {
        SpaceBullet *bullet = &state->bullets[b];
        if (!bullet->active) {
            continue;
        }

        for (int m = 0; m < SPACE_MAX_ENEMY_SHOTS; ++m) {
            SpaceEnemyShot *missile = &state->enemy_shots[m];
            if (!missile->active || !missile->is_missile) {
                continue;
            }

            const float dx = bullet->x - missile->x;
            const float dy = bullet->y - missile->y;
            const float collision_radius = 8.0f;  // Small collision radius
            if (dx * dx + dy * dy <= collision_radius * collision_radius) {
                // Both projectiles destroyed
                bullet->active = SDL_FALSE;
                missile->active = SDL_FALSE;
                state->score += 10;

                // Small explosion at collision point
                space_spawn_explosion(state, missile->x, missile->y, 15.0f);
                break;  // Bullet can only hit one missile
            }
        }
    }
}

static void space_update_upgrades(SpaceBenchState *state, float dt)
{
    for (int i = 0; i < SPACE_MAX_UPGRADES; ++i) {
        SpaceUpgrade *upgrade = &state->upgrades[i];
        if (!upgrade->active) {
            continue;
        }
        upgrade->x += upgrade->vx * dt;
        upgrade->y += upgrade->vy * dt;
        upgrade->phase += SPACE_UPGRADE_OSC_SPEED * dt;
        const float oscillation = sinf(upgrade->phase);
        if (upgrade->type == SPACE_UPGRADE_THUMPER) {
            upgrade->vy = oscillation * 14.0f;
            upgrade->x += cosf(upgrade->phase * 0.5f) * 6.0f * dt;

            if (space_rand_float(state) < dt * 18.0f) {
                for (int p = 0; p < SPACE_MAX_PARTICLES; ++p) {
                    SpaceParticle *particle = &state->particles[p];
                    if (!particle->active) {
                        particle->active = SDL_TRUE;
                        particle->x = upgrade->x + space_rand_range(state, -6.0f, 6.0f);
                        particle->y = upgrade->y + space_rand_range(state, -6.0f, 6.0f);
                        particle->vx = space_rand_range(state, -20.0f, 20.0f);
                        particle->vy = space_rand_range(state, -20.0f, 20.0f);
                        particle->life = particle->max_life = space_rand_range(state, 0.35f, 0.55f);
                        particle->r = 200;
                        particle->g = 240;
                        particle->b = 255;
                        break;
                    }
                }
            }
        } else {
            upgrade->vy = oscillation * 10.0f;
        }
        if (upgrade->x < -40.0f) {
            upgrade->active = SDL_FALSE;
        }
    }
}

static void space_update_drones(SpaceBenchState *state, float dt)
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

        // Add trail point behind the drone
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

static void space_spawn_bullet(SpaceBenchState *state, float x, float y, float vx, float vy)
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

static void space_update_guided_bullets(SpaceBenchState *state, float dt)
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

static void space_activate_anomaly(SpaceBenchState *state)
{
    if (state->anomaly.active) {
        return;
    }

    state->anomaly.active = SDL_TRUE;
    state->anomaly_pending = SDL_FALSE;
    state->anomaly_warning_timer = 0.0f;
    state->anomaly_warning_phase = 0.0f;
    state->thumper_dropped = SDL_FALSE;
    state->anomaly.x = (float)SPACE_SCREEN_W - 80.0f;
    state->anomaly.y = space_rand_range(state, state->play_area_top + 60.0f, state->play_area_bottom - 60.0f);
    state->anomaly.vx = -20.0f;
    state->anomaly.rotation = space_rand_range(state, 0.0f, (float)(M_PI * 2.0));
    state->anomaly.rotation_speed = space_rand_range(state, -1.2f, 1.2f);
    state->anomaly.scale = space_rand_range(state, 30.0f, 45.0f);
    state->anomaly.pulse = space_rand_range(state, 0.0f, (float)(M_PI * 2.0));
    state->anomaly.shape_index = (int)(space_rand_float(state) * SHAPE_COUNT) % SHAPE_COUNT;
    state->anomaly.render_mode = (int)(space_rand_float(state) * 3.0f);
    state->anomaly.gun_cooldown = 0.0f;
    state->anomaly.current_layer = 0;  // Start with outer layer

    // Initialize layer health
    state->anomaly.layer_health[0] = state->anomaly.layer_max_health[0] = 480.0f;  // Outer
    state->anomaly.layer_health[1] = state->anomaly.layer_max_health[1] = 640.0f;  // Mid2
    state->anomaly.layer_health[2] = state->anomaly.layer_max_health[2] = 820.0f;  // Mid1
    state->anomaly.layer_health[3] = state->anomaly.layer_max_health[3] = 1000.0f;  // Inner
    state->anomaly.layer_health[4] = state->anomaly.layer_max_health[4] = 2400.0f;  // Core

    // Initialize all weapon layers
    for (int i = 0; i < 10; ++i) {
        state->anomaly.outer_points[i].angle = (float)(M_PI * 2.0) * (float)i / 10.0f;
        state->anomaly.outer_points[i].radius = state->anomaly.scale * 1.2f;
        state->anomaly.outer_points[i].speed = 1.5f;
        state->anomaly.outer_points[i].fire_timer = space_rand_range(state, 0.0f, 2.0f);
        state->anomaly.outer_points[i].laser_timer = space_rand_range(state, 0.5f, 1.5f);
        state->anomaly.outer_points[i].active = SDL_TRUE;
    }

    for (int i = 0; i < 8; ++i) {
        state->anomaly.mid2_points[i].angle = (float)(M_PI * 2.0) * (float)i / 8.0f;
        state->anomaly.mid2_points[i].radius = state->anomaly.scale * 0.9f;
        state->anomaly.mid2_points[i].speed = -2.0f;
        state->anomaly.mid2_points[i].fire_timer = space_rand_range(state, 0.0f, 1.0f);
        state->anomaly.mid2_points[i].active = SDL_FALSE;  // Inactive until layer exposed
    }

    for (int i = 0; i < 6; ++i) {
        state->anomaly.mid1_points[i].angle = (float)(M_PI * 2.0) * (float)i / 6.0f;
        state->anomaly.mid1_points[i].radius = state->anomaly.scale * 0.65f;
        state->anomaly.mid1_points[i].speed = 2.5f;
        state->anomaly.mid1_points[i].fire_timer = space_rand_range(state, 0.0f, 1.5f);
        state->anomaly.mid1_points[i].active = SDL_FALSE;
    }

    for (int i = 0; i < 4; ++i) {
        state->anomaly.inner_points[i].angle = (float)(M_PI * 2.0) * (float)i / 4.0f;
        state->anomaly.inner_points[i].radius = state->anomaly.scale * 0.4f;
        state->anomaly.inner_points[i].speed = -3.0f;
        state->anomaly.inner_points[i].fire_timer = space_rand_range(state, 0.0f, 2.0f);
        state->anomaly.inner_points[i].laser_timer = space_rand_range(state, 1.0f, 3.0f);
        state->anomaly.inner_points[i].active = SDL_FALSE;
    }

    for (int i = 0; i < 20; ++i) {
        state->anomaly.core_points[i].angle = (float)(M_PI * 2.0) * (float)i / 20.0f;  // 18 degree spacing
        state->anomaly.core_points[i].radius = state->anomaly.scale * 0.2f;
        state->anomaly.core_points[i].speed = 3.0f;
        state->anomaly.core_points[i].fire_timer = space_rand_range(state, 0.0f, 1.0f);
        state->anomaly.core_points[i].laser_timer = space_rand_range(state, 0.0f, 2.0f);
        state->anomaly.core_points[i].active = SDL_FALSE;  // Activated when core layer reached
    }

    // Initialize rotating lasers
    for (int i = 0; i < SPACE_ANOMALY_MID_LASER_COUNT; ++i) {
        state->anomaly.lasers[i].active = SDL_FALSE;
        state->anomaly.lasers[i].length = 0.0f;
        state->anomaly.lasers[i].fade = 0.0f;
    }
    state->anomaly.mid_laser_angle = space_rand_range(state, 0.0f, (float)(M_PI * 2.0));
    state->anomaly.mid_laser_cycle_timer = 0.0f;
    state->anomaly.mid_laser_enabled = SDL_FALSE;

    // Initialize orbital points with random spiral pattern
    for (int i = 0; i < 64; ++i) {
        state->anomaly.orbital_points[i].angle = space_rand_range(state, 0.0f, (float)(M_PI * 2.0));
        state->anomaly.orbital_points[i].radius = space_rand_range(state, state->anomaly.scale * 0.3f, state->anomaly.scale * 1.5f);
        state->anomaly.orbital_points[i].angular_velocity = space_rand_range(state, -2.0f, 2.0f);
        state->anomaly.orbital_points[i].phase_offset = space_rand_range(state, 0.0f, (float)(M_PI * 2.0));
        state->anomaly.orbital_points[i].active = SDL_TRUE;
    }

    state->anomaly.core_rotation = 0.0f;
    state->anomaly.mid1_rotation = 0.0f;
    state->anomaly.mid2_rotation = 0.0f;
    state->anomaly.inner_rotation = 0.0f;
    state->anomaly.outer_rotation = 0.0f;

    // Initialize anomaly shield
    state->anomaly.shield_strength = 500.0f;
    state->anomaly.shield_max_strength = 500.0f;
    state->anomaly.shield_active = SDL_TRUE;
    state->anomaly.shield_pulse = 0.0f;

    state->anomaly_wall_timer = 3.5f;
    state->anomaly_wall_phase = 0;
}

static void space_fire_anomaly_laser_shot(SpaceBenchState *state, float x, float y, float dx, float dy)
{
    // Fire rapid laser shots as projectiles
    for (int i = 0; i < SPACE_MAX_ENEMY_SHOTS; ++i) {
        SpaceEnemyShot *shot = &state->enemy_shots[i];
        if (!shot->active) {
            shot->active = SDL_TRUE;
            shot->x = x;
            shot->y = y;
            shot->vx = dx * 300.0f;  // Fast laser projectiles
            shot->vy = dy * 300.0f;
            shot->life = 2.0f;
            shot->is_missile = SDL_FALSE;
            shot->trail_count = 0;
            shot->trail_timer = 0.0f;
            shot->damage = 6.0f;
            return;
        }
    }
}

static void space_update_anomaly_lasers(SpaceBenchState *state, float dt)
{
    // Remove old tracking laser system - now uses rapid-fire projectiles
    (void)state;
    (void)dt;
}

static void space_update_anomaly(SpaceBenchState *state, float dt)
{
    if (!state->anomaly.active) {
        return;
    }

    SpaceAnomaly *anomaly = &state->anomaly;
    anomaly->x += anomaly->vx * dt;
    anomaly->rotation += anomaly->rotation_speed * dt;
    anomaly->pulse += dt;
    anomaly->gun_cooldown -= dt;

    // Update layer rotations
    anomaly->core_rotation += 5.0f * dt;
    anomaly->inner_rotation -= 1.8f * dt;
    anomaly->mid1_rotation += 1.6f * dt;
    anomaly->mid2_rotation -= 2.0f * dt;
    anomaly->outer_rotation += 1.5f * dt;

    // Update shield pulse animation
    if (anomaly->shield_active) {
        anomaly->shield_pulse += dt * 3.0f;
    }

    // Keep anomaly on right side of screen
    if (anomaly->x < SPACE_SCREEN_W * 0.7f) {
        anomaly->vx = SDL_max(anomaly->vx, 0.0f);
    }

    for (int i = 0; i < SPACE_ANOMALY_MID_LASER_COUNT; ++i) {
        AnomalyLaser *laser = &anomaly->lasers[i];
        laser->active = SDL_FALSE;
        laser->fade = SDL_max(0.0f, laser->fade - dt * 3.0f);
    }

    // Update weapons based on current active layer
    const SDL_bool outer_fire = (anomaly->current_layer == 0) || (anomaly->current_layer >= 4);
    const SDL_bool mid2_fire = (anomaly->current_layer == 1) || (anomaly->current_layer >= 4);
    const SDL_bool mid1_fire = (anomaly->current_layer >= 2);

    if (outer_fire) {
        for (int i = 0; i < 10; ++i) {
            AnomalyWeaponPoint *point = &anomaly->outer_points[i];
            if (!point->active) continue;
            point->fire_timer -= dt;

            const float angle = point->angle + anomaly->outer_rotation;
            const float gun_x = anomaly->x + cosf(angle) * point->radius;
            const float gun_y = anomaly->y + sinf(angle) * point->radius;

            if (point->fire_timer <= 0.0f && state->player_alive) {
                const float dx = state->player_x - gun_x;
                const float dy = state->player_y - gun_y;
                const float len = SDL_max(1.0f, SDL_sqrtf(dx * dx + dy * dy));
                space_fire_anomaly_missile(state, gun_x, gun_y, dx / len, dy / len);
                float cooldown = space_rand_range(state, 1.5f, 3.0f);
                if (anomaly->current_layer >= 4) {
                    cooldown *= 0.65f;  // More aggressive at core
                }
                point->fire_timer = cooldown;
            }
        }
    }

    if (mid2_fire) {
        for (int i = 0; i < 8; ++i) {
            AnomalyWeaponPoint *point = &anomaly->mid2_points[i];
            if (!point->active) continue;
            point->fire_timer -= dt;

            const float angle = point->angle + anomaly->mid2_rotation;
            const float gun_x = anomaly->x + cosf(angle) * point->radius;
            const float gun_y = anomaly->y + sinf(angle) * point->radius;

            if (point->fire_timer <= 0.0f && state->player_alive) {
                for (int s = 0; s < SPACE_MAX_ENEMY_SHOTS; ++s) {
                    SpaceEnemyShot *shot = &state->enemy_shots[s];
                    if (!shot->active) {
                        const float dx = state->player_x - gun_x;
                        const float dy = state->player_y - gun_y;
                        const float len = SDL_max(1.0f, SDL_sqrtf(dx * dx + dy * dy));
                        shot->active = SDL_TRUE;
                        shot->x = gun_x;
                        shot->y = gun_y;
                        const float speed = (anomaly->current_layer >= 4) ? 260.0f : 200.0f;
                        shot->vx = (dx / len) * speed;
                        shot->vy = (dy / len) * speed;
                        shot->life = 3.0f;
                        shot->is_missile = SDL_FALSE;
                        shot->damage = 5.0f;
                        break;
                    }
                }
                float cooldown = space_rand_range(state, 0.9f, 1.6f);
                if (anomaly->current_layer >= 4) {
                    cooldown *= 0.6f;
                }
                point->fire_timer = cooldown;
            }
        }
    }

    if (mid1_fire) {
        const float radius = anomaly->mid1_points[0].radius;
        const float spacing = (float)(M_PI * 2.0f) / (float)SPACE_ANOMALY_MID_LASER_COUNT;
        const float beam_length = (float)SPACE_SCREEN_W * (anomaly->current_layer >= 4 ? 1.4f : 1.1f);
        const float rotation_speed = (anomaly->current_layer >= 4) ? 0.7f : 0.5f;
        anomaly->mid_laser_angle += dt * rotation_speed;
        anomaly->mid_laser_cycle_timer += dt;
        while (anomaly->mid_laser_cycle_timer >= 3.0f) {
            anomaly->mid_laser_cycle_timer -= 3.0f;
        }

        const SDL_bool lasers_enabled = (anomaly->mid_laser_cycle_timer >= 1.0f);
        anomaly->mid_laser_enabled = lasers_enabled;

        for (int i = 0; i < SPACE_ANOMALY_MID_LASER_COUNT; ++i) {
            AnomalyLaser *laser = &anomaly->lasers[i];
            const float angle = anomaly->mid_laser_angle + spacing * (float)i;
            const float cos_a = cosf(angle);
            const float sin_a = sinf(angle);
            const float origin_x = anomaly->x + cos_a * radius;
            const float origin_y = anomaly->y + sin_a * radius;

            const float target_fade = lasers_enabled ? 1.0f : 0.0f;
            const float fade_speed = lasers_enabled ? 3.5f : 3.0f;
            if (target_fade > laser->fade) {
                laser->fade = SDL_min(target_fade, laser->fade + dt * fade_speed);
            } else {
                laser->fade = SDL_max(target_fade, laser->fade - dt * fade_speed);
            }

            laser->x = origin_x;
            laser->y = origin_y;
            laser->target_x = origin_x + cos_a * beam_length;
            laser->target_y = origin_y + sin_a * beam_length;
            laser->length = beam_length;
            laser->duration = 0.0f;
            laser->active = (laser->fade > 0.01f);
        }
    } else {
        for (int i = 0; i < SPACE_ANOMALY_MID_LASER_COUNT; ++i) {
            anomaly->lasers[i].active = SDL_FALSE;
            anomaly->lasers[i].fade = 0.0f;
        }
        anomaly->mid_laser_cycle_timer = 0.0f;
        anomaly->mid_laser_enabled = SDL_FALSE;
    }

    // Laser firing from all active layers
    if (anomaly->current_layer >= 0 && state->player_alive) {
        // Outer layer radial pattern (always active, rotating guns)
        for (int i = 0; i < 10; ++i) {
            AnomalyWeaponPoint *point = &anomaly->outer_points[i];
            if (!point->active) continue;
            point->laser_timer -= dt;

            const float angle = point->angle + anomaly->outer_rotation;
            const float gun_x = anomaly->x + cosf(angle) * point->radius;
            const float gun_y = anomaly->y + sinf(angle) * point->radius;

            if (point->laser_timer <= 0.0f) {
                // Fire radially outward in rotating pattern (not toward player)
                const float dx = cosf(angle);
                const float dy = sinf(angle);
                space_fire_anomaly_laser_shot(state, gun_x, gun_y, dx, dy);
                point->laser_timer = space_rand_range(state, 0.5f, 0.9f);  // Moderate fire rate
            }
        }

        // Inner layer rapid lasers (when exposed)
        if (anomaly->current_layer >= 3) {
            for (int i = 0; i < 4; ++i) {
                AnomalyWeaponPoint *point = &anomaly->inner_points[i];
                if (!point->active) continue;
                point->laser_timer -= dt;

                const float angle = point->angle + anomaly->inner_rotation;
                const float gun_x = anomaly->x + cosf(angle) * point->radius;
                const float gun_y = anomaly->y + sinf(angle) * point->radius;

                if (point->laser_timer <= 0.0f) {
                    const float dx = state->player_x - gun_x;
                    const float dy = state->player_y - gun_y;
                    const float len = SDL_max(1.0f, SDL_sqrtf(dx * dx + dy * dy));
                    space_fire_anomaly_laser_shot(state, gun_x, gun_y, dx / len, dy / len);
                    point->laser_timer = space_rand_range(state, 0.1f, 0.3f);
                }
            }
        }

        // Core layer radial pattern firing (when exposed)
        if (anomaly->current_layer >= 4) {
            for (int i = 0; i < 20; ++i) {
                AnomalyWeaponPoint *point = &anomaly->core_points[i];
                if (!point->active) continue;
                point->laser_timer -= dt;

                const float angle = point->angle + anomaly->core_rotation;
                const float gun_x = anomaly->x + cosf(angle) * point->radius;
                const float gun_y = anomaly->y + sinf(angle) * point->radius;

                if (point->laser_timer <= 0.0f) {
                    // Fire radially outward from core
                    const float dx = cosf(angle);
                    const float dy = sinf(angle);
                    space_fire_anomaly_laser_shot(state, gun_x, gun_y, dx, dy);
                    point->laser_timer = space_rand_range(state, 0.3f, 0.7f);  // Moderate fire rate
                }

                // Core layer rapid fire projectiles
                point->fire_timer -= dt;
                if (point->fire_timer <= 0.0f) {
                    for (int s = 0; s < SPACE_MAX_ENEMY_SHOTS; ++s) {
                        SpaceEnemyShot *shot = &state->enemy_shots[s];
                        if (!shot->active) {
                            const float dx = state->player_x - gun_x;
                            const float dy = state->player_y - gun_y;
                            const float base_angle = atan2f(dy, dx);
                            const float spread = space_rand_range(state, -0.25f, 0.25f);
                            const float angle = base_angle + spread;
                            const float speed = 260.0f + space_rand_range(state, -20.0f, 20.0f);
                            shot->active = SDL_TRUE;
                            shot->x = gun_x;
                            shot->y = gun_y;
                            shot->vx = cosf(angle) * speed;
                            shot->vy = sinf(angle) * speed;
                            shot->life = 3.0f;
                            shot->is_missile = SDL_FALSE;
                            shot->damage = 3.0f;
                            break;
                        }
                    }
                    point->fire_timer = space_rand_range(state, 0.15f, 0.4f);  // Rapid fire
                }
            }
        }
    }

    // Update orbital points
    for (int i = 0; i < 64; ++i) {
        AnomalyOrbitalPoint *point = &anomaly->orbital_points[i];
        if (!point->active) continue;

        point->angle += point->angular_velocity * dt;
        if (point->angle > (float)(M_PI * 2.0)) {
            point->angle -= (float)(M_PI * 2.0);
        } else if (point->angle < 0.0f) {
            point->angle += (float)(M_PI * 2.0);
        }

        // Add spiraling effect by slowly changing radius
        point->radius += sinf(anomaly->pulse + point->phase_offset) * 2.0f * dt;
        point->radius = SDL_max(anomaly->scale * 0.2f, SDL_min(anomaly->scale * 1.8f, point->radius));
    }

    // Update tracking lasers
    space_update_anomaly_lasers(state, dt);

    // Check if anomaly should be destroyed due to going off screen
    if (anomaly->x < -100.0f) {
        anomaly->active = SDL_FALSE;
    }
}

static void space_apply_anomaly_laser_damage(SpaceBenchState *state, float dt)
{
    if (!state->anomaly.active || state->anomaly.current_layer < 2) {
        return;
    }

    const float hit_radius = state->player_radius + SPACE_ANOMALY_LASER_THICKNESS;
    const float hit_radius_sq = hit_radius * hit_radius;

    for (int i = 0; i < SPACE_ANOMALY_MID_LASER_COUNT; ++i) {
        const AnomalyLaser *laser = &state->anomaly.lasers[i];
        if (!laser->active) {
            continue;
        }

        const float dist_sq = space_distance_sq_to_segment(state->player_x,
                                                           state->player_y,
                                                           laser->x,
                                                           laser->y,
                                                           laser->target_x,
                                                           laser->target_y);
        if (dist_sq <= hit_radius_sq) {
            const float fade = SDL_clamp(laser->fade, 0.2f, 1.0f);
            space_player_take_beam_damage(state, SPACE_ANOMALY_LASER_DPS * fade * dt);
        }
    }
}

static void space_handle_anomaly_hits(SpaceBenchState *state, float dt)
{
    if (!state->anomaly.active) {
        return;
    }

    SpaceAnomaly *anomaly = &state->anomaly;
    const float radius = anomaly->scale * (1.2f - anomaly->current_layer * 0.2f);
    const float radius_sq = radius * radius;
    float damage_taken = 0.0f;

    // Check bullet hits
    for (int i = 0; i < SPACE_MAX_BULLETS; ++i) {
        SpaceBullet *bullet = &state->bullets[i];
        if (!bullet->active) {
            continue;
        }
        const float dx = anomaly->x - bullet->x;
        const float dy = anomaly->y - bullet->y;
        if (dx * dx + dy * dy <= radius_sq) {
            bullet->active = SDL_FALSE;
            damage_taken += 12.0f;
            space_spawn_explosion(state, bullet->x, bullet->y, 20.0f);
        }
    }

    // Check player laser beam hits
    if (state->player_laser.is_firing) {
        const float beam_top = state->player_laser.origin_y - 1.5f;
        const float beam_bottom = state->player_laser.origin_y + 1.5f;

        // Check if anomaly intersects with beam path
        if (anomaly->x + radius > state->player_laser.origin_x &&
            anomaly->y + radius > beam_top &&
            anomaly->y - radius < beam_bottom) {
            damage_taken += 220.0f * dt;
        }
    }

    // Check drone laser beam hits
    for (int d = 0; d < state->weapon_upgrades.drone_count; ++d) {
        if (state->drone_lasers[d].is_firing) {
            const float beam_top = state->drone_lasers[d].origin_y - 1.0f;
            const float beam_bottom = state->drone_lasers[d].origin_y + 1.0f;

            // Check if anomaly intersects with drone beam path
            if (anomaly->x + radius > state->drone_lasers[d].origin_x &&
                anomaly->y + radius > beam_top &&
                anomaly->y - radius < beam_bottom) {
                damage_taken += 180.0f * dt;  // Drone beams do less damage
            }
        }
    }

    // Apply damage to current layer (shield absorbs first)
    if (damage_taken > 0.0f && anomaly->current_layer < 5) {
        // Shield absorbs damage first
        if (anomaly->shield_active && anomaly->shield_strength > 0.0f) {
            const float shield_absorbed = SDL_min(damage_taken, anomaly->shield_strength);
            anomaly->shield_strength -= shield_absorbed;
            damage_taken -= shield_absorbed;

            if (anomaly->shield_strength <= 0.0f) {
                anomaly->shield_active = SDL_FALSE;
                space_spawn_explosion(state, anomaly->x, anomaly->y, anomaly->scale * 1.5f);
            }
        }

        // Remaining damage goes to layer health
        if (damage_taken > 0.0f) {
            anomaly->layer_health[anomaly->current_layer] -= damage_taken;
        }

        // Check if layer destroyed
        if (anomaly->layer_health[anomaly->current_layer] <= 0.0f) {
            // Create layer explosion
            space_spawn_layer_explosion(state, anomaly->x, anomaly->y, radius, 8);
            state->score += 100 * (anomaly->current_layer + 1);

            // Activate next layer
            anomaly->current_layer++;

            if (anomaly->current_layer == 1) {
                // Activate mid2 layer
                for (int i = 0; i < 8; ++i) {
                    anomaly->mid2_points[i].active = SDL_TRUE;
                }
            } else if (anomaly->current_layer == 2) {
                // Activate mid1 layer
                for (int i = 0; i < 6; ++i) {
                    anomaly->mid1_points[i].active = SDL_TRUE;
                }
            } else if (anomaly->current_layer == 3) {
                // Activate inner layer
                for (int i = 0; i < 4; ++i) {
                    anomaly->inner_points[i].active = SDL_TRUE;
                }
                if (!state->weapon_upgrades.thumper_active && !state->thumper_dropped) {
                    space_spawn_upgrade(state, SPACE_UPGRADE_THUMPER, anomaly->x, anomaly->y, 0.0f);
                    space_spawn_explosion(state, anomaly->x, anomaly->y, anomaly->scale * 0.6f);
                }
            } else if (anomaly->current_layer == 4) {
                // Core exposed - visual only
                for (int i = 0; i < 20; ++i) {
                    anomaly->core_points[i].active = SDL_TRUE;
                }
            } else if (anomaly->current_layer >= 5) {
                // Anomaly destroyed - don't respawn immediately
                anomaly->active = SDL_FALSE;
                state->score += 1000;
                space_spawn_layer_explosion(state, anomaly->x, anomaly->y, anomaly->scale * 2.0f, 16);
                // Reset trigger requirements and add cooldown to prevent immediate respawn
                state->enemies_destroyed = 0;
                state->score = SDL_max(0, state->score - SPACE_ANOMALY_TRIGGER_SCORE);
                state->anomaly_cooldown_timer = 25.0f;  // Longer cooldown before next anomaly
                state->anomaly_wall_timer = 0.0f;
                state->anomaly_wall_phase = 0;
                state->anomaly_pending = SDL_FALSE;
                state->anomaly_warning_timer = 0.0f;
                state->anomaly_warning_phase = 0.0f;
            }
        }
    }

    // Note: Laser system now uses rapid-fire projectiles instead of beams
}

static void space_spawn_star(SpaceBenchState *state, int index, float min_x)
{
    state->star_x[index] = min_x + space_rand_range(state, 0.0f, 80.0f);
    state->star_y[index] = space_rand_range(state, state->play_area_top + 4.0f, state->play_area_bottom - 4.0f);
    state->star_speed[index] = space_rand_range(state, 0.35f, 1.25f);
    state->star_brightness[index] = (Uint8)(220 - (Uint8)(state->star_speed[index] * 80.0f));
}

static void space_spawn_enemy_formation(SpaceBenchState *state)
{
    // Randomize formation size (1-5 enemies)
    const int formation_size = 1 + (space_rand_u32(state) % 5);
    const float base_x = (float)SPACE_SCREEN_W + space_rand_range(state, 20.0f, 80.0f);
    const float base_y = space_rand_range(state, state->play_area_top + 30.0f, state->play_area_bottom - 30.0f);
    const float base_speed = space_rand_range(state, SPACE_ENEMY_BASE_SPEED, SPACE_ENEMY_BASE_SPEED + 60.0f);

    int spawned = 0;
    for (int i = 0; i < SPACE_MAX_ENEMIES && spawned < formation_size; ++i) {
        SpaceEnemy *enemy = &state->enemies[i];
        if (!enemy->active) {
            enemy->active = SDL_TRUE;
            enemy->radius = space_rand_range(state, 6.0f, 10.0f);
            enemy->trail_emit_timer = 0.0f;
            space_trail_reset(&state->enemy_trails[i]);

            // Formation patterns
            if (formation_size == 1) {
                // Single enemy
                enemy->x = base_x;
                enemy->y = base_y;
            } else if (formation_size <= 3) {
                // Line formation
                enemy->x = base_x + (spawned * space_rand_range(state, 25.0f, 45.0f));
                enemy->y = base_y + (spawned * space_rand_range(state, -15.0f, 15.0f));
            } else {
                // V-formation or scattered
                const float angle = (float)(M_PI * 2.0) * (float)spawned / (float)formation_size;
                const float offset = space_rand_range(state, 20.0f, 50.0f);
                enemy->x = base_x + cosf(angle) * offset;
                enemy->y = base_y + sinf(angle) * offset * 0.5f;
            }

            enemy->vx = base_speed + space_rand_range(state, -20.0f, 20.0f);
            enemy->vy = space_rand_range(state, -25.0f, 25.0f);
            enemy->hp = SDL_max(1.0f, enemy->radius * 0.08f);
            enemy->rotation = space_rand_range(state, 0.0f, (float)(M_PI * 2.0));
            float spin = space_rand_range(state, SPACE_ENEMY_ROLL_SPEED_MIN, SPACE_ENEMY_ROLL_SPEED_MAX);
            if (space_rand_u32(state) & 1u) {
                spin = -spin;
            }
            enemy->rotation_speed = spin;
            enemy->fire_interval = space_rand_range(state, SPACE_ENEMY_FIRE_BASE, SPACE_ENEMY_FIRE_BASE + SPACE_ENEMY_FIRE_VARIANCE);
            enemy->fire_interval = SDL_max(0.15f, enemy->fire_interval * 0.5f);
            enemy->fire_timer = enemy->fire_interval * space_rand_range(state, 0.2f, 0.8f);
            spawned++;
        }
    }
}

static void space_spawn_enemy_wall(SpaceBenchState *state, SDL_bool top_half)
{
    const float margin = 24.0f;
    const float mid_line = (state->play_area_top + state->play_area_bottom) * 0.5f;
    const float min_y = top_half ? state->play_area_top + margin : mid_line + margin;
    const float max_y = top_half ? mid_line - margin : state->play_area_bottom - margin;
    const int wall_count = 6;
    if (max_y <= min_y) {
        return;
    }

    const float spacing = (max_y - min_y) / (float)wall_count;
    const float spawn_x = (float)SPACE_SCREEN_W + 40.0f;

    for (int idx = 0; idx < wall_count; ++idx) {
        for (int i = 0; i < SPACE_MAX_ENEMIES; ++i) {
            SpaceEnemy *enemy = &state->enemies[i];
            if (enemy->active) {
                continue;
            }

            enemy->active = SDL_TRUE;
            enemy->radius = 8.0f;
            enemy->x = spawn_x + space_rand_range(state, -6.0f, 6.0f);
            enemy->y = min_y + spacing * ((float)idx + 0.5f);
            enemy->vx = SPACE_ENEMY_BASE_SPEED + 60.0f;
            enemy->vy = 0.0f;
            enemy->hp = 3.0f;
            enemy->rotation = 0.0f;
            enemy->rotation_speed = space_rand_range(state, -1.2f, 1.2f);
            enemy->fire_interval = space_rand_range(state, 1.2f, 1.8f);
            enemy->fire_timer = enemy->fire_interval * space_rand_range(state, 0.1f, 0.6f);
            enemy->trail_emit_timer = 0.0f;
            space_trail_reset(&state->enemy_trails[i]);
            break;
        }
    }
}

static void space_spawn_explosion(SpaceBenchState *state, float x, float y, float radius)
{
    for (int i = 0; i < SPACE_MAX_EXPLOSIONS; ++i) {
        SpaceExplosion *exp = &state->explosions[i];
        if (!exp->active) {
            exp->active = SDL_TRUE;
            exp->x = x;
            exp->y = y;
            exp->radius = radius;
            exp->timer = 0.0f;
            exp->lifetime = SPACE_EXPLOSION_LIFETIME;
            return;
        }
    }
}

static void space_spawn_firing_particles(SpaceBenchState *state, float x, float y, SDL_bool is_laser)
{
    const int count = is_laser ? 6 : 3;
    const Uint8 base_r = is_laser ? 210 : 230;
    const Uint8 base_g = is_laser ? 220 : 170;
    const Uint8 base_b = is_laser ? 255 : 90;
    for (int p = 0; p < count; ++p) {
        for (int i = 0; i < SPACE_MAX_PARTICLES; ++i) {
            SpaceParticle *particle = &state->particles[i];
            if (!particle->active) {
                particle->active = SDL_TRUE;
                particle->x = x + space_rand_range(state, -5.0f, 5.0f);
                particle->y = y + space_rand_range(state, -3.0f, 3.0f);
                particle->r = base_r;
                particle->g = base_g;
                particle->b = base_b;

                if (is_laser) {
                    // Laser particles - bright and fast
                    particle->vx = space_rand_range(state, 80.0f, 150.0f);
                    particle->vy = space_rand_range(state, -20.0f, 20.0f);
                    particle->life = particle->max_life = space_rand_range(state, 0.3f, 0.6f);
                } else {
                    // Gun particles - smaller and shorter
                    particle->vx = space_rand_range(state, 60.0f, 120.0f);
                    particle->vy = space_rand_range(state, -15.0f, 15.0f);
                    particle->life = particle->max_life = space_rand_range(state, 0.2f, 0.4f);
                }
                break;
            }
        }
    }
}

static void space_update_particles(SpaceBenchState *state, float dt)
{
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

static void space_reset_input_triggers(SpaceBenchState *state)
{
    state->input.fire_gun = SDL_FALSE;
}

static void space_update_stars(SpaceBenchState *state, float dt)
{
    const float base = state->scroll_speed * dt;

#ifdef __ARM_NEON
    const float32x4_t base_vec = vdupq_n_f32(base);
    int i = 0;
    for (; i <= SPACE_STAR_COUNT - 4; i += 4) {
        float32x4_t xs = vld1q_f32(&state->star_x[i]);
        float32x4_t speeds = vld1q_f32(&state->star_speed[i]);
        xs = vsubq_f32(xs, vmulq_f32(speeds, base_vec));
        vst1q_f32(&state->star_x[i], xs);
    }
#else
    int i = SPACE_STAR_COUNT; /* dummy to silence unused warning */
    (void)i;
#endif

    for (int idx = 0; idx < SPACE_STAR_COUNT; ++idx) {
        state->star_x[idx] -= state->star_speed[idx] * base;
        if (state->star_x[idx] < -4.0f) {
            space_spawn_star(state, idx, (float)SPACE_SCREEN_W + 4.0f);
        }
    }
}

static void space_handle_projectile_hits(SpaceBenchState *state, SpaceEnemy *enemy)
{
    for (int b = 0; b < SPACE_MAX_BULLETS; ++b) {
        SpaceBullet *bullet = &state->bullets[b];
        if (!bullet->active) {
            continue;
        }
        const float dx = enemy->x - bullet->x;
        const float dy = enemy->y - bullet->y;
        const float dist_sq = dx * dx + dy * dy;
        const float hit_radius = enemy->radius + SPACE_BULLET_RADIUS * 0.5f;
        if (dist_sq <= hit_radius * hit_radius) {
            bullet->active = SDL_FALSE;
            enemy->hp -= 1.0f;
            state->score += 5;
            if (enemy->hp <= 0.0f) {
                enemy->active = SDL_FALSE;
                state->score += 20;
                state->enemies_destroyed++;
                state->total_enemies_killed++;
                state->target_scroll_speed = SDL_min(SPACE_SCROLL_TARGET_MAX, state->target_scroll_speed + 6.0f);
                space_spawn_explosion(state, enemy->x, enemy->y, enemy->radius * 1.0f);
                space_try_drop_upgrade(state,
                                       enemy->x,
                                       enemy->y,
                                       -(state->scroll_speed + enemy->vx) * 0.4f);
                return;
            }
        }
    }

    // Check player laser beam collision
    if (state->player_laser.is_firing) {
        const float beam_top = state->player_laser.origin_y - 1.5f;
        const float beam_bottom = state->player_laser.origin_y + 1.5f;

        // Check if enemy is in the beam path (from laser origin to screen edge)
        if (enemy->x + enemy->radius > state->player_laser.origin_x &&
            enemy->y + enemy->radius > beam_top &&
            enemy->y - enemy->radius < beam_bottom) {

            enemy->active = SDL_FALSE;
            state->score += 30;
            state->enemies_destroyed++;
            state->target_scroll_speed = SDL_min(SPACE_SCROLL_TARGET_MAX, state->target_scroll_speed + 8.0f);
            space_spawn_explosion(state, enemy->x, enemy->y, enemy->radius * 1.2f);
            space_try_drop_upgrade(state,
                                   enemy->x,
                                   enemy->y,
                                   -(state->scroll_speed + enemy->vx) * 0.5f);
            return;  // Enemy destroyed, no need to check drone lasers
        }
    }

    // Check drone laser beam collisions
    for (int d = 0; d < state->weapon_upgrades.drone_count; ++d) {
        if (state->drone_lasers[d].is_firing) {
            const float beam_top = state->drone_lasers[d].origin_y - 1.0f;
            const float beam_bottom = state->drone_lasers[d].origin_y + 1.0f;

            // Check if enemy is in the drone beam path
            if (enemy->x + enemy->radius > state->drone_lasers[d].origin_x &&
                enemy->y + enemy->radius > beam_top &&
                enemy->y - enemy->radius < beam_bottom) {

                enemy->active = SDL_FALSE;
                state->score += 25;  // Slightly less score for drone kills
                state->enemies_destroyed++;
                state->total_enemies_killed++;
                state->target_scroll_speed = SDL_min(SPACE_SCROLL_TARGET_MAX, state->target_scroll_speed + 6.0f);
                space_spawn_explosion(state, enemy->x, enemy->y, enemy->radius * 1.1f);
                space_try_drop_upgrade(state,
                                       enemy->x,
                                       enemy->y,
                                       -(state->scroll_speed + enemy->vx) * 0.4f);
                return;  // Enemy destroyed
            }
        }
    }
}

void space_state_init(SpaceBenchState *state)
{
    if (!state) {
        return;
    }

    SDL_memset(state, 0, sizeof(*state));

    state->player_x = 120.0f;
    state->player_y = SPACE_SCREEN_H * 0.5f;
    state->player_prev_x = state->player_x;
    state->player_prev_y = state->player_y;
    state->player_trail_offset_y = 0.0f;
    state->player_speed = SPACE_PLAYER_BASE_SPEED;
    state->player_radius = 8.0f;
    state->player_roll = 0.0f;
    state->player_roll_speed = SPACE_PLAYER_ROLL_SPEED;
    state->player_max_health = SPACE_PLAYER_MAX_HEALTH;
    state->player_health = state->player_max_health;
    state->player_invulnerable = 0.0f;
    state->player_alive = SDL_TRUE;

    state->scroll_speed = SPACE_SCROLL_BASE;
    state->target_scroll_speed = SPACE_SCROLL_BASE + 20.0f;

    state->gun_cooldown = 0.0f;
    state->laser_cooldown = 0.0f;
    state->laser_hold_timer = 0.0f;
    state->laser_recharge_timer = 0.0f;

    state->anomaly_cooldown_timer = 20.0f;
    state->anomaly_wall_timer = 0.0f;
    state->anomaly_wall_phase = 0;

    state->spawn_interval = 2.4f;
    state->min_spawn_interval = 0.6f;
    state->spawn_timer = 1.0f;

    state->rng_state = SDL_GetPerformanceCounter() | 1u;

    state->play_area_top = 0.0f;
    state->play_area_bottom = (float)SPACE_SCREEN_H;

    state->game_state = SPACE_GAME_PLAYING;
    state->gameover_selected = SPACE_GAMEOVER_RETRY;

    for (int i = 0; i < SPACE_STAR_COUNT; ++i) {
        space_spawn_star(state, i, space_rand_range(state, 0.0f, (float)SPACE_SCREEN_W));
    }

    for (int i = 0; i < SPACE_MAX_BULLETS; ++i) {
        state->bullets[i].active = SDL_FALSE;
        state->bullets[i].vx = 0.0f;
        state->bullets[i].vy = 0.0f;
    }

    // Initialize laser beams
    state->player_laser.is_firing = SDL_FALSE;
    state->player_laser.intensity = 1.0f;

    for (int i = 0; i < SPACE_MAX_DRONES; ++i) {
        state->drone_lasers[i].is_firing = SDL_FALSE;
        state->drone_lasers[i].intensity = 1.0f;
    }

    for (int i = 0; i < SPACE_MAX_ENEMY_SHOTS; ++i) {
        state->enemy_shots[i].active = SDL_FALSE;
        state->enemy_shots[i].is_missile = SDL_FALSE;
        state->enemy_shots[i].trail_count = 0;
        state->enemy_shots[i].trail_timer = 0.0f;
        state->enemy_shots[i].damage = 0.0f;
    }

    for (int i = 0; i < SPACE_MAX_UPGRADES; ++i) {
        state->upgrades[i].active = SDL_FALSE;
    }

    for (int i = 0; i < SPACE_MAX_PARTICLES; ++i) {
        state->particles[i].active = SDL_FALSE;
    }

    state->weapon_upgrades.split_level = 0;
    state->weapon_upgrades.guidance_active = SDL_FALSE;
    state->weapon_upgrades.drone_count = 0;
    state->weapon_upgrades.beam_scale = 1.0f;
    state->weapon_upgrades.thumper_active = SDL_FALSE;
    state->weapon_upgrades.thumper_pulse_timer = 0.0f;
    state->weapon_upgrades.thumper_wave_timer = 0.0f;

    for (int i = 0; i < SPACE_MAX_DRONES; ++i) {
        state->drones[i].active = SDL_FALSE;
        state->drones[i].prev_y = state->player_y;
        state->drones[i].trail_offset_y = 0.0f;
        space_trail_reset(&state->drone_trails[i]);
    }
    space_trail_reset(&state->player_trail);
    for (int i = 0; i < SPACE_MAX_ENEMIES; ++i) {
        space_trail_reset(&state->enemy_trails[i]);
        state->enemies[i].trail_emit_timer = 0.0f;
    }

    state->anomaly.active = SDL_FALSE;
    state->time_accumulator = 0.0f;
    state->anomaly_pending = SDL_FALSE;
    state->anomaly_warning_timer = 0.0f;
    state->anomaly_warning_phase = 0.0f;
    state->thumper_dropped = SDL_FALSE;

    state->shield_strength = 0.0f;
    state->shield_max_strength = 0.0f;
    state->shield_active = SDL_FALSE;
    state->shield_pulse = 0.0f;

    state->gameover_countdown = 0.0f;
}

void space_state_update_layout(SpaceBenchState *state, int overlay_height)
{
    if (!state) {
        return;
    }

    state->play_area_top = (float)overlay_height + 6.0f;
    state->play_area_bottom = (float)SPACE_SCREEN_H - 6.0f;
}

void space_state_update(SpaceBenchState *state, float dt)
{
    if (!state || dt <= 0.0f) {
        space_reset_input_triggers(state);
        return;
    }

    // Handle game over state separately
    if (state->game_state == SPACE_GAME_OVER) {
        if (state->gameover_countdown > 0.0f) {
            state->gameover_countdown -= dt;
            if (state->gameover_countdown < 0.0f) {
                state->gameover_countdown = 0.0f;
            }
        }
        space_reset_input_triggers(state);
        return;
    }

    space_update_stars(state, dt);
    state->time_accumulator += dt;

    state->scroll_speed += (state->target_scroll_speed - state->scroll_speed) * SDL_min(1.0f, dt * 0.85f);

    const float move_x = (state->input.right ? 1.0f : 0.0f) - (state->input.left ? 1.0f : 0.0f);
    const float move_y = (state->input.down ? 1.0f : 0.0f) - (state->input.up ? 1.0f : 0.0f);

    state->player_x += move_x * state->player_speed * dt;
    state->player_y += move_y * state->player_speed * dt;
    state->player_roll += state->player_roll_speed * dt;
    if (state->player_roll > (float)(M_PI * 2.0)) {
        state->player_roll -= (float)(M_PI * 2.0);
    }

    if (state->player_invulnerable > 0.0f) {
        state->player_invulnerable = SDL_max(0.0f, state->player_invulnerable - dt);
    }

    // Update shield pulse effect
    if (state->shield_pulse > 0.0f) {
        state->shield_pulse -= dt * 2.0f;
        if (state->shield_pulse < 0.0f) {
            state->shield_pulse = 0.0f;
        }
    }

    const float min_x = 20.0f;
    const float max_x = (float)SPACE_SCREEN_W - 20.0f;
    const float min_y = state->play_area_top + 20.0f;
    const float max_y = state->play_area_bottom - 20.0f;
    if (state->player_x < min_x) state->player_x = min_x;
    if (state->player_x > max_x) state->player_x = max_x;
    if (state->player_y < min_y) state->player_y = min_y;
    if (state->player_y > max_y) state->player_y = max_y;

    // Add smoothed trail point behind the player ship
    const float trail_offset = 18.0f;
    const float vertical_delta = state->player_y - state->player_prev_y;
    float desired_offset = vertical_delta * 8.0f;
    if (desired_offset > SPACE_PLAYER_TRAIL_MAX_OFFSET) {
        desired_offset = SPACE_PLAYER_TRAIL_MAX_OFFSET;
    } else if (desired_offset < -SPACE_PLAYER_TRAIL_MAX_OFFSET) {
        desired_offset = -SPACE_PLAYER_TRAIL_MAX_OFFSET;
    }
    const float smoothing = SDL_min(1.0f, dt * 12.0f);
    state->player_trail_offset_y += (desired_offset - state->player_trail_offset_y) * smoothing;

    const float trail_x = state->player_x - trail_offset;
    const float trail_y = state->player_y - state->player_trail_offset_y;

    space_trail_push(&state->player_trail, trail_x, trail_y);
    space_trail_update(&state->player_trail, dt, SPACE_TRAIL_DECAY_RATE, state->scroll_speed);

    // Update previous position for next frame
    state->player_prev_x = state->player_x;
    state->player_prev_y = state->player_y;
    space_update_drones(state, dt);

    if (state->gun_cooldown > 0.0f) {
        state->gun_cooldown -= dt;
    }
    if (state->laser_cooldown > 0.0f) {
        state->laser_cooldown -= dt;
    }
    if (state->laser_hold_timer > 0.0f) {
        state->laser_hold_timer -= dt;
    }

    if (state->input.fire_gun && state->gun_cooldown <= 0.0f) {
        const float base_speed = SPACE_BULLET_SPEED + state->scroll_speed * 0.25f;
        space_spawn_bullet(state, state->player_x + 26.0f, state->player_y, base_speed, 0.0f);

        if (state->weapon_upgrades.split_level > 0) {
            const float offset = 14.0f;
            const float spread = 60.0f;
            space_spawn_bullet(state,
                               state->player_x + 24.0f,
                               state->player_y - offset,
                               base_speed,
                               -spread * (float)state->weapon_upgrades.split_level);
            space_spawn_bullet(state,
                               state->player_x + 24.0f,
                               state->player_y + offset,
                               base_speed,
                               spread * (float)state->weapon_upgrades.split_level);
        }

        for (int i = 0; i < state->weapon_upgrades.drone_count; ++i) {
            const SpaceDrone *ship = &state->drones[i];
            if (!ship->active) {
                continue;
            }
            space_spawn_bullet(state, ship->x + 16.0f, ship->y, base_speed * 0.9f, ship->z * 0.08f);
        }

        state->gun_cooldown = 0.12f;
        space_spawn_firing_particles(state, state->player_x + 18.0f, state->player_y, SDL_FALSE);
    }

    // Handle laser recharge timer
    if (state->laser_recharge_timer > 0.0f) {
        state->laser_recharge_timer -= dt;
    }

    // Handle laser hold timer
    if (state->laser_hold_timer > 0.0f) {
        state->laser_hold_timer -= dt;

        // When laser timer expires, start 10-second recharge
        if (state->laser_hold_timer <= 0.0f) {
            state->laser_recharge_timer = 10.0f;
        }
    }

    if (state->input.fire_laser && state->laser_recharge_timer <= 0.0f) {
        // Start 3-second timer when first pressed
        if (state->laser_hold_timer <= 0.0f) {
            state->laser_hold_timer = 3.0f;
        }
    } else if (!state->input.fire_laser) {
        // Reset timer when button released (only if not in recharge)
        if (state->laser_recharge_timer <= 0.0f) {
            state->laser_hold_timer = 0.0f;
        }
    }

    // Activate continuous laser beams while held and timer > 0
    if (state->input.fire_laser && state->laser_hold_timer > 0.0f && state->laser_recharge_timer <= 0.0f) {
        // Player laser beam
        state->player_laser.is_firing = SDL_TRUE;
        state->player_laser.origin_x = state->player_x + 18.0f;
        state->player_laser.origin_y = state->player_y;

        // Update pulsing intensity
        state->player_laser.intensity = 0.7f + 0.3f * sinf(state->time_accumulator * 8.0f);

        // Spawn firing particles
        if (state->laser_cooldown <= 0.0f) {
            space_spawn_firing_particles(state, state->player_x + 18.0f, state->player_y, SDL_TRUE);
            state->laser_cooldown = 0.1f;  // Particle spawn rate
        }

        // Drone laser beams
        for (int d = 0; d < state->weapon_upgrades.drone_count; ++d) {
            SpaceDrone *drone = &state->drones[d];
            if (!drone->active) continue;

            state->drone_lasers[d].is_firing = SDL_TRUE;
            state->drone_lasers[d].origin_x = drone->x + 8.0f;
            state->drone_lasers[d].origin_y = drone->y;
            state->drone_lasers[d].intensity = 0.6f + 0.4f * sinf(state->time_accumulator * 10.0f);
        }
    } else {
        // Deactivate all laser beams
        state->player_laser.is_firing = SDL_FALSE;
        for (int d = 0; d < SPACE_MAX_DRONES; ++d) {
            state->drone_lasers[d].is_firing = SDL_FALSE;
        }
    }

    if (state->weapon_upgrades.thumper_active) {
        state->weapon_upgrades.thumper_wave_timer -= dt;
        if (state->weapon_upgrades.thumper_wave_timer <= 0.0f) {
            state->weapon_upgrades.thumper_wave_timer += 0.5f;
            state->weapon_upgrades.thumper_pulse_timer = 0.0f;
            space_spawn_thumper_wave(state);
        } else if (state->weapon_upgrades.thumper_pulse_timer < 0.4f) {
            state->weapon_upgrades.thumper_pulse_timer += dt;
        }
    }

    state->spawn_timer -= dt;
    if (state->spawn_timer <= 0.0f) {
        space_spawn_enemy_formation(state);
        state->spawn_interval = SDL_max(state->min_spawn_interval, state->spawn_interval * 0.965f);
        state->spawn_timer += state->spawn_interval;
    }

    for (int i = 0; i < SPACE_MAX_BULLETS; ++i) {
        SpaceBullet *bullet = &state->bullets[i];
        if (!bullet->active) {
            continue;
        }
        bullet->x += bullet->vx * dt;
        bullet->y += bullet->vy * dt;
        if (bullet->x > SPACE_SCREEN_W + 30.0f) {
            bullet->active = SDL_FALSE;
        }
    }

    space_update_guided_bullets(state, dt);
    space_update_particles(state, dt);

    // Laser beams are now handled during firing logic - no projectile updates needed

    for (int i = 0; i < SPACE_MAX_ENEMIES; ++i) {
        SpaceEnemy *enemy = &state->enemies[i];
        SpaceTrail *trail = &state->enemy_trails[i];
        if (!enemy->active) {
            space_trail_update(trail, dt, SPACE_ENEMY_TRAIL_DECAY, state->scroll_speed);
            continue;
        }
        enemy->x -= (state->scroll_speed + enemy->vx) * dt;
        enemy->y += enemy->vy * dt;
        enemy->rotation += enemy->rotation_speed * dt;
        if (enemy->rotation > (float)(M_PI * 2.0)) {
            enemy->rotation -= (float)(M_PI * 2.0);
        } else if (enemy->rotation < 0.0f) {
            enemy->rotation += (float)(M_PI * 2.0);
        }

        enemy->trail_emit_timer -= dt;
        while (enemy->trail_emit_timer <= 0.0f) {
            enemy->trail_emit_timer += SPACE_ENEMY_TRAIL_EMIT_INTERVAL;
            const float vel_x = -(state->scroll_speed + enemy->vx);
            const float vel_y = enemy->vy;
            const float len = SDL_max(1.0f, SDL_sqrtf(vel_x * vel_x + vel_y * vel_y));
            const float back_x = -(vel_x / len);
            const float back_y = -(vel_y / len);
            const float offset = enemy->radius * 1.3f;
            const float jitter = sinf(state->time_accumulator * 5.0f + (float)i) * enemy->radius * 0.12f;
            space_trail_push(trail,
                             enemy->x + back_x * offset,
                             enemy->y + back_y * offset + jitter);
        }
        space_trail_update(trail, dt, SPACE_ENEMY_TRAIL_DECAY, state->scroll_speed + enemy->vx);

        enemy->fire_timer -= dt;
        if (enemy->fire_timer <= 0.0f && state->player_alive) {
            space_fire_enemy_shot(state, enemy);
            enemy->fire_timer = enemy->fire_interval;
        }

        if (enemy->y - enemy->radius < state->play_area_top) {
            enemy->y = state->play_area_top + enemy->radius;
            enemy->vy = fabsf(enemy->vy);
        } else if (enemy->y + enemy->radius > state->play_area_bottom) {
            enemy->y = state->play_area_bottom - enemy->radius;
            enemy->vy = -fabsf(enemy->vy);
        }

        if (enemy->x + enemy->radius < -20.0f) {
            enemy->active = SDL_FALSE;
            state->player_hits++;
            // Don't damage player when enemy goes off screen
            continue;
        }

        const float dx = enemy->x - state->player_x;
        const float dy = enemy->y - state->player_y;
        const float collision_radius = enemy->radius + state->player_radius;
        if (dx * dx + dy * dy <= collision_radius * collision_radius) {
            enemy->active = SDL_FALSE;
            space_player_take_damage(state, 10.0f);
            space_spawn_explosion(state, enemy->x, enemy->y, enemy->radius * 1.2f);
            continue;
        }

        space_handle_projectile_hits(state, enemy);
    }

    space_update_enemy_shots(state, dt);
    space_update_upgrades(state, dt);
    space_handle_upgrade_pickups(state);
    space_handle_bullet_missile_collisions(state);
    space_handle_anomaly_hits(state, dt);
    space_update_anomaly(state, dt);
    space_apply_anomaly_laser_damage(state, dt);

    if (state->anomaly.active) {
        if (state->anomaly_wall_timer > 0.0f) {
            state->anomaly_wall_timer -= dt;
        }

        if (state->anomaly_wall_timer <= 0.0f) {
            const SDL_bool top_half = (state->anomaly_wall_phase % 2) == 0 ? SDL_TRUE : SDL_FALSE;
            space_spawn_enemy_wall(state, top_half);
            state->anomaly_wall_phase++;
            state->anomaly_wall_timer = 5.5f;
        }
        state->anomaly_warning_timer = 0.0f;
        state->anomaly_warning_phase = 0.0f;
    } else {
        state->anomaly_wall_timer = 0.0f;
        state->anomaly_wall_phase = 0;
    }

    // Update anomaly cooldown / warning timers
    if (state->anomaly_cooldown_timer > 0.0f) {
        state->anomaly_cooldown_timer = SDL_max(0.0f, state->anomaly_cooldown_timer - dt);
    }

    if (!state->anomaly.active) {
        if (state->anomaly_pending) {
            state->anomaly_warning_phase += dt;
            state->anomaly_warning_timer = state->anomaly_cooldown_timer;
            if (state->anomaly_cooldown_timer <= 0.0f) {
                space_activate_anomaly(state);
            }
        } else if (state->anomaly_cooldown_timer <= 0.0f &&
                   state->enemies_destroyed >= SPACE_ANOMALY_TRIGGER_KILLS &&
                   state->score >= SPACE_ANOMALY_TRIGGER_SCORE) {
            state->anomaly_pending = SDL_TRUE;
            state->anomaly_cooldown_timer = 10.0f;
            state->anomaly_warning_timer = 10.0f;
            state->anomaly_warning_phase = 0.0f;
        } else {
            state->anomaly_warning_timer = 0.0f;
            state->anomaly_warning_phase = 0.0f;
        }
    }

    for (int i = 0; i < SPACE_MAX_EXPLOSIONS; ++i) {
        SpaceExplosion *exp = &state->explosions[i];
        if (!exp->active) {
            continue;
        }
        exp->timer += dt;
        if (exp->timer >= exp->lifetime) {
            exp->active = SDL_FALSE;
        }
    }

    space_reset_input_triggers(state);
}
