#include "space_bench/render.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <math.h>

#include "common/geometry/shapes.h"
#include "common/overlay.h"

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



static SpaceVec3 space_rotate_roll(SpaceVec3 v, float roll)
{
    const float c = cosf(roll);
    const float s = sinf(roll);
    const float old_y = v.y;
    const float old_z = v.z;
    v.y = old_y * c - old_z * s;
    v.z = old_y * s + old_z * c;
    return v;
}

static SDL_FPoint space_project_point(SpaceVec3 v, float origin_x, float origin_y)
{
    SDL_FPoint p = {origin_x + v.x, origin_y + v.y};
    return p;
}


static void space_render_trail(const SpaceTrail *trail,
                               SDL_Renderer *renderer,
                               BenchMetrics *metrics,
                               SDL_Color primary,
                               SDL_Color secondary __attribute__((unused)))
{
    if (!trail || trail->count < 2) {
        return;
    }

    for (int i = 1; i < trail->count; ++i) {
        const SpaceTrailPoint *prev = &trail->points[i - 1];
        const SpaceTrailPoint *curr = &trail->points[i];
        const float t = (float)(trail->count - i) / (float)trail->count;  // Inverted for correct gradient

        // Fade from bright near player to black far away
        SDL_Color blend = primary;
        blend.r = (Uint8)(primary.r * t);
        blend.g = (Uint8)(primary.g * t);
        blend.b = (Uint8)(primary.b * t);
        blend.a = (Uint8)(200 * t * curr->alpha);

        SDL_SetRenderDrawColor(renderer, blend.r, blend.g, blend.b, blend.a);
        SDL_RenderDrawLineF(renderer, prev->x, prev->y, curr->x, curr->y);
        if (metrics) {
            metrics->draw_calls++;
            metrics->vertices_rendered += 2;
        }
    }
}

static void space_render_enemy_shots(const SpaceBenchState *state,
                                     SDL_Renderer *renderer,
                                     BenchMetrics *metrics)
{
    SDL_Color inner = {255, 120, 60, 220};
    SDL_Color outer = {255, 220, 160, 160};
    SDL_Color missile_inner = {255, 60, 120, 240};
    SDL_Color missile_outer = {255, 160, 60, 180};

    for (int i = 0; i < SPACE_MAX_ENEMY_SHOTS; ++i) {
        const SpaceEnemyShot *shot = &state->enemy_shots[i];
        if (!shot->active) {
            continue;
        }

        if (shot->is_missile) {
            // Render missile trail as fading points
            if (shot->trail_count > 0) {
                for (int t = 0; t < shot->trail_count; ++t) {
                    const float age = (float)(shot->trail_count - t) / (float)shot->trail_count;
                    const Uint8 brightness = (Uint8)(255 * age * 0.8f);
                    const Uint8 opacity = (Uint8)(200 * age);
                    SDL_SetRenderDrawColor(renderer, brightness, brightness * 0.6f, brightness * 0.3f, opacity);
                    SDL_RenderDrawPointF(renderer, shot->trail_points[t][0], shot->trail_points[t][1]);
                }
            }

            // Thinner missile body
            SDL_FRect body = {shot->x - 4.0f, shot->y - 1.5f, 8.0f, 3.0f};
            SDL_SetRenderDrawColor(renderer, missile_outer.r, missile_outer.g, missile_outer.b, missile_outer.a);
            SDL_RenderFillRectF(renderer, &body);
            SDL_FRect core = {shot->x - 2.0f, shot->y - 0.75f, 4.0f, 1.5f};
            SDL_SetRenderDrawColor(renderer, missile_inner.r, missile_inner.g, missile_inner.b, missile_inner.a);
            SDL_RenderFillRectF(renderer, &core);

            if (metrics) {
                metrics->draw_calls += 2 + shot->trail_count;
                metrics->vertices_rendered += 8 + shot->trail_count;
            }
        } else {
            // Regular shot
            SDL_FRect body = {shot->x - 4.0f, shot->y - 1.5f, 8.0f, 3.0f};
            SDL_SetRenderDrawColor(renderer, outer.r, outer.g, outer.b, outer.a);
            SDL_RenderFillRectF(renderer, &body);
            SDL_FRect core = {shot->x - 2.0f, shot->y - 0.75f, 4.0f, 1.5f};
            SDL_SetRenderDrawColor(renderer, inner.r, inner.g, inner.b, inner.a);
            SDL_RenderFillRectF(renderer, &core);

            if (metrics) {
                metrics->draw_calls += 2;
                metrics->vertices_rendered += 8;
            }
        }
    }
}

static SDL_Color space_upgrade_color(SpaceUpgradeType type)
{
    switch (type) {
        case SPACE_UPGRADE_SPLIT_CANNON:
            return (SDL_Color){255, 200, 100, 230};
        case SPACE_UPGRADE_GUIDANCE:
            return (SDL_Color){120, 240, 255, 230};
        case SPACE_UPGRADE_DRONES:
            return (SDL_Color){180, 140, 255, 230};
        case SPACE_UPGRADE_BEAM_FOCUS:
            return (SDL_Color){255, 120, 200, 230};
        case SPACE_UPGRADE_SHIELD:
            return (SDL_Color){100, 255, 150, 230};
        case SPACE_UPGRADE_THUMPER:
            return (SDL_Color){180, 240, 255, 255};
        case SPACE_UPGRADE_COUNT:
        default:
            return (SDL_Color){255, 255, 255, 230};
    }
}

