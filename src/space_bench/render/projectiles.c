#include "space_bench/render/internal.h"

#include <math.h>

void space_render_enemy_shots(const SpaceBenchState *state,
                              SDL_Renderer *renderer,
                              BenchMetrics *metrics)
{
    SDL_Color inner = {255, 120, 60, 220};
    SDL_Color outer = {255, 220, 160, 160};
    SDL_Color missile_inner = {255, 60, 120, 240};
    SDL_Color missile_outer = {255, 160, 60, 180};

    SDL_FRect shot_outer_rects[SPACE_MAX_ENEMY_SHOTS];
    SDL_FRect shot_inner_rects[SPACE_MAX_ENEMY_SHOTS];
    SDL_FRect missile_outer_rects[SPACE_MAX_ENEMY_SHOTS];
    SDL_FRect missile_inner_rects[SPACE_MAX_ENEMY_SHOTS];
    int shot_outer_count = 0;
    int shot_inner_count = 0;
    int missile_outer_count = 0;
    int missile_inner_count = 0;

    for (int i = 0; i < SPACE_MAX_ENEMY_SHOTS; ++i) {
        const SpaceEnemyShot *shot = &state->enemy_shots[i];
        if (!shot->active) {
            continue;
        }

        if (shot->is_missile) {
            missile_outer_rects[missile_outer_count++] = (SDL_FRect){shot->x - 4.0f, shot->y - 1.5f, 8.0f, 3.0f};
            missile_inner_rects[missile_inner_count++] = (SDL_FRect){shot->x - 2.0f, shot->y - 0.75f, 4.0f, 1.5f};
        } else {
            shot_outer_rects[shot_outer_count++] = (SDL_FRect){shot->x - 4.0f, shot->y - 1.5f, 8.0f, 3.0f};
            shot_inner_rects[shot_inner_count++] = (SDL_FRect){shot->x - 2.0f, shot->y - 0.75f, 4.0f, 1.5f};
        }
    }

    if (shot_outer_count > 0) {
        SDL_SetRenderDrawColor(renderer, outer.r, outer.g, outer.b, outer.a);
        SDL_RenderFillRectsF(renderer, shot_outer_rects, shot_outer_count);
        if (metrics) {
            metrics->draw_calls++;
            metrics->vertices_rendered += shot_outer_count * 4;
        }
    }
    if (shot_inner_count > 0) {
        SDL_SetRenderDrawColor(renderer, inner.r, inner.g, inner.b, inner.a);
        SDL_RenderFillRectsF(renderer, shot_inner_rects, shot_inner_count);
        if (metrics) {
            metrics->draw_calls++;
            metrics->vertices_rendered += shot_inner_count * 4;
        }
    }

    if (missile_outer_count > 0) {
        SDL_SetRenderDrawColor(renderer, missile_outer.r, missile_outer.g, missile_outer.b, missile_outer.a);
        SDL_RenderFillRectsF(renderer, missile_outer_rects, missile_outer_count);
        if (metrics) {
            metrics->draw_calls++;
            metrics->vertices_rendered += missile_outer_count * 4;
        }
    }
    if (missile_inner_count > 0) {
        SDL_SetRenderDrawColor(renderer, missile_inner.r, missile_inner.g, missile_inner.b, missile_inner.a);
        SDL_RenderFillRectsF(renderer, missile_inner_rects, missile_inner_count);
        if (metrics) {
            metrics->draw_calls++;
            metrics->vertices_rendered += missile_inner_count * 4;
        }
    }
}

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
                              SDL_Color secondary)
{
    const int segments = 12;
    const float dx = x2 - x1;
    const float dy = y2 - y1;
    const float length = SDL_sqrtf(dx * dx + dy * dy);
    if (length <= 0.01f) {
        return;
    }
    const float inv_length = 1.0f / length;
    const float dir_x = dx * inv_length;
    const float dir_y = dy * inv_length;
    const float perp_x = -dir_y;
    const float perp_y = dir_x;

    for (int i = 0; i < segments; ++i) {
        const float t1 = (float)i / (float)segments;
        const float t2 = (float)(i + 1) / (float)segments;
        const float wave1 = sinf((state->time_accumulator + t1 * length * frequency) * 5.0f) * amplitude;
        const float wave2 = sinf((state->time_accumulator + t2 * length * frequency) * 5.0f) * amplitude;

        const float ax = x1 + dir_x * length * t1 + perp_x * wave1;
        const float ay = y1 + dir_y * length * t1 + perp_y * wave1;
        const float bx = x1 + dir_x * length * t2 + perp_x * wave2;
        const float by = y1 + dir_y * length * t2 + perp_y * wave2;

        SDL_Color color = (i % 2 == 0) ? primary : secondary;
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_RenderDrawLineF(renderer, ax, ay, bx, by);
        if (metrics) {
            metrics->draw_calls++;
            metrics->vertices_rendered += 2;
        }
    }
}

