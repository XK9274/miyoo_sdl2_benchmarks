#include "space_bench/state.h"

#include <math.h>

Uint32 space_rand_u32(SpaceBenchState *state)
{
    Uint32 x = state->rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    state->rng_state = (x == 0) ? 0x1234567u : x;
    return state->rng_state;
}

float space_rand_float(SpaceBenchState *state)
{
    return (float)(space_rand_u32(state) & 0xFFFFFFu) / (float)0x1000000u;
}

float space_rand_range(SpaceBenchState *state, float min_val, float max_val)
{
    return min_val + (max_val - min_val) * space_rand_float(state);
}

float space_distance_sq_to_segment(float px,
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

void space_trail_reset(SpaceTrail *trail)
{
    if (!trail) {
        return;
    }
    trail->count = 0;
}

void space_trail_push(SpaceTrail *trail, float x, float y)
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

void space_trail_update(SpaceTrail *trail, float dt, float decay_rate, float scroll_speed)
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
        p->x -= backward_movement;
        if (p->alpha <= 0.0f || p->x < -50.0f) {
            continue;
        }
        trail->points[write_idx++] = *p;
    }
    trail->count = write_idx;
}
