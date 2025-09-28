#include "space_bench/state/internal.h"
#include "space_bench/state/constants.h"

void space_spawn_upgrade(SpaceBenchState *state,
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

void space_try_drop_upgrade(SpaceBenchState *state, float x, float y, float vx)
{
    if (space_rand_float(state) <= SPACE_UPGRADE_DROP_CHANCE) {
        const SpaceUpgradeType type = space_random_upgrade(state);
        space_spawn_upgrade(state, type, x, y, vx);
    }
}

SpaceUpgradeType space_random_upgrade(SpaceBenchState *state)
{
    const SDL_bool can_drop_thumper = (!state->weapon_upgrades.thumper_active &&
                                       !state->thumper_dropped &&
                                       (state->anomaly_pending || state->anomaly.active));

    const SDL_bool can_drop_minigun = (state->weapon_upgrades.split_level >= 2 &&
                                       !state->weapon_upgrades.minigun_active);

    if (can_drop_thumper && space_rand_float(state) < 0.18f) {
        return SPACE_UPGRADE_THUMPER;
    }

    if (can_drop_minigun && space_rand_float(state) < 0.6f) {
        return SPACE_UPGRADE_MINIGUN;
    }

    for (int attempt = 0; attempt < 4; ++attempt) {
        const int choice = (int)(space_rand_float(state) * SPACE_UPGRADE_COUNT) % SPACE_UPGRADE_COUNT;
        if (choice == SPACE_UPGRADE_THUMPER && !can_drop_thumper) {
            continue;
        }
        if (choice == SPACE_UPGRADE_THUMPER && state->weapon_upgrades.thumper_active) {
            continue;
        }
        if (choice == SPACE_UPGRADE_MINIGUN && !can_drop_minigun) {
            continue;
        }
        return (SpaceUpgradeType)choice;
    }

    return SPACE_UPGRADE_SPLIT_CANNON;
}

void space_apply_upgrade(SpaceBenchState *state, SpaceUpgradeType type)
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
        case SPACE_UPGRADE_MINIGUN:
            state->weapon_upgrades.minigun_active = SDL_TRUE;
            break;
        case SPACE_UPGRADE_COUNT:
        default:
            break;
    }
}

void space_handle_upgrade_pickups(SpaceBenchState *state)
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

void space_update_upgrades(SpaceBenchState *state, float dt)
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