void space_render_lasers(const SpaceBenchState *state,
                         SDL_Renderer *renderer,
                         BenchMetrics *metrics)
{
    if (!state->player_laser.is_firing) {
        return;
    }

    SDL_Color core_color = {255, 255, 200, 255};
    SDL_Color glow_color = {120, 220, 255, 180};

    SDL_SetRenderDrawColor(renderer, glow_color.r, glow_color.g, glow_color.b, glow_color.a);
    SDL_RenderDrawLineF(renderer,
                        state->player_laser.origin_x,
                        state->player_laser.origin_y - 2.5f,
                        (float)SPACE_SCREEN_W,
                        state->player_laser.origin_y - 2.5f);
    SDL_RenderDrawLineF(renderer,
                        state->player_laser.origin_x,
                        state->player_laser.origin_y + 2.5f,
                        (float)SPACE_SCREEN_W,
                        state->player_laser.origin_y + 2.5f);
    SDL_SetRenderDrawColor(renderer, core_color.r, core_color.g, core_color.b, core_color.a);
    SDL_RenderDrawLineF(renderer,
                        state->player_laser.origin_x,
                        state->player_laser.origin_y,
                        (float)SPACE_SCREEN_W,
                        state->player_laser.origin_y);

    if (metrics) {
        metrics->draw_calls += 3;
        metrics->vertices_rendered += 6;
    }

    const SDL_Color player_helix_a = {255, 210, 160, 160};
    const SDL_Color player_helix_b = {180, 140, 255, 150};
    space_render_laser_helix(state,
                             renderer,
                             metrics,
                             state->player_laser.origin_x,
                             state->player_laser.origin_y,
                             (float)SPACE_SCREEN_W,
                             state->player_laser.origin_y,
                             1.4f * state->weapon_upgrades.beam_scale,
                             0.8f,
                             player_helix_a,
                             player_helix_b);

    for (int i = 0; i < state->weapon_upgrades.drone_count; ++i) {
        const SpaceLaserBeam *laser = &state->drone_lasers[i];
        if (!laser->is_firing) {
            continue;
        }
        SDL_Color drone_core = {255, 180, 220, 220};
        SDL_Color drone_glow = {180, 140, 255, 160};

        SDL_SetRenderDrawColor(renderer, drone_glow.r, drone_glow.g, drone_glow.b, drone_glow.a);
        SDL_RenderDrawLineF(renderer,
                            laser->origin_x,
                            laser->origin_y - 1.8f,
                            (float)SPACE_SCREEN_W,
                            laser->origin_y - 1.8f);
        SDL_RenderDrawLineF(renderer,
                            laser->origin_x,
                            laser->origin_y + 1.8f,
                            (float)SPACE_SCREEN_W,
                            laser->origin_y + 1.8f);
        SDL_SetRenderDrawColor(renderer, drone_core.r, drone_core.g, drone_core.b, drone_core.a);
        SDL_RenderDrawLineF(renderer,
                            laser->origin_x,
                            laser->origin_y,
                            (float)SPACE_SCREEN_W,
                            laser->origin_y);

        if (metrics) {
            metrics->draw_calls += 3;
            metrics->vertices_rendered += 6;
        }

        const SDL_Color drone_helix_a = {200, 160, 255, 140};
        const SDL_Color drone_helix_b = {255, 200, 240, 140};
        space_render_laser_helix(state,
                                 renderer,
                                 metrics,
                                 laser->origin_x,
                                 laser->origin_y,
                                 (float)SPACE_SCREEN_W,
                                 laser->origin_y,
                                 1.0f,
                                 1.0f,
                                 drone_helix_a,
                                 drone_helix_b);
    }
}

void space_render_bullets(const SpaceBenchState *state,
                          SDL_Renderer *renderer,
                          BenchMetrics *metrics)
{
    SDL_FRect bullet_rects[SPACE_MAX_BULLETS];
    int bullet_count = 0;

    for (int i = 0; i < SPACE_MAX_BULLETS; ++i) {
        const SpaceBullet *bullet = &state->bullets[i];
        if (!bullet->active) {
            continue;
        }
        bullet_rects[bullet_count++] = (SDL_FRect){bullet->x - 2.5f, bullet->y - 1.2f, 5.0f, 2.4f};
    }

    if (bullet_count > 0) {
        SDL_SetRenderDrawColor(renderer, 255, 255, 160, 220);
        SDL_RenderFillRectsF(renderer, bullet_rects, bullet_count);
        if (metrics) {
            metrics->draw_calls++;
            metrics->vertices_rendered += bullet_count * 4;
        }
    }
}
