#include "space_bench/state/internal.h"
#include "space_bench/state/constants.h"

#include <math.h>

void space_spawn_star(SpaceBenchState *state, int index, float min_x)
{
    state->star_x[index] = min_x + space_rand_range(state, 0.0f, 80.0f);
    state->star_y[index] = space_rand_range(state, state->play_area_top + 4.0f, state->play_area_bottom - 4.0f);
    state->star_speed[index] = space_rand_range(state, 0.35f, 1.25f);
    state->star_brightness[index] = (Uint8)(220 - (Uint8)(state->star_speed[index] * 80.0f));
}

void space_spawn_enemy_formation(SpaceBenchState *state)
{
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

            if (formation_size == 1) {
                enemy->x = base_x;
                enemy->y = base_y;
            } else if (formation_size <= 3) {
                enemy->x = base_x + (spawned * space_rand_range(state, 25.0f, 45.0f));
                enemy->y = base_y + (spawned * space_rand_range(state, -15.0f, 15.0f));
            } else {
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

void space_spawn_enemy_wall(SpaceBenchState *state, SDL_bool top_half)
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
