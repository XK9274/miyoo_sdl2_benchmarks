#include "software_buf/state.h"

#include <stdint.h>
#include <stdlib.h>
#include <time.h>

// Fast xorshift32 random number generator
static uint32_t g_rng_state = 1;

static uint32_t fast_rand(void)
{
    g_rng_state ^= g_rng_state << 13;
    g_rng_state ^= g_rng_state >> 17;
    g_rng_state ^= g_rng_state << 5;
    return g_rng_state;
}

static float random_range(float min_val, float max_val)
{
    if (max_val <= min_val) {
        return min_val;
    }
    const float t = (float)fast_rand() / (float)UINT32_MAX;
    return min_val + t * (max_val - min_val);
}

void sb_state_respawn_particle(SoftwareBenchState *state, BenchParticle *p)
{
    const float min_y = state->top_margin;
    const float max_y = (float)SB_SCREEN_H - 1.0f;
    p->x = random_range(0.0f, (float)SB_SCREEN_W);
    p->y = random_range(min_y, max_y);
    p->dx = random_range(-1.0f, 1.0f);
    p->dy = random_range(-1.0f, 1.0f);
    p->r = (Uint8)(fast_rand() % 255);
    p->g = (Uint8)(fast_rand() % 255);
    p->b = (Uint8)(fast_rand() % 255);
    p->a = (Uint8)(fast_rand() % 128 + 127);
    p->life = 1.0f;
}

void sb_state_init(SoftwareBenchState *state)
{
    if (!state) {
        return;
    }

    SDL_memset(state, 0, sizeof(*state));
    state->particle_count = 350;
    state->particle_speed = 400.0f;
    state->cube_rotation = 0.0f;
    state->show_cube = SDL_TRUE;
    state->show_particles = SDL_TRUE;
    state->stress_grid = SDL_FALSE;
    state->render_mode = 1;
    state->shape_type = 0;

    sb_state_update_layout(state, 0);

    // Seed fast random number generator
    g_rng_state = (uint32_t)time(NULL);
    if (g_rng_state == 0) g_rng_state = 1; // Ensure non-zero seed

    for (int i = 0; i < SB_MAX_PARTICLES; ++i) {
        sb_state_respawn_particle(state, &state->particles[i]);
    }
}

void sb_state_update_layout(SoftwareBenchState *state, int overlay_height)
{
    if (!state) {
        return;
    }
    state->top_margin = (float)overlay_height;
    float available = SB_SCREEN_H - state->top_margin;
    if (available < 1.0f) {
        available = 1.0f;
    }
    state->center_y = state->top_margin + available * 0.5f;
}