static void space_render_upgrades(const SpaceBenchState *state,
                                  SDL_Renderer *renderer,
                                  BenchMetrics *metrics)
{
    for (int i = 0; i < SPACE_MAX_UPGRADES; ++i) {
        const SpaceUpgrade *upgrade = &state->upgrades[i];
        if (!upgrade->active) {
            continue;
        }

        if (upgrade->type == SPACE_UPGRADE_THUMPER) {
            const float pulse = (sinf(state->time_accumulator * 5.0f + upgrade->phase) * 0.5f + 0.5f);
            const float radius_inner = 6.0f + pulse * 2.5f;
            const float radius_outer = radius_inner + 3.0f;
            const int segments = 18;

            SDL_SetRenderDrawColor(renderer, 180, 240, 255, 220);
            for (int s = 0; s < segments; ++s) {
                const float angle1 = (float)(M_PI * 2.0) * (float)s / (float)segments;
                const float angle2 = (float)(M_PI * 2.0) * (float)(s + 1) / (float)segments;
                const float x1 = upgrade->x + cosf(angle1) * radius_outer;
                const float y1 = upgrade->y + sinf(angle1) * radius_outer;
                const float x2 = upgrade->x + cosf(angle2) * radius_outer;
                const float y2 = upgrade->y + sinf(angle2) * radius_outer;
                SDL_RenderDrawLineF(renderer, x1, y1, x2, y2);
            }

            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_FRect core = {upgrade->x - radius_inner * 0.35f,
                              upgrade->y - radius_inner * 0.35f,
                              radius_inner * 0.7f,
                              radius_inner * 0.7f};
            SDL_RenderFillRectF(renderer, &core);

            SDL_SetRenderDrawColor(renderer, 120, 200, 255, 200);
            SDL_RenderDrawLineF(renderer, upgrade->x - radius_inner,
                                upgrade->y,
                                upgrade->x + radius_inner,
                                upgrade->y);
            SDL_RenderDrawLineF(renderer, upgrade->x,
                                upgrade->y - radius_inner,
                                upgrade->x,
                                upgrade->y + radius_inner);

            if (metrics) {
                metrics->draw_calls += segments + 2;
                metrics->vertices_rendered += (segments + 2) * 2;
            }
            continue;
        }

        SDL_Color color = space_upgrade_color(upgrade->type);
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        const float size = 6.0f;
        SDL_FPoint points[5] = {
            {upgrade->x, upgrade->y - size},
            {upgrade->x + size, upgrade->y},
            {upgrade->x, upgrade->y + size},
            {upgrade->x - size, upgrade->y},
            {upgrade->x, upgrade->y - size}
        };
        SDL_RenderDrawLinesF(renderer, points, 5);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, color.a);
        SDL_FRect core = {upgrade->x - 3.0f, upgrade->y - 3.0f, 6.0f, 6.0f};
        SDL_RenderFillRectF(renderer, &core);

        if (metrics) {
            metrics->draw_calls += 2;
            metrics->vertices_rendered += 9;
        }
    }
}

static void space_render_background(SDL_Renderer *renderer, BenchMetrics *metrics)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(renderer, SPACE_BACKGROUND_R, SPACE_BACKGROUND_G, SPACE_BACKGROUND_B, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    if (metrics) {
        metrics->draw_calls++;
    }
}

static void space_render_stars(const SpaceBenchState *state,
                               SDL_Renderer *renderer,
                               BenchMetrics *metrics)
{
    for (int i = 0; i < SPACE_STAR_COUNT; ++i) {
        SDL_SetRenderDrawColor(renderer,
                               state->star_brightness[i],
                               state->star_brightness[i],
                               255,
                               255);
        SDL_RenderDrawPointF(renderer, state->star_x[i], state->star_y[i]);
        if (metrics) {
            metrics->draw_calls++;
            metrics->vertices_rendered++;
        }
    }
}

static void space_render_laser_helix(const SpaceBenchState *state,
                                     SDL_Renderer *renderer,
                                     BenchMetrics *metrics,
                                     float origin_x,
                                     float origin_y,
                                     float target_x,
                                     float target_y,
                                     float amplitude,
                                     float twist_density,
                                     SDL_Color color_a,
                                     SDL_Color color_b)
{
    const float dir_x = target_x - origin_x;
    const float dir_y = target_y - origin_y;
    const float length = SDL_sqrtf(dir_x * dir_x + dir_y * dir_y);
    if (length <= 0.0f) {
        return;
    }

    const int segments = 42;
    const float omega = twist_density * 2.0f * (float)M_PI;
    const float time_phase = state->time_accumulator * 6.0f;
    const float inv_len = 1.0f / length;
    const float dir_nx = dir_x * inv_len;
    const float dir_ny = dir_y * inv_len;
    const float perp_x = -dir_ny;
    const float perp_y = dir_nx;

    for (int strand = 0; strand < 2; ++strand) {
        const SDL_Color color = (strand == 0) ? color_a : color_b;
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

        const float strand_phase = time_phase + strand * (float)M_PI;
        float prev_angle = strand_phase;
        float prev_radial = sinf(prev_angle) * amplitude;
        float prev_forward = cosf(prev_angle) * amplitude * 0.15f;
        float prev_x = origin_x + dir_nx * prev_forward + perp_x * prev_radial;
        float prev_y = origin_y + dir_ny * prev_forward + perp_y * prev_radial;

        for (int i = 1; i <= segments; ++i) {
            const float t = (float)i / (float)segments;
            const float angle = strand_phase + omega * t;
            const float radial = sinf(angle) * amplitude;
            const float forward = (t * length) + cosf(angle) * amplitude * 0.15f;
            const float x = origin_x + dir_nx * forward + perp_x * radial;
            const float y = origin_y + dir_ny * forward + perp_y * radial;
            SDL_RenderDrawLineF(renderer, prev_x, prev_y, x, y);

            if (metrics) {
                metrics->draw_calls += 1;
                metrics->vertices_rendered += 2;
            }

            prev_x = x;
            prev_y = y;
            prev_angle = angle;
        }
    }
}

