#ifndef SPACE_BENCH_STATE_H
#define SPACE_BENCH_STATE_H

#include <SDL2/SDL.h>

#include "bench_common.h"

#define SPACE_SCREEN_W BENCH_SCREEN_W
#define SPACE_SCREEN_H BENCH_SCREEN_H

#define SPACE_STAR_COUNT 96
#define SPACE_SPEEDLINE_COUNT 24
#define SPACE_MAX_BULLETS 128
#define SPACE_MAX_LASERS 16
#define SPACE_MAX_ENEMIES 40
#define SPACE_MAX_EXPLOSIONS 64
#define SPACE_MAX_ENEMY_SHOTS 128
#define SPACE_MAX_UPGRADES 10
#define SPACE_MAX_DRONES 2
#define SPACE_TRAIL_POINTS 128
#define SPACE_MAX_PARTICLES 64
#define SPACE_ANOMALY_MID_LASER_COUNT 10

#define SPACE_ANOMALY_LASER_DPS 32.0f
#define SPACE_ANOMALY_LASER_THICKNESS 4.0f

#define SPACE_BULLET_RADIUS 3.0f
#define SPACE_LASER_LENGTH 320.0f
#define SPACE_LASER_WIDTH 24.0f

typedef struct {
    float x;
    float y;
    float vx;
    float vy;
    SDL_bool active;
} SpaceBullet;

typedef struct {
    SDL_bool is_firing;
    float origin_x;
    float origin_y;
    float intensity;  // For pulsing effect
} SpaceLaserBeam;

typedef struct {
    float x;
    float y;
    float vx;
    float vy;
    float radius;
    float hp;
    float rotation;
    float rotation_speed;
    float fire_timer;
    float fire_interval;
    float trail_emit_timer;
    SDL_bool active;
} SpaceEnemy;

typedef enum {
    SPACE_UPGRADE_SPLIT_CANNON = 0,
    SPACE_UPGRADE_GUIDANCE,
    SPACE_UPGRADE_DRONES,
    SPACE_UPGRADE_BEAM_FOCUS,
    SPACE_UPGRADE_SHIELD,
    SPACE_UPGRADE_THUMPER,
    SPACE_UPGRADE_COUNT
} SpaceUpgradeType;

typedef struct {
    float x;
    float y;
    float vx;
    float vy;
    float life;
    SDL_bool active;
    SDL_bool is_missile;  // Missile vs regular shot
    float trail_points[16][2];  // Longer trail for missiles
    int trail_count;
    float trail_timer;  // Timer to add trail points
    float damage;
} SpaceEnemyShot;

typedef struct {
    float x;
    float y;
    float vx;
    float vy;
    float phase;
    SpaceUpgradeType type;
    SDL_bool active;
} SpaceUpgrade;

typedef struct {
    float x;
    float y;
    float alpha;
} SpaceTrailPoint;

typedef struct {
    SpaceTrailPoint points[SPACE_TRAIL_POINTS];
    int count;
} SpaceTrail;

typedef struct {
    float angle;
    float radius;
    float angular_velocity;
    float x;
    float y;
    float z;
    float prev_y;
    float trail_offset_y;
    SDL_bool active;
} SpaceDrone;

typedef struct {
    int split_level;
    SDL_bool guidance_active;
    int drone_count;
    float beam_scale;
    SDL_bool thumper_active;
    float thumper_pulse_timer;
    float thumper_wave_timer;
} SpaceWeaponUpgrades;

typedef struct {
    float angle;
    float radius;
    float speed;
    float fire_timer;
    float laser_timer;
    SDL_bool active;
} AnomalyWeaponPoint;

typedef struct {
    float angle;
    float radius;
    float angular_velocity;
    float phase_offset;
    SDL_bool active;
} AnomalyOrbitalPoint;

typedef struct {
    float x;
    float y;
    float target_x;
    float target_y;
    float length;
    float duration;
    float curve_factor;
    float fade;
    SDL_bool active;
} AnomalyLaser;

