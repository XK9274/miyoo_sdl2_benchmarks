#include "space_bench/state/internal.h"
#include "space_bench/state/constants.h"

#include <float.h>
#include <math.h>

void space_update_enemy_shots(SpaceBenchState *state, float dt)
{
    for (int i = 0; i < SPACE_MAX_ENEMY_SHOTS; ++i) {
        SpaceEnemyShot *shot = &state->enemy_shots[i];
        if (!shot->active) {
            continue;
        }
        shot->x += shot->vx * dt;
        shot->y += shot->vy * dt;
        shot->life -= dt;

        if (shot->is_missile) {
            shot->trail_emit_timer += dt;
            if (shot->trail_emit_timer >= 0.04f) {
                shot->trail_emit_timer -= 0.04f;
                space_spawn_enemy_missile_trail(state,
                                                shot->x,
                                                shot->y,
                                                shot->vx,
                                                shot->vy);
            }
        }

        if (shot->life <= 0.0f || shot->x < -40.0f || shot->x > SPACE_SCREEN_W + 40.0f ||
            shot->y < state->play_area_top - 40.0f || shot->y > state->play_area_bottom + 40.0f) {
            shot->active = SDL_FALSE;
            shot->damage = 0.0f;
            shot->trail_emit_timer = 0.0f;
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
                shot->trail_emit_timer = 0.0f;
                continue;
            }
        }

        const float dx = shot->x - state->player_x;
        const float dy = shot->y - state->player_y;
        const float hit_radius = state->player_radius + 6.0f;
        const float dist_sq = dx * dx + dy * dy;

        if (state->weapon_upgrades.thumper_active && dist_sq <= hit_radius * hit_radius) {
            state->weapon_upgrades.thumper_pulse_timer = 0.0f;

            const SDL_bool deflect = (space_rand_u32(state) & 1u) == 0u;
            if (!deflect) {
                space_spawn_explosion(state, shot->x, shot->y, 7.5f);
                shot->active = SDL_FALSE;
                shot->damage = 0.0f;
                shot->trail_emit_timer = 0.0f;
                continue;
            }

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
            shot->life = SDL_min(shot->life + 0.45f, 3.0f);
            shot->damage = SDL_max(1.0f, shot->damage * 0.5f);
            shot->is_missile = SDL_FALSE;
            shot->trail_emit_timer = 0.0f;
            space_spawn_explosion(state, shot->x, shot->y, 9.0f);
            continue;
        }

        if (dist_sq <= hit_radius * hit_radius) {
            shot->active = SDL_FALSE;
            const float damage = (shot->damage > 0.0f) ? shot->damage : (shot->is_missile ? 12.0f : 5.0f);
            shot->damage = 0.0f;
            shot->trail_emit_timer = 0.0f;
            space_player_take_damage(state, damage);
        }
    }
}

void space_fire_enemy_shot(SpaceBenchState *state, const SpaceEnemy *enemy)
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
            shot->trail_emit_timer = 0.0f;
            shot->damage = 5.0f;
            return;
        }
    }
}

void space_fire_anomaly_missile(SpaceBenchState *state, float x, float y, float dx, float dy)
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
            shot->trail_emit_timer = 0.0f;
            shot->damage = 12.0f;
            return;
        }
    }
}

void space_handle_bullet_missile_collisions(SpaceBenchState *state)
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
            const float collision_radius = 8.0f;
            if (dx * dx + dy * dy <= collision_radius * collision_radius) {
                bullet->active = SDL_FALSE;
                missile->active = SDL_FALSE;
                state->score += 10;
                space_spawn_explosion(state, missile->x, missile->y, 15.0f);
                break;
            }
        }
    }
}

void space_handle_projectile_hits(SpaceBenchState *state, SpaceEnemy *enemy)
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

    if (state->player_laser.is_firing) {
        const float beam_top = state->player_laser.origin_y - 1.5f;
        const float beam_bottom = state->player_laser.origin_y + 1.5f;

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
            return;
        }
    }
}