static void space_render_lasers(const SpaceBenchState *state,
                                SDL_Renderer *renderer,
                                BenchMetrics *metrics)
{
    // Render player laser beam
    if (state->player_laser.is_firing) {
        const float beam_end_x = SPACE_SCREEN_W;
        const float beam_y = state->player_laser.origin_y;

        // Color with intensity modulation for pulsing effect
        const Uint8 intensity = (Uint8)(255 * state->player_laser.intensity);
        SDL_SetRenderDrawColor(renderer, intensity, (Uint8)(220 * state->player_laser.intensity), (Uint8)(160 * state->player_laser.intensity), 255);

        // Draw 3 parallel lines for thickness
        SDL_RenderDrawLineF(renderer, state->player_laser.origin_x, beam_y - 1.0f, beam_end_x, beam_y - 1.0f);
        SDL_RenderDrawLineF(renderer, state->player_laser.origin_x, beam_y, beam_end_x, beam_y);
        SDL_RenderDrawLineF(renderer, state->player_laser.origin_x, beam_y + 1.0f, beam_end_x, beam_y + 1.0f);

        const float player_length = beam_end_x - state->player_laser.origin_x;
        const float player_twist_density = SDL_max(1.6f, player_length / 85.0f);
        SDL_Color helix_a = {
            (Uint8)SDL_min(255, intensity + 40),
            (Uint8)(200 * state->player_laser.intensity),
            255,
            220
        };
        SDL_Color helix_b = {
            190,
            (Uint8)(170 * state->player_laser.intensity),
            (Uint8)SDL_min(255, 180 + intensity / 3),
            200
        };
        space_render_laser_helix(state,
                                 renderer,
                                 metrics,
                                 state->player_laser.origin_x,
                                 beam_y,
                                 beam_end_x,
                                 beam_y,
                                 2.4f,
                                 player_twist_density,
                                 helix_a,
                                 helix_b);

        if (metrics) {
            metrics->draw_calls += 3;
            metrics->vertices_rendered += 6;
        }
    }

    // Render drone laser beams
    for (int d = 0; d < state->weapon_upgrades.drone_count; ++d) {
        if (state->drone_lasers[d].is_firing) {
            const float beam_end_x = SPACE_SCREEN_W;
            const float beam_y = state->drone_lasers[d].origin_y;

            // Slightly dimmer and different color for drones
            const Uint8 intensity = (Uint8)(200 * state->drone_lasers[d].intensity);
            SDL_SetRenderDrawColor(renderer, intensity, (Uint8)(180 * state->drone_lasers[d].intensity), (Uint8)(120 * state->drone_lasers[d].intensity), 255);

            // Draw 2 parallel lines for thinner drone beams
            SDL_RenderDrawLineF(renderer, state->drone_lasers[d].origin_x, beam_y - 0.5f, beam_end_x, beam_y - 0.5f);
            SDL_RenderDrawLineF(renderer, state->drone_lasers[d].origin_x, beam_y + 0.5f, beam_end_x, beam_y + 0.5f);

            const float drone_length = beam_end_x - state->drone_lasers[d].origin_x;
            const float drone_twist_density = SDL_max(1.3f, drone_length / 100.0f);
            SDL_Color helix_a = {
                (Uint8)SDL_min(255, intensity + 50),
                (Uint8)(160 * state->drone_lasers[d].intensity),
                230,
                200
            };
            SDL_Color helix_b = {
                150,
                (Uint8)(140 * state->drone_lasers[d].intensity),
                (Uint8)SDL_min(255, 190 + intensity / 4),
                180
            };
            space_render_laser_helix(state,
                                     renderer,
                                     metrics,
                                     state->drone_lasers[d].origin_x,
                                     beam_y,
                                     beam_end_x,
                                     beam_y,
                                     1.8f,
                                     drone_twist_density,
                                     helix_a,
                                     helix_b);

            if (metrics) {
                metrics->draw_calls += 2;
                metrics->vertices_rendered += 4;
            }
        }
    }
}

static void space_render_bullets(const SpaceBenchState *state,
                                 SDL_Renderer *renderer,
                                 BenchMetrics *metrics)
{
    SDL_SetRenderDrawColor(renderer, 255, 230, 120, 255);
    for (int i = 0; i < SPACE_MAX_BULLETS; ++i) {
        const SpaceBullet *bullet = &state->bullets[i];
        if (!bullet->active) {
            continue;
        }
        SDL_FRect rect = {bullet->x - 3.0f, bullet->y - 1.5f, 6.0f, 3.0f};
        SDL_RenderFillRectF(renderer, &rect);
        if (metrics) {
            metrics->draw_calls++;
            metrics->vertices_rendered += 4;
        }
    }
}

static void space_render_particles(const SpaceBenchState *state,
                                   SDL_Renderer *renderer,
                                   BenchMetrics *metrics)
{
    for (int i = 0; i < SPACE_MAX_PARTICLES; ++i) {
        const SpaceParticle *particle = &state->particles[i];
        if (!particle->active) {
            continue;
        }

        // Fade particle based on remaining life
        const float life_ratio = particle->life / particle->max_life;
        const float fade = SDL_clamp(life_ratio, 0.0f, 1.0f);
        const Uint8 alpha = (Uint8)(255 * fade);
        const Uint8 r = (Uint8)(particle->r * fade);
        const Uint8 g = (Uint8)(particle->g * fade);
        const Uint8 b = (Uint8)(particle->b * fade);

        SDL_SetRenderDrawColor(renderer, r, g, b, alpha);
        SDL_RenderDrawPointF(renderer, particle->x, particle->y);

        if (metrics) {
            metrics->vertices_rendered += 1;
        }
    }
    if (metrics) {
        metrics->draw_calls += 1;
    }
}

static void space_render_enemies(const SpaceBenchState *state,
                                 SDL_Renderer *renderer,
                                 BenchMetrics *metrics)
{
    for (int i = 0; i < SPACE_MAX_ENEMIES; ++i) {
        const SpaceEnemy *enemy = &state->enemies[i];
        if (!enemy->active) {
            continue;
        }

        const float origin_x = enemy->x;
        const float origin_y = enemy->y;
        const float roll = enemy->rotation;
        const float r = enemy->radius;

        // Wireframe pyramid for enemies (flipped 180 degrees)
        const float scale = r * 0.8f;
        const SpaceVec3 local_apex = {-scale * 1.2f, 0.0f, 0.0f};
        const SpaceVec3 local_base_a = {scale, -scale, -scale};
        const SpaceVec3 local_base_b = {scale, scale, -scale};
        const SpaceVec3 local_base_c = {scale, scale, scale};
        const SpaceVec3 local_base_d = {scale, -scale, scale};

        SpaceVec3 apex = space_rotate_roll(local_apex, roll);
        SpaceVec3 base_a = space_rotate_roll(local_base_a, roll);
        SpaceVec3 base_b = space_rotate_roll(local_base_b, roll);
        SpaceVec3 base_c = space_rotate_roll(local_base_c, roll);
        SpaceVec3 base_d = space_rotate_roll(local_base_d, roll);

        const SDL_FPoint apex_pt = space_project_point(apex, origin_x, origin_y);
        const SDL_FPoint base_a_pt = space_project_point(base_a, origin_x, origin_y);
        const SDL_FPoint base_b_pt = space_project_point(base_b, origin_x, origin_y);
        const SDL_FPoint base_c_pt = space_project_point(base_c, origin_x, origin_y);
        const SDL_FPoint base_d_pt = space_project_point(base_d, origin_x, origin_y);

        // Different colors for enemies based on index
        Uint8 red = 255 - (i * 17) % 100;
        Uint8 green = 100 + (i * 31) % 155;
        Uint8 blue = 150 + (i * 23) % 105;
        SDL_SetRenderDrawColor(renderer, red, green, blue, 220);

        // Pyramid edges
        SDL_RenderDrawLineF(renderer, apex_pt.x, apex_pt.y, base_a_pt.x, base_a_pt.y);
        SDL_RenderDrawLineF(renderer, apex_pt.x, apex_pt.y, base_b_pt.x, base_b_pt.y);
        SDL_RenderDrawLineF(renderer, apex_pt.x, apex_pt.y, base_c_pt.x, base_c_pt.y);
        SDL_RenderDrawLineF(renderer, apex_pt.x, apex_pt.y, base_d_pt.x, base_d_pt.y);

        // Base square
        SDL_RenderDrawLineF(renderer, base_a_pt.x, base_a_pt.y, base_b_pt.x, base_b_pt.y);
        SDL_RenderDrawLineF(renderer, base_b_pt.x, base_b_pt.y, base_c_pt.x, base_c_pt.y);
        SDL_RenderDrawLineF(renderer, base_c_pt.x, base_c_pt.y, base_d_pt.x, base_d_pt.y);
        SDL_RenderDrawLineF(renderer, base_d_pt.x, base_d_pt.y, base_a_pt.x, base_a_pt.y);

        if (metrics) {
            metrics->draw_calls += 8;
            metrics->vertices_rendered += 16;
        }

        // Engine trail at back of ship (right side since ships move left)
        SDL_SetRenderDrawColor(renderer, 255, 200, 80, 130);
        const float exhaust_x = origin_x + r * 2.0f;  // Behind the ship
        SDL_RenderDrawLineF(renderer, base_a_pt.x, base_a_pt.y, exhaust_x, origin_y - r * 0.3f);
        SDL_RenderDrawLineF(renderer, base_d_pt.x, base_d_pt.y, exhaust_x, origin_y + r * 0.3f);
        if (metrics) {
            metrics->draw_calls += 2;
            metrics->vertices_rendered += 4;
        }
    }
}