typedef struct {
    SDL_bool active;
    float x;
    float y;
    float vx;
    float target_y;
    float rotation;
    float rotation_speed;
    float scale;
    float pulse;
    int shape_index;
    int render_mode;
    float gun_cooldown;

    // Multi-layer system with individual HP
    int current_layer;  // 0=outer, 1=mid2, 2=mid1, 3=inner, 4=core
    float layer_health[5];
    float layer_max_health[5];

    // Expanded weapon layers
    AnomalyWeaponPoint outer_points[10];   // Outer layer - radial guns every 36 degrees
    AnomalyWeaponPoint mid2_points[8];     // Mid layer 2 - rapid fire
    AnomalyWeaponPoint mid1_points[6];     // Mid layer 1 - heavy shots
    AnomalyWeaponPoint inner_points[4];    // Inner layer - lasers
    AnomalyWeaponPoint core_points[20];    // Core - radial guns every 18 degrees

    float outer_rotation;
    float mid2_rotation;
    float mid1_rotation;
    float inner_rotation;
    float core_rotation;

    AnomalyLaser lasers[SPACE_ANOMALY_MID_LASER_COUNT];  // Rotating lasers for mid layers
    float mid_laser_angle;
    float mid_laser_cycle_timer;
    SDL_bool mid_laser_enabled;

    // Shield system
    float shield_strength;
    float shield_max_strength;
    SDL_bool shield_active;
    float shield_pulse;

    // Orbital visual effects - spiraling 1x1 points
    AnomalyOrbitalPoint orbital_points[64];
} SpaceAnomaly;

typedef struct {
    float x;
    float y;
    float vx;
    float vy;
    float life;
    float max_life;
    Uint8 r;
    Uint8 g;
    Uint8 b;
    SDL_bool active;
} SpaceParticle;

typedef struct {
    float x;
    float y;
    float timer;
    float lifetime;
    float radius;
    SDL_bool active;
} SpaceExplosion;

typedef struct {
    SDL_bool up;
    SDL_bool down;
    SDL_bool left;
    SDL_bool right;
    SDL_bool fire_gun;
    SDL_bool fire_laser;
} SpaceInputState;

typedef enum {
    SPACE_GAME_PLAYING,
    SPACE_GAME_OVER
} SpaceGameState;

typedef enum {
    SPACE_GAMEOVER_RETRY = 0,
    SPACE_GAMEOVER_QUIT = 1
} SpaceGameOverOption;

typedef struct {
    float player_x;
    float player_y;
    float player_prev_x;
    float player_prev_y;
    float player_trail_offset_y;
    float player_speed;
    float player_radius;
    float player_roll;
    float player_roll_speed;
    float player_health;
    float player_max_health;
    float player_invulnerable;
    SDL_bool player_alive;

    float shield_strength;
    float shield_max_strength;
    SDL_bool shield_active;
    float shield_pulse;

    float scroll_speed;
    float target_scroll_speed;

    float gun_cooldown;
    float laser_cooldown;
    float laser_hold_timer;
    float laser_recharge_timer;

    float spawn_timer;
    float spawn_interval;
    float min_spawn_interval;

    int score;
    int enemies_destroyed;  // For anomaly trigger logic
    int total_enemies_killed;  // For display purposes
    int player_hits;

    float play_area_top;
    float play_area_bottom;

    SpaceGameState game_state;
    SpaceGameOverOption gameover_selected;
    float gameover_countdown;

    SpaceInputState input;

    float star_x[SPACE_STAR_COUNT];
    float star_y[SPACE_STAR_COUNT];
    float star_speed[SPACE_STAR_COUNT];
    Uint8 star_brightness[SPACE_STAR_COUNT];
    float speedline_x[SPACE_SPEEDLINE_COUNT];
    float speedline_y[SPACE_SPEEDLINE_COUNT];
    float speedline_length[SPACE_SPEEDLINE_COUNT];
    float speedline_speed[SPACE_SPEEDLINE_COUNT];

    SpaceBullet bullets[SPACE_MAX_BULLETS];
    SpaceLaserBeam player_laser;
    SpaceLaserBeam drone_lasers[SPACE_MAX_DRONES];
    SpaceEnemy enemies[SPACE_MAX_ENEMIES];
    SpaceExplosion explosions[SPACE_MAX_EXPLOSIONS];
    SpaceEnemyShot enemy_shots[SPACE_MAX_ENEMY_SHOTS];
    SpaceParticle particles[SPACE_MAX_PARTICLES];
    SpaceUpgrade upgrades[SPACE_MAX_UPGRADES];
    SpaceDrone drones[SPACE_MAX_DRONES];
    SpaceTrail player_trail;
    SpaceTrail drone_trails[SPACE_MAX_DRONES];
    SpaceTrail enemy_trails[SPACE_MAX_ENEMIES];
    SpaceWeaponUpgrades weapon_upgrades;
    SDL_bool thumper_dropped;
    SpaceAnomaly anomaly;
    float anomaly_cooldown_timer;
    float anomaly_wall_timer;
    int anomaly_wall_phase;
    SDL_bool anomaly_pending;
    float anomaly_warning_timer;
    float anomaly_warning_phase;

    Uint32 rng_state;
    float time_accumulator;
} SpaceBenchState;

void space_state_init(SpaceBenchState *state);
void space_state_update_layout(SpaceBenchState *state, int overlay_height);
void space_state_update(SpaceBenchState *state, float dt);

#endif /* SPACE_BENCH_STATE_H */
