#ifndef DOUBLE_BUF_STATE_H
#define DOUBLE_BUF_STATE_H

#include <SDL2/SDL.h>

#include "bench_common.h"

#define DB_MAX_PARTICLES 500
#define DB_SCREEN_W BENCH_SCREEN_W
#define DB_SCREEN_H BENCH_SCREEN_H
#define DB_PARTICLE_PALETTE_SIZE 8

typedef struct {
    BenchParticle particles[DB_MAX_PARTICLES];
    Uint8 particle_color_index[DB_MAX_PARTICLES];
    int particle_count;
    float particle_speed;
    float cube_rotation;
    SDL_bool show_cube;
    SDL_bool show_particles;
    SDL_bool backdrop_grid;
    int render_mode;
    int shape_type;
    float top_margin;
    float center_y;

} DoubleBenchState;

void db_state_init(DoubleBenchState *state);
void db_state_update_layout(DoubleBenchState *state, int overlay_height);
void db_state_respawn_particle(DoubleBenchState *state, BenchParticle *particle);
const SDL_Color *db_state_particle_palette(void);

#endif /* DOUBLE_BUF_STATE_H */