static void space_render_drones(const SpaceBenchState *state,
                                  SDL_Renderer *renderer,
                                  BenchMetrics *metrics)
{
    // Render drones in z-order (back to front)
    typedef struct {
        int index;
        float z;
    } DroneOrder;
    DroneOrder drone_order[SPACE_MAX_DRONES];

    int active_drones = 0;
    for (int i = 0; i < state->weapon_upgrades.drone_count; ++i) {
        if (state->drones[i].active) {
            drone_order[active_drones].index = i;
            drone_order[active_drones].z = state->drones[i].z;
            active_drones++;
        }
    }

    // Sort by z (back to front)
    for (int i = 0; i < active_drones - 1; ++i) {
        for (int j = i + 1; j < active_drones; ++j) {
            if (drone_order[i].z > drone_order[j].z) {
                DroneOrder temp = drone_order[i];
                drone_order[i] = drone_order[j];
                drone_order[j] = temp;
            }
        }
    }

    for (int d = 0; d < active_drones; ++d) {
        const int i = drone_order[d].index;
        const SpaceDrone *ship = &state->drones[i];

        const float origin_x = ship->x;
        const float origin_y = ship->y;
        const float roll = state->player_roll + ship->angle * 0.5f;

        // Wireframe pyramid for drones
        const SpaceVec3 local_apex = {8.0f, 0.0f, 0.0f};
        const SpaceVec3 local_base_a = {-4.0f, -4.0f, -4.0f};
        const SpaceVec3 local_base_b = {-4.0f, 4.0f, -4.0f};
        const SpaceVec3 local_base_c = {-4.0f, 4.0f, 4.0f};
        const SpaceVec3 local_base_d = {-4.0f, -4.0f, 4.0f};

        SpaceVec3 apex = space_rotate_roll(local_apex, roll);
        SpaceVec3 base_a = space_rotate_roll(local_base_a, roll);
        SpaceVec3 base_b = space_rotate_roll(local_base_b, roll);
        SpaceVec3 base_c = space_rotate_roll(local_base_c, roll);
        SpaceVec3 base_d = space_rotate_roll(local_base_d, roll);

        // Apply z offset from orbit
        apex.z += ship->z * 0.08f;
        base_a.z += ship->z * 0.08f;
        base_b.z += ship->z * 0.08f;
        base_c.z += ship->z * 0.08f;
        base_d.z += ship->z * 0.08f;

        const SDL_FPoint apex_pt = space_project_point(apex, origin_x, origin_y);
        const SDL_FPoint base_a_pt = space_project_point(base_a, origin_x, origin_y);
        const SDL_FPoint base_b_pt = space_project_point(base_b, origin_x, origin_y);
        const SDL_FPoint base_c_pt = space_project_point(base_c, origin_x, origin_y);
        const SDL_FPoint base_d_pt = space_project_point(base_d, origin_x, origin_y);

        // Draw wireframe pyramid
        SDL_SetRenderDrawColor(renderer, 140, 220, 255, 230);

        // Pyramid edges
        SDL_RenderDrawLineF(renderer, apex_pt.x, apex_pt.y, base_a_pt.x, base_a_pt.y);
        SDL_RenderDrawLineF(renderer, apex_pt.x, apex_pt.y, base_b_pt.x, base_b_pt.y);
        SDL_RenderDrawLineF(renderer, apex_pt.x, apex_pt.y, base_c_pt.x, base_c_pt.y);
        SDL_RenderDrawLineF(renderer, apex_pt.x, apex_pt.y, base_d_pt.x, base_d_pt.y);

        // Base square
        SDL_RenderDrawLineF(renderer, base_a_pt.x, base_a_pt.y, base_b_pt.x, base_b_pt.y);
        SDL_RenderDrawLineF(renderer, base_b_pt.x, base_b_pt.y, base_c_pt.x, base_c_pt.y);
        SDL_RenderDrawLineF(renderer, base_c_pt.x, base_c_pt.y, base_d_pt.x, base_d_pt.y);
        SDL_RenderDrawLineF(renderer, base_d_pt.x, base_d_pt.y, base_a_pt.x, base_a_pt.y);

        if (metrics) {
            metrics->draw_calls += 8;
            metrics->vertices_rendered += 16;
        }
    }
}

