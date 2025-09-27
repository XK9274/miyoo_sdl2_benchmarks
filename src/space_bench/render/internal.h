#ifndef SPACE_BENCH_RENDER_INTERNAL_H
#define SPACE_BENCH_RENDER_INTERNAL_H

#include "space_bench/render.h"
#include "space_bench/state.h"

#include "common/metrics.h"
#include "common/overlay.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define SPACE_BACKGROUND_R 8
#define SPACE_BACKGROUND_G 12
#define SPACE_BACKGROUND_B 24

typedef struct {
    float x;
    float y;
    float z;
} SpaceVec3;

SpaceVec3 space_rotate_roll(SpaceVec3 v, float roll);
SDL_FPoint space_project_point(SpaceVec3 v, float origin_x, float origin_y);
void space_render_trail(const SpaceTrail *trail,
                        SDL_Renderer *renderer,
                        BenchMetrics *metrics,
                        SDL_Color primary,
                        SDL_Color secondary);

void space_render_background(SDL_Renderer *renderer, BenchMetrics *metrics);
void space_render_stars(const SpaceBenchState *state, SDL_Renderer *renderer, BenchMetrics *metrics);
void space_render_enemy_shots(const SpaceBenchState *state, SDL_Renderer *renderer, BenchMetrics *metrics);
void space_render_upgrades(const SpaceBenchState *state, SDL_Renderer *renderer, BenchMetrics *metrics);
void space_render_laser_helix(const SpaceBenchState *state,
                              SDL_Renderer *renderer,
                              BenchMetrics *metrics,
                              float x1,
                              float y1,
                              float x2,
                              float y2,
                              float amplitude,
                              float frequency,
                              SDL_Color primary,
                              SDL_Color secondary);
void space_render_lasers(const SpaceBenchState *state, SDL_Renderer *renderer, BenchMetrics *metrics);
void space_render_bullets(const SpaceBenchState *state, SDL_Renderer *renderer, BenchMetrics *metrics);
void space_render_particles(const SpaceBenchState *state, SDL_Renderer *renderer, BenchMetrics *metrics);
void space_render_enemies(const SpaceBenchState *state, SDL_Renderer *renderer, BenchMetrics *metrics);
void space_render_drones(const SpaceBenchState *state, SDL_Renderer *renderer, BenchMetrics *metrics);
void space_render_anomaly(const SpaceBenchState *state, SDL_Renderer *renderer, BenchMetrics *metrics);
void space_render_explosions(const SpaceBenchState *state, SDL_Renderer *renderer, BenchMetrics *metrics);
void space_render_player(const SpaceBenchState *state, SDL_Renderer *renderer, BenchMetrics *metrics);
void space_render_game_over(const SpaceBenchState *state, SDL_Renderer *renderer, BenchMetrics *metrics);

#endif  // SPACE_BENCH_RENDER_INTERNAL_H
