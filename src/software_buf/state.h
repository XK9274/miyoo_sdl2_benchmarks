#ifndef SOFTWARE_BUF_STATE_H
#define SOFTWARE_BUF_STATE_H

#include <SDL2/SDL.h>

#include "bench_common.h"

#define SB_MAX_PARTICLES 500
#define SB_SCREEN_W BENCH_SCREEN_W
#define SB_SCREEN_H BENCH_SCREEN_H

typedef struct {
    BenchParticle particles[SB_MAX_PARTICLES];
    int particle_count;
    float particle_speed;
    float cube_rotation;
    SDL_bool show_cube;
    SDL_bool show_particles;
    SDL_bool stress_grid;
    int render_mode;
    int shape_type;
    float top_margin;
    float center_y;
} SoftwareBenchState;

void sb_state_init(SoftwareBenchState *state);
void sb_state_update_layout(SoftwareBenchState *state, int overlay_height);
void sb_state_respawn_particle(SoftwareBenchState *state, BenchParticle *particle);

#endif /* SOFTWARE_BUF_STATE_H */