static void space_render_anomaly(const SpaceBenchState *state,
                                 SDL_Renderer *renderer,
                                 BenchMetrics *metrics)
{
    if (!state->anomaly.active) {
        return;
    }

    const SpaceAnomaly *anomaly = &state->anomaly;
    int draw_calls = 0;
    int vertices = 0;

    // Render HP bar above anomaly
    if (anomaly->current_layer < 5) {
        const float bar_width = 80.0f;
        const float bar_height = 6.0f;
        const float bar_x = anomaly->x - bar_width * 0.5f;
        const float bar_y = anomaly->y - anomaly->scale * 1.5f - 20.0f;

        // Background
        SDL_SetRenderDrawColor(renderer, 40, 40, 40, 200);
        SDL_FRect bg_rect = {bar_x - 1, bar_y - 1, bar_width + 2, bar_height + 2};
        SDL_RenderFillRectF(renderer, &bg_rect);

        // Current layer HP
        float hp_ratio = anomaly->layer_health[anomaly->current_layer] / anomaly->layer_max_health[anomaly->current_layer];
        hp_ratio = SDL_max(0.0f, SDL_min(1.0f, hp_ratio));

        // Color based on layer
        SDL_Color hp_colors[] = {
            {255, 100, 100, 255},  // Red - outer
            {255, 180, 100, 255},  // Orange - mid2
            {255, 255, 100, 255},  // Yellow - mid1
            {100, 255, 100, 255},  // Green - inner
            {100, 100, 255, 255}   // Blue - core
        };
        SDL_Color hp_color = hp_colors[anomaly->current_layer];

        SDL_SetRenderDrawColor(renderer, hp_color.r, hp_color.g, hp_color.b, hp_color.a);
        SDL_FRect hp_rect = {bar_x, bar_y, bar_width * hp_ratio, bar_height};
        SDL_RenderFillRectF(renderer, &hp_rect);

        draw_calls += 2;
        vertices += 8;
    }

    // Core layer - visual only
    if (anomaly->core_points[0].active) {
        SDL_SetRenderDrawColor(renderer, 255, 255, 100, 200);
        for (int i = 0; i < 20; ++i) {
            const float angle = anomaly->core_points[i].angle + anomaly->core_rotation;
            const float x = anomaly->x + cosf(angle) * anomaly->core_points[i].radius;
            const float y = anomaly->y + sinf(angle) * anomaly->core_points[i].radius;
            SDL_RenderDrawPointF(renderer, x, y);
            vertices++;
        }
        draw_calls++;
    }

    // Inner layer - lasers
    if (anomaly->inner_points[0].active) {
        SDL_SetRenderDrawColor(renderer, 100, 100, 255, 220);
        for (int i = 0; i < 4; ++i) {
            if (!anomaly->inner_points[i].active) continue;
            const float angle = anomaly->inner_points[i].angle + anomaly->inner_rotation;
            const float x = anomaly->x + cosf(angle) * anomaly->inner_points[i].radius;
            const float y = anomaly->y + sinf(angle) * anomaly->inner_points[i].radius;
            SDL_FRect launcher = {x - 1.0f, y - 1.0f, 2.0f, 2.0f};
            SDL_RenderFillRectF(renderer, &launcher);
            vertices += 4;
            draw_calls++;
        }
    }

    // Mid1 layer - heavy shots
    if (anomaly->mid1_points[0].active) {
        SDL_SetRenderDrawColor(renderer, 255, 255, 100, 200);
        for (int i = 0; i < 6; ++i) {
            if (!anomaly->mid1_points[i].active) continue;
            const float angle = anomaly->mid1_points[i].angle + anomaly->mid1_rotation;
            const float x = anomaly->x + cosf(angle) * anomaly->mid1_points[i].radius;
            const float y = anomaly->y + sinf(angle) * anomaly->mid1_points[i].radius;
            SDL_FRect launcher = {x - 1.0f, y - 1.0f, 2.0f, 2.0f};
            SDL_RenderFillRectF(renderer, &launcher);
            vertices += 4;
            draw_calls++;
        }
    }

    // Rotating laser ring
    for (int i = 0; i < SPACE_ANOMALY_MID_LASER_COUNT; ++i) {
        const AnomalyLaser *laser = &anomaly->lasers[i];
        if (!laser->active) {
            continue;
        }

        const float dir_x = laser->target_x - laser->x;
        const float dir_y = laser->target_y - laser->y;
        const float len = SDL_sqrtf(dir_x * dir_x + dir_y * dir_y);
        if (len <= 0.0f) {
            continue;
        }

        const float inv_len = 1.0f / len;
        const float perp_x = -dir_y * inv_len;
        const float perp_y = dir_x * inv_len;
        const float thickness = 1.5f;
        const float fade = SDL_clamp(laser->fade, 0.0f, 1.0f);
        const Uint8 main_alpha = (Uint8)(200 * fade);
        const Uint8 core_alpha = (Uint8)(255 * fade);
        const Uint8 pulse_alpha = (Uint8)(180 * fade);

        SDL_SetRenderDrawColor(renderer, 255, 210, 120, main_alpha);
        SDL_RenderDrawLineF(renderer,
                            laser->x - perp_x * thickness,
                            laser->y - perp_y * thickness,
                            laser->target_x - perp_x * thickness,
                            laser->target_y - perp_y * thickness);
        SDL_RenderDrawLineF(renderer,
                            laser->x + perp_x * thickness,
                            laser->y + perp_y * thickness,
                            laser->target_x + perp_x * thickness,
                            laser->target_y + perp_y * thickness);
        SDL_SetRenderDrawColor(renderer, 255, 255, 200, core_alpha);
        SDL_RenderDrawLineF(renderer, laser->x, laser->y, laser->target_x, laser->target_y);

        SDL_Color helix_primary = {255, 180, 120, pulse_alpha};
        SDL_Color helix_secondary = {255, 140, 220, (Uint8)(150 * fade)};
        space_render_laser_helix(state,
                                 renderer,
                                 metrics,
                                 laser->x,
                                 laser->y,
                                 laser->target_x,
                                 laser->target_y,
                                 1.2f * fade,
                                 2.0f,
                                 helix_primary,
                                 helix_secondary);

        draw_calls += 3;
        vertices += 6;
    }

    // Mid2 layer - rapid fire
    if (anomaly->mid2_points[0].active) {
        SDL_SetRenderDrawColor(renderer, 255, 180, 100, 200);
        for (int i = 0; i < 8; ++i) {
            if (!anomaly->mid2_points[i].active) continue;
            const float angle = anomaly->mid2_points[i].angle + anomaly->mid2_rotation;
            const float x = anomaly->x + cosf(angle) * anomaly->mid2_points[i].radius;
            const float y = anomaly->y + sinf(angle) * anomaly->mid2_points[i].radius;
            SDL_FRect launcher = {x - 1.0f, y - 1.0f, 2.0f, 2.0f};
            SDL_RenderFillRectF(renderer, &launcher);
            vertices += 4;
            draw_calls++;
        }
    }

    // Outer layer - missile launchers
    if (anomaly->outer_points[0].active) {
        SDL_SetRenderDrawColor(renderer, 255, 80, 80, 220);
        for (int i = 0; i < 10; ++i) {
            if (!anomaly->outer_points[i].active) continue;
            const float angle = anomaly->outer_points[i].angle + anomaly->outer_rotation;
            const float x = anomaly->x + cosf(angle) * anomaly->outer_points[i].radius;
            const float y = anomaly->y + sinf(angle) * anomaly->outer_points[i].radius;
            SDL_FRect launcher = {x - 1.0f, y - 1.0f, 2.0f, 2.0f};
            SDL_RenderFillRectF(renderer, &launcher);
            vertices += 4;
            draw_calls++;
        }
    }

    // Render orbital points around anomaly center
    for (int i = 0; i < 64; ++i) {
        const AnomalyOrbitalPoint *point = &anomaly->orbital_points[i];
        if (!point->active) continue;

        const float x = anomaly->x + cosf(point->angle) * point->radius;
        const float y = anomaly->y + sinf(point->angle) * point->radius;

        // Color based on distance from center
        const float dist_factor = point->radius / (anomaly->scale * 1.5f);
        const Uint8 red = (Uint8)(100 + 155 * dist_factor);
        const Uint8 green = (Uint8)(150 - 50 * dist_factor);
        const Uint8 blue = (Uint8)(255 - 100 * dist_factor);

        SDL_SetRenderDrawColor(renderer, red, green, blue, 180);
        SDL_RenderDrawPointF(renderer, x, y);
        draw_calls++;
        vertices++;
    }

    // Render anomaly shield
    if (anomaly->shield_active && anomaly->shield_strength > 0.0f) {
        const float shield_radius = anomaly->scale * 2.0f;
        const float pulse_factor = 0.7f + 0.3f * sinf(anomaly->shield_pulse);
        const float shield_alpha = (anomaly->shield_strength / anomaly->shield_max_strength) * 120.0f * pulse_factor;

        // Draw shield circle
        const int segments = 32;
        for (int i = 0; i < segments; ++i) {
            const float angle1 = (float)(M_PI * 2.0) * (float)i / (float)segments;
            const float angle2 = (float)(M_PI * 2.0) * (float)(i + 1) / (float)segments;

            const float x1 = anomaly->x + cosf(angle1) * shield_radius;
            const float y1 = anomaly->y + sinf(angle1) * shield_radius;
            const float x2 = anomaly->x + cosf(angle2) * shield_radius;
            const float y2 = anomaly->y + sinf(angle2) * shield_radius;

            SDL_SetRenderDrawColor(renderer, 100, 200, 255, (Uint8)shield_alpha);
            SDL_RenderDrawLineF(renderer, x1, y1, x2, y2);
            draw_calls++;
        }
    }

    if (metrics) {
        metrics->draw_calls += draw_calls;
        metrics->vertices_rendered += vertices;
    }
}

