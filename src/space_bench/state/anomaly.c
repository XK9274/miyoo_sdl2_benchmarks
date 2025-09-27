#include "space_bench/state/internal.h"
#include "space_bench/state/constants.h"
#include "common/geometry/shapes.h"

#include <math.h>

void space_activate_anomaly(SpaceBenchState *state)
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
    const float center_y = (state->play_area_top + state->play_area_bottom) * 0.5f;
    state->anomaly.target_y = center_y;
    state->anomaly.y = space_rand_range(state,
                                        center_y - state->anomaly.scale * 1.2f,
                                        center_y + state->anomaly.scale * 1.2f);
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

void space_fire_anomaly_laser_shot(SpaceBenchState *state, float x, float y, float dx, float dy)
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

void space_update_anomaly_lasers(SpaceBenchState *state, float dt)
{
    // Remove old tracking laser system - now uses rapid-fire projectiles
    (void)state;
    (void)dt;
}

void space_update_anomaly(SpaceBenchState *state, float dt)
{
    if (!state->anomaly.active) {
        return;
    }

    SpaceAnomaly *anomaly = &state->anomaly;
    anomaly->x += anomaly->vx * dt;
    const float target_y = anomaly->target_y;
    const float vertical_delta = target_y - anomaly->y;
    if (fabsf(vertical_delta) > 1.0f) {
        const float settle_speed = 1.5f;
        anomaly->y += vertical_delta * SDL_min(1.0f, dt * settle_speed);
    }
    anomaly->rotation += anomaly->rotation_speed * dt;
    anomaly->pulse += dt;
    anomaly->gun_cooldown -= dt;

    // Update layer rotations
    anomaly->core_rotation += 5.0f * dt;
    anomaly->inner_rotation -= 1.0f * dt;
    anomaly->mid1_rotation += 1.0f * dt;
    anomaly->mid2_rotation -= 1.8f * dt;
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
        const float rotation_speed = (anomaly->current_layer >= 4) ? 0.55f : 0.4f;
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

void space_apply_anomaly_laser_damage(SpaceBenchState *state, float dt)
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

void space_handle_anomaly_hits(SpaceBenchState *state, float dt)
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
