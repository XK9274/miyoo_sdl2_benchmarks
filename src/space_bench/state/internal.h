#ifndef SPACE_BENCH_STATE_INTERNAL_H
#define SPACE_BENCH_STATE_INTERNAL_H

#include "space_bench/state.h"

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Uint32 space_rand_u32(SpaceBenchState *state);
float space_rand_float(SpaceBenchState *state);
float space_rand_range(SpaceBenchState *state, float min_val, float max_val);
float space_distance_sq_to_segment(float px,
                                   float py,
                                   float x1,
                                   float y1,
                                   float x2,
                                   float y2);

void space_trail_reset(SpaceTrail *trail);
void space_trail_push(SpaceTrail *trail, float x, float y);
void space_trail_update(SpaceTrail *trail, float dt, float decay_rate, float scroll_speed);

SpaceUpgradeType space_random_upgrade(SpaceBenchState *state);
void space_spawn_upgrade(SpaceBenchState *state, SpaceUpgradeType type, float x, float y, float vx);
void space_try_drop_upgrade(SpaceBenchState *state, float x, float y, float vx);
void space_apply_upgrade(SpaceBenchState *state, SpaceUpgradeType type);
void space_handle_upgrade_pickups(SpaceBenchState *state);
void space_update_upgrades(SpaceBenchState *state, float dt);

void space_player_take_damage(SpaceBenchState *state, float amount);
void space_player_take_beam_damage(SpaceBenchState *state, float amount);
void space_update_drones(SpaceBenchState *state, float dt);
void space_spawn_bullet(SpaceBenchState *state, float x, float y, float vx, float vy);
void space_update_guided_bullets(SpaceBenchState *state, float dt);
void space_spawn_thumper_wave(SpaceBenchState *state);

void space_fire_enemy_shot(SpaceBenchState *state, const SpaceEnemy *enemy);
void space_fire_anomaly_missile(SpaceBenchState *state, float x, float y, float dx, float dy);
void space_update_enemy_shots(SpaceBenchState *state, float dt);
void space_handle_projectile_hits(SpaceBenchState *state, SpaceEnemy *enemy);
void space_handle_bullet_missile_collisions(SpaceBenchState *state);

void space_spawn_explosion(SpaceBenchState *state, float x, float y, float radius);
void space_spawn_layer_explosion(SpaceBenchState *state, float x, float y, float radius, int particles);
void space_spawn_firing_particles(SpaceBenchState *state, float x, float y, SDL_bool is_laser);
void space_update_particles(SpaceBenchState *state, float dt);

void space_activate_anomaly(SpaceBenchState *state);
void space_update_anomaly(SpaceBenchState *state, float dt);
void space_update_anomaly_lasers(SpaceBenchState *state, float dt);
void space_fire_anomaly_laser_shot(SpaceBenchState *state, float x, float y, float dx, float dy);
void space_apply_anomaly_laser_damage(SpaceBenchState *state, float dt);
void space_handle_anomaly_hits(SpaceBenchState *state, float dt);

void space_spawn_star(SpaceBenchState *state, int index, float min_x);
void space_update_stars(SpaceBenchState *state, float dt);
void space_spawn_enemy_formation(SpaceBenchState *state);
void space_spawn_enemy_wall(SpaceBenchState *state, SDL_bool top_half);

void space_reset_input_triggers(SpaceBenchState *state);

#endif  // SPACE_BENCH_STATE_INTERNAL_H