static void space_render_explosions(const SpaceBenchState *state,
                                    SDL_Renderer *renderer,
                                    BenchMetrics *metrics)
{
    for (int i = 0; i < SPACE_MAX_EXPLOSIONS; ++i) {
        const SpaceExplosion *exp = &state->explosions[i];
        if (!exp->active) {
            continue;
        }

        const float t = exp->timer / exp->lifetime;
        const float radius = exp->radius * (0.4f + 0.6f * t);
        const int segments = 12;
        SDL_FPoint ring[segments + 1];
        for (int s = 0; s <= segments; ++s) {
            const float angle = (float)(M_PI * 2.0) * (float)s / (float)segments;
            ring[s].x = exp->x + cosf(angle) * radius;
            ring[s].y = exp->y + sinf(angle) * radius;
        }
        const Uint8 alpha = (Uint8)(255 * (1.0f - t));
        SDL_SetRenderDrawColor(renderer, 255, 190, 90, alpha);
        SDL_RenderDrawLinesF(renderer, ring, segments + 1);
        if (metrics) {
            metrics->draw_calls++;
            metrics->vertices_rendered += segments + 1;
        }
    }
}

static void space_render_player(const SpaceBenchState *state,
                                SDL_Renderer *renderer,
                                BenchMetrics *metrics)
{
    const float origin_x = state->player_x;
    const float origin_y = state->player_y;
    const float roll = state->player_roll;

    // Render shield first if active
    if (state->shield_active && state->shield_strength > 0.0f) {
        const float shield_radius = state->player_radius + 6.0f;
        const float pulse = sinf(state->shield_pulse * 10.0f) * 0.3f + 0.7f;
        const float noise_factor = sinf(state->shield_pulse * 15.0f) * 2.0f;

        const int segments = 16;
        SDL_FPoint shield_points[segments + 1];
        for (int i = 0; i <= segments; ++i) {
            const float angle = (float)(M_PI * 2.0) * (float)i / (float)segments;
            const float radius_variation = shield_radius + noise_factor * sinf(angle * 3.0f);
            shield_points[i].x = origin_x + cosf(angle) * radius_variation;
            shield_points[i].y = origin_y + sinf(angle) * radius_variation;
        }

        const Uint8 shield_alpha = (Uint8)(120 * pulse * (state->shield_strength / state->shield_max_strength));
        SDL_SetRenderDrawColor(renderer, 100, 255, 150, shield_alpha);
        SDL_RenderDrawLinesF(renderer, shield_points, segments + 1);

        if (metrics) {
            metrics->draw_calls++;
            metrics->vertices_rendered += segments + 1;
        }
    }

    if (state->weapon_upgrades.thumper_active) {
        const float pulse_time = state->weapon_upgrades.thumper_pulse_timer;
        if (pulse_time < 0.45f) {
            const float t = SDL_clamp(pulse_time / 0.3f, 0.0f, 1.0f);
            const float ring_radius = state->player_radius + 10.0f + t * 24.0f;
            const Uint8 ring_alpha = (Uint8)(170 * (1.0f - t));
            const int ring_segments = 24;
            SDL_SetRenderDrawColor(renderer, 160, 220, 255, ring_alpha);
            for (int i = 0; i < ring_segments; ++i) {
                const float angle1 = (float)(M_PI * 2.0) * (float)i / (float)ring_segments;
                const float angle2 = (float)(M_PI * 2.0) * (float)(i + 1) / (float)ring_segments;
                const float x1 = origin_x + cosf(angle1) * ring_radius;
                const float y1 = origin_y + sinf(angle1) * ring_radius;
                const float x2 = origin_x + cosf(angle2) * ring_radius;
                const float y2 = origin_y + sinf(angle2) * ring_radius;
                SDL_RenderDrawLineF(renderer, x1, y1, x2, y2);
            }

            if (metrics) {
                metrics->draw_calls += ring_segments;
                metrics->vertices_rendered += ring_segments * 2;
            }
        }
    }

    // Wireframe pyramid geometry
    const SpaceVec3 local_apex = {14.0f, 0.0f, 0.0f};
    const SpaceVec3 local_base_a = {-8.0f, -8.0f, -8.0f};
    const SpaceVec3 local_base_b = {-8.0f, 8.0f, -8.0f};
    const SpaceVec3 local_base_c = {-8.0f, 8.0f, 8.0f};
    const SpaceVec3 local_base_d = {-8.0f, -8.0f, 8.0f};

    SpaceVec3 apex = space_rotate_roll(local_apex, roll);
    SpaceVec3 base_a = space_rotate_roll(local_base_a, roll);
    SpaceVec3 base_b = space_rotate_roll(local_base_b, roll);
    SpaceVec3 base_c = space_rotate_roll(local_base_c, roll);
    SpaceVec3 base_d = space_rotate_roll(local_base_d, roll);

    const SDL_FPoint apex_pt = space_project_point(apex, origin_x, origin_y);
    const SDL_FPoint base_a_pt = space_project_point(base_a, origin_x, origin_y);
    const SDL_FPoint base_b_pt = space_project_point(base_b, origin_x, origin_y);
    const SDL_FPoint base_c_pt = space_project_point(base_c, origin_x, origin_y);
    const SDL_FPoint base_d_pt = space_project_point(base_d, origin_x, origin_y);

    // Draw wireframe pyramid
    SDL_SetRenderDrawColor(renderer, 255, 200, 120, 255);

    // Pyramid edges from apex to base corners
    SDL_RenderDrawLineF(renderer, apex_pt.x, apex_pt.y, base_a_pt.x, base_a_pt.y);
    SDL_RenderDrawLineF(renderer, apex_pt.x, apex_pt.y, base_b_pt.x, base_b_pt.y);
    SDL_RenderDrawLineF(renderer, apex_pt.x, apex_pt.y, base_c_pt.x, base_c_pt.y);
    SDL_RenderDrawLineF(renderer, apex_pt.x, apex_pt.y, base_d_pt.x, base_d_pt.y);

    // Base square
    SDL_RenderDrawLineF(renderer, base_a_pt.x, base_a_pt.y, base_b_pt.x, base_b_pt.y);
    SDL_RenderDrawLineF(renderer, base_b_pt.x, base_b_pt.y, base_c_pt.x, base_c_pt.y);
    SDL_RenderDrawLineF(renderer, base_c_pt.x, base_c_pt.y, base_d_pt.x, base_d_pt.y);
    SDL_RenderDrawLineF(renderer, base_d_pt.x, base_d_pt.y, base_a_pt.x, base_a_pt.y);

    if (metrics) {
        metrics->draw_calls += 8;
        metrics->vertices_rendered += 16;
    }

    // Engine glow
    SDL_SetRenderDrawColor(renderer, 120, 200, 255, 180);
    const float exhaust_x = origin_x - 15.0f;
    SDL_RenderDrawLineF(renderer, base_b_pt.x, base_b_pt.y, exhaust_x, origin_y);
    SDL_RenderDrawLineF(renderer, base_c_pt.x, base_c_pt.y, exhaust_x, origin_y);

    if (metrics) {
        metrics->draw_calls += 2;
        metrics->vertices_rendered += 4;
    }
}

