#ifndef SPRITE_BENCH_STATE_H
#define SPRITE_BENCH_STATE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "bench_common.h"

#define SPRITE_SIZE 16
#define SPRITE_POOL_SIZE 16
#define SPRITE_COUNT_CAP 50000
#define SPRITE_STEP_MIN 10
#define SPRITE_STEP_MAX 5000
#define SPRITE_STEP_DEFAULT 10
#define SPRITE_INTERVAL_DEFAULT 5.0
#define SPRITE_INTERVAL_MIN 0.1
#define SPRITE_INTERVAL_MAX 10.0

typedef struct {
    SDL_Texture *texture;
    int pattern;
    float phase;
    float phase_speed;
    SDL_BlendMode blend;
} SpritePoolSlot;

typedef struct {
    float x;
    float y;
    float vx;
    float vy;
    int pool_index;
} SpriteInstance;

typedef struct {
    SpritePoolSlot pool[SPRITE_POOL_SIZE];

    SpriteInstance *instances;
    int instance_capacity;
    int initialized_count;

    int sprite_count;
    int step_size;
    double interval_seconds;
    int direction;
    double ramp_timer_seconds;
    SDL_bool static_mode;
} SpriteBenchState;

SDL_bool sprite_state_init(SpriteBenchState *state, SDL_Renderer *renderer);
void sprite_state_destroy(SpriteBenchState *state);

void sprite_state_update_pool(SpriteBenchState *state, float delta_seconds);
void sprite_state_update_ramp(SpriteBenchState *state, double delta_seconds);
void sprite_state_update_instances(SpriteBenchState *state, float delta_seconds);
void sprite_state_render(SpriteBenchState *state, SDL_Renderer *renderer, BenchMetrics *metrics);

#endif /* SPRITE_BENCH_STATE_H */
