#include "space_bench/state.h"
#include "space_bench/state/constants.h"
#include "space_bench/state/internal.h"

#include <float.h>
#include <math.h>
#include <string.h>

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

void space_reset_input_triggers(SpaceBenchState *state)
{
    state->input.fire_gun = SDL_FALSE;
}

void space_update_stars(SpaceBenchState *state, float dt)
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
    int i = SPACE_STAR_COUNT;
    (void)i;
#endif

    for (int idx = 0; idx < SPACE_STAR_COUNT; ++idx) {
        state->star_x[idx] -= state->star_speed[idx] * base;
        if (state->star_x[idx] < -4.0f) {
            space_spawn_star(state, idx, (float)SPACE_SCREEN_W + 4.0f);
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
        state->star_speed[i] = space_rand_range(state, 18.0f, 32.0f);
        state->star_brightness[i] = (Uint8)space_rand_range(state, 80.0f, 160.0f);
    }

    for (int i = 0; i < SPACE_SPEEDLINE_COUNT; ++i) {
        state->speedline_x[i] = space_rand_range(state, 0.0f, (float)SPACE_SCREEN_W);
        state->speedline_y[i] = space_rand_range(state, state->play_area_top + 10.0f, state->play_area_bottom - 10.0f);
        state->speedline_length[i] = space_rand_range(state, 8.0f, 18.0f);
        state->speedline_speed[i] = space_rand_range(state, 90.0f, 140.0f);
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
    state->anomaly.target_y = (state->play_area_top + state->play_area_bottom) * 0.5f;
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
    for (int i = 0; i < SPACE_SPEEDLINE_COUNT; ++i) {
        state->speedline_x[i] -= (state->speedline_speed[i] + state->scroll_speed) * dt;
        if (state->speedline_x[i] + state->speedline_length[i] < 0.0f) {
            state->speedline_x[i] = (float)SPACE_SCREEN_W + space_rand_range(state, 20.0f, 60.0f);
            state->speedline_y[i] = space_rand_range(state, state->play_area_top + 10.0f, state->play_area_bottom - 10.0f);
            state->speedline_length[i] = space_rand_range(state, 8.0f, 18.0f);
            state->speedline_speed[i] = space_rand_range(state, 90.0f, 140.0f);
        }
    }
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