static void space_render_game_over(const SpaceBenchState *state,
                                   SDL_Renderer *renderer,
                                   BenchMetrics *metrics)
{
    // Render stars behind overlay
    space_render_stars(state, renderer, metrics);

    // Semi-transparent black overlay
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 140);  // Lighter overlay to show stars
    SDL_Rect overlay_rect = {0, (int)state->play_area_top, SPACE_SCREEN_W, (int)(state->play_area_bottom - state->play_area_top)};
    SDL_RenderFillRect(renderer, &overlay_rect);

    // Calculate center positions
    const float center_x = SPACE_SCREEN_W * 0.5f;
    const float center_y = (state->play_area_top + state->play_area_bottom) * 0.5f;

    // Try to load font for text rendering
    TTF_Font *font = bench_load_font(24);
    TTF_Font *small_font = bench_load_font(16);

    if (font) {
        // Render "GAME OVER" with TTF
        SDL_Color text_color = {255, 80, 80, 255};
        SDL_Surface *text_surface = TTF_RenderUTF8_Blended(font, "GAME OVER", text_color);
        if (text_surface) {
            SDL_Texture *text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
            if (text_texture) {
                SDL_Rect text_rect = {
                    (int)(center_x - text_surface->w * 0.5f),
                    (int)(center_y - 60.0f),
                    text_surface->w,
                    text_surface->h
                };
                SDL_RenderCopy(renderer, text_texture, NULL, &text_rect);
                SDL_DestroyTexture(text_texture);
            }
            SDL_FreeSurface(text_surface);
        }

        // Show score and kills
        SDL_Color stats_color = {255, 255, 255, 255};
        char stats_text[64];
        SDL_snprintf(stats_text, sizeof(stats_text), "Score: %d  Enemies Killed: %d", state->score, state->total_enemies_killed);

        SDL_Surface *stats_surface = TTF_RenderUTF8_Blended(small_font ? small_font : font, stats_text, stats_color);
        if (stats_surface) {
            SDL_Texture *stats_texture = SDL_CreateTextureFromSurface(renderer, stats_surface);
            if (stats_texture) {
                SDL_Rect stats_rect = {
                    (int)(center_x - stats_surface->w * 0.5f),
                    (int)(center_y - 20.0f),
                    stats_surface->w,
                    stats_surface->h
                };
                SDL_RenderCopy(renderer, stats_texture, NULL, &stats_rect);
                SDL_DestroyTexture(stats_texture);
            }
            SDL_FreeSurface(stats_surface);
        }

        // Show countdown if active
        if (state->gameover_countdown > 0.0f) {
            SDL_Color countdown_color = {255, 255, 100, 200};
            const int countdown_seconds = (int)(state->gameover_countdown) + 1;
            char countdown_text[32];
            SDL_snprintf(countdown_text, sizeof(countdown_text), "Select in %d", countdown_seconds);

            SDL_Surface *countdown_surface = TTF_RenderUTF8_Blended(small_font ? small_font : font, countdown_text, countdown_color);
            if (countdown_surface) {
                SDL_Texture *countdown_texture = SDL_CreateTextureFromSurface(renderer, countdown_surface);
                if (countdown_texture) {
                    SDL_Rect countdown_rect = {
                        (int)(center_x - countdown_surface->w * 0.5f),
                        (int)(center_y + 60.0f),
                        countdown_surface->w,
                        countdown_surface->h
                    };
                    SDL_RenderCopy(renderer, countdown_texture, NULL, &countdown_rect);
                    SDL_DestroyTexture(countdown_texture);
                }
                SDL_FreeSurface(countdown_surface);
            }
        }

        // Retry option
        SDL_Color retry_color = (state->gameover_selected == SPACE_GAMEOVER_RETRY) ?
                               (SDL_Color){255, 255, 120, 255} : (SDL_Color){180, 180, 180, 255};
        SDL_Surface *retry_surface = TTF_RenderUTF8_Blended(small_font ? small_font : font, "RETRY", retry_color);
        if (retry_surface) {
            SDL_Texture *retry_texture = SDL_CreateTextureFromSurface(renderer, retry_surface);
            if (retry_texture) {
                SDL_Rect retry_rect = {
                    (int)(center_x - 100.0f - retry_surface->w * 0.5f),
                    (int)(center_y + 20.0f),
                    retry_surface->w,
                    retry_surface->h
                };
                SDL_RenderCopy(renderer, retry_texture, NULL, &retry_rect);
                SDL_DestroyTexture(retry_texture);
            }
            SDL_FreeSurface(retry_surface);
        }

        // Quit option
        SDL_Color quit_color = (state->gameover_selected == SPACE_GAMEOVER_QUIT) ?
                              (SDL_Color){255, 255, 120, 255} : (SDL_Color){180, 180, 180, 255};
        SDL_Surface *quit_surface = TTF_RenderUTF8_Blended(small_font ? small_font : font, "QUIT", quit_color);
        if (quit_surface) {
            SDL_Texture *quit_texture = SDL_CreateTextureFromSurface(renderer, quit_surface);
            if (quit_texture) {
                SDL_Rect quit_rect = {
                    (int)(center_x + 100.0f - quit_surface->w * 0.5f),
                    (int)(center_y + 20.0f),
                    quit_surface->w,
                    quit_surface->h
                };
                SDL_RenderCopy(renderer, quit_texture, NULL, &quit_rect);
                SDL_DestroyTexture(quit_texture);
            }
            SDL_FreeSurface(quit_surface);
        }

        TTF_CloseFont(font);
        if (small_font) TTF_CloseFont(small_font);
    } else {
        // Fallback to rectangle rendering if no font available
        const float char_width = 8.0f;
        const float char_height = 16.0f;
        SDL_SetRenderDrawColor(renderer, 255, 80, 80, 255);
        const char* game_over_text = "GAME OVER";
        const float text_width = strlen(game_over_text) * (char_width + 2.0f) - 2.0f;
        const float text_start_x = center_x - text_width * 0.5f;
        for (int i = 0; game_over_text[i] != '\0'; ++i) {
            if (game_over_text[i] != ' ') {
                SDL_FRect char_rect = {text_start_x + i * (char_width + 2.0f), center_y - 60.0f, char_width, char_height};
                SDL_RenderFillRectF(renderer, &char_rect);
            }
        }
    }

    if (metrics) {
        metrics->draw_calls += 4;
    }
}

