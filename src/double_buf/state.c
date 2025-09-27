#include "double_buf/state.h"

#include <stdlib.h>
#include <time.h>

// Pre-compute expensive division to avoid per-particle cost
static const float INV_RAND_MAX = 1.0f / (float)RAND_MAX;

static void respawn_particle(DoubleBenchState *state, BenchParticle *p)
{
    const float min_y = state->top_margin;
    const float max_y = (float)DB_SCREEN_H - 1.0f;
    p->x = (float)(rand() % DB_SCREEN_W);
    p->y = min_y + ((float)rand() * INV_RAND_MAX) * (max_y - min_y);
    p->dx = -0.9f + ((float)rand() * INV_RAND_MAX) * 1.8f;
    p->dy = -0.9f + ((float)rand() * INV_RAND_MAX) * 1.8f;
    p->r = (Uint8)(rand() % 255);
    p->g = (Uint8)(rand() % 255);
    p->b = (Uint8)(rand() % 255);
    p->a = (Uint8)(rand() % 128 + 127);
    p->life = 1.0f;
}

void db_state_init(DoubleBenchState *state)
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
    state->backdrop_grid = SDL_FALSE;
    state->render_mode = 1;
    state->shape_type = 0;
    db_state_update_layout(state, 0);

    srand((unsigned int)time(NULL));
    for (int i = 0; i < DB_MAX_PARTICLES; ++i) {
        respawn_particle(state, &state->particles[i]);
    }
}

void db_state_update_layout(DoubleBenchState *state, int overlay_height)
{
    if (!state) {
        return;
    }
    state->top_margin = (float)overlay_height;
    float available = DB_SCREEN_H - state->top_margin;
    if (available < 1.0f) {
        available = 1.0f;
    }
    state->center_y = state->top_margin + available * 0.5f;
}

void db_state_respawn_particle(DoubleBenchState *state, BenchParticle *p)
{
    respawn_particle(state, p);
}