void space_render_scene(SpaceBenchState *state,
                        SDL_Renderer *renderer,
                        BenchMetrics *metrics)
{
    if (!state || !renderer) {
        return;
    }

    // Set clip rect to limit rendering to game area below overlay
    const int overlay_height = (int)state->play_area_top;
    const int game_height = (int)(state->play_area_bottom - state->play_area_top);
    SDL_Rect game_clip = {0, overlay_height, SPACE_SCREEN_W, game_height};
    SDL_RenderSetClipRect(renderer, &game_clip);

    space_render_background(renderer, metrics);
    space_render_stars(state, renderer, metrics);
    space_render_upgrades(state, renderer, metrics);
    space_render_lasers(state, renderer, metrics);
    space_render_enemy_shots(state, renderer, metrics);
    space_render_bullets(state, renderer, metrics);
    space_render_particles(state, renderer, metrics);

    if (state->anomaly_pending && state->anomaly_warning_timer > 0.0f) {
        const float flash = 0.5f + 0.5f * sinf(state->anomaly_warning_phase * 3.2f);
        const float alpha = 140.0f + 100.0f * flash;
        const float warning_y = state->play_area_top + 8.0f;
        char warn_text[64];
        const float time_left = SDL_max(0.0f, state->anomaly_warning_timer);
        const int seconds_left = (int)ceilf(time_left);
        SDL_snprintf(warn_text, sizeof(warn_text), "ANOMALY DETECTED... %ds", seconds_left);

        TTF_Font *warn_font = bench_load_font(18);
        if (warn_font) {
            SDL_Color warn_color = {255, (Uint8)(140 + 80 * flash), (Uint8)(40 + 60 * flash), (Uint8)alpha};
            SDL_Surface *warn_surface = TTF_RenderUTF8_Blended(warn_font, warn_text, warn_color);
            if (warn_surface) {
                SDL_Texture *warn_texture = SDL_CreateTextureFromSurface(renderer, warn_surface);
                if (warn_texture) {
                    const SDL_Rect warn_rect = {
                        (int)(SPACE_SCREEN_W * 0.5f - warn_surface->w * 0.5f),
                        (int)warning_y,
                        warn_surface->w,
                        warn_surface->h
                    };
                    SDL_SetTextureAlphaMod(warn_texture, (Uint8)alpha);
                    SDL_RenderCopy(renderer, warn_texture, NULL, &warn_rect);
                    SDL_DestroyTexture(warn_texture);
                }
                SDL_FreeSurface(warn_surface);
            }
            TTF_CloseFont(warn_font);
        } else {
            SDL_SetRenderDrawColor(renderer, 255, 120, 60, (Uint8)alpha);
            const int bar_width = SPACE_SCREEN_W - 40;
            SDL_RenderDrawLine(renderer, 20, (int)warning_y, 20 + bar_width, (int)warning_y);
        }
    }
    space_render_trail(&state->player_trail,
                       renderer,
                       metrics,
                       (SDL_Color){120, 210, 255, 200},
                       (SDL_Color){255, 255, 200, 200});
    for (int i = 0; i < state->weapon_upgrades.drone_count; ++i) {
        space_render_trail(&state->drone_trails[i],
                           renderer,
                           metrics,
                           (SDL_Color){160, 120, 255, 200},
                           (SDL_Color){255, 200, 255, 200});
    }
    for (int i = 0; i < SPACE_MAX_ENEMIES; ++i) {
        space_render_trail(&state->enemy_trails[i],
                           renderer,
                           metrics,
                           (SDL_Color){255, 140, 120, 200},
                           (SDL_Color){255, 200, 160, 160});
    }
    space_render_enemies(state, renderer, metrics);
    space_render_anomaly(state, renderer, metrics);
    space_render_drones(state, renderer, metrics);
    space_render_explosions(state, renderer, metrics);
    space_render_player(state, renderer, metrics);

    // Render game over overlay if needed
    if (state->game_state == SPACE_GAME_OVER) {
        space_render_game_over(state, renderer, metrics);
    }

    // Reset clip rect to full screen
    SDL_RenderSetClipRect(renderer, NULL);
}
