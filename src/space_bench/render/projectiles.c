
#include "space_bench/render/internal.h"
#include "space_bench/gl_effects.h"

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

/* Tiles one beam cross-section texture, with optional muzzle-height taper. */
static int space_render_laser_tile_strip(SDL_Renderer *renderer,
                                         SDL_Texture *tex,
                                         float origin_x,
                                         float origin_y,
                                         float end_x,
                                         float base_height,
                                         float cone_height,
                                         float cone_length,
                                         SDL_Color tint)
{
    if (!tex || base_height <= 0.0f) {
        return 0;
    }
    SDL_SetTextureColorMod(tex, tint.r, tint.g, tint.b);

    int tiles = 0;
    float x = origin_x;
    while (x < end_x) {
        const float dist = x - origin_x;
        const SDL_bool in_cone = (cone_length > 0.0f) && (dist < cone_length);

        /* Smaller cone steps keep the muzzle flare visibly tapered. */
        const float step = in_cone ? SDL_max(8.0f, cone_length * 0.25f) : (float)SPACE_GL_LASER_TILE_W;
        const float tile_w = SDL_min(step, end_x - x + 1.0f);

        float height = base_height;
        if (in_cone) {
            const float t = SDL_min(1.0f, (dist + tile_w * 0.5f) / cone_length);
            height = cone_height + (base_height - cone_height) * t;
        }

        const SDL_FRect dst = {x, origin_y - height * 0.5f, tile_w, height};
        SDL_RenderCopyF(renderer, tex, NULL, &dst);
        x += step;
        tiles++;
    }
    return tiles;
}

/* GL-shaded beam with a flat-line fallback. */
static void space_render_laser_beam(SDL_Renderer *renderer,
                                    BenchMetrics *metrics,
                                    float origin_x,
                                    float origin_y,
                                    float end_x,
                                    SDL_Color color,
                                    float thickness_scale)
{
    const float length = end_x - origin_x;
    if (length <= 0.5f) {
        return;
    }

    SDL_Texture *glow_tex = space_gl_effect_laser_glow_texture();
    SDL_Texture *edge_tex = space_gl_effect_laser_edge_texture();
    SDL_Texture *core_tex = space_gl_effect_laser_core_texture();

    if (glow_tex && edge_tex && core_tex) {
        int tiles = 0;

        /* Muzzle flare tapers to the steady-state beam width. */
        const float cone_length = 48.0f * thickness_scale;

        tiles += space_render_laser_tile_strip(renderer, glow_tex, origin_x, origin_y, end_x,
                                               26.0f * thickness_scale, 42.0f * thickness_scale,
                                               cone_length, color);
        tiles += space_render_laser_tile_strip(renderer, edge_tex, origin_x, origin_y, end_x,
                                               12.0f * thickness_scale, 20.0f * thickness_scale,
                                               cone_length, color);

        /* Keep the core hot, but tinted enough for the beam color to read. */
        const SDL_Color core_tint = {
            (Uint8)(255 - (255 - color.r) * 0.55f),
            (Uint8)(255 - (255 - color.g) * 0.55f),
            (Uint8)(255 - (255 - color.b) * 0.55f),
            255
        };
        tiles += space_render_laser_tile_strip(renderer, core_tex, origin_x, origin_y, end_x,
                                               2.5f * thickness_scale, 3.5f * thickness_scale,
                                               cone_length, core_tint);

        SDL_Texture *flare_tex = space_gl_effect_bolt_texture();
        if (flare_tex) {
            SDL_SetTextureColorMod(flare_tex, color.r, color.g, color.b);
            SDL_SetTextureBlendMode(flare_tex, SDL_BLENDMODE_ADD);
            const float flare_size = 28.0f * thickness_scale;
            const SDL_FRect flare_dst = {origin_x - flare_size * 0.5f, origin_y - flare_size * 0.5f, flare_size, flare_size};
            SDL_RenderCopyF(renderer, flare_tex, NULL, &flare_dst);
            SDL_SetTextureBlendMode(flare_tex, SDL_BLENDMODE_BLEND);
            tiles++;
        }

        if (metrics) {
            metrics->draw_calls += tiles;
            metrics->vertices_rendered += tiles * 4;
        }
        return;
    }

    /* Fallback: flat lines, as before GL effects existed. */
    const SDL_Color glow_color = {color.r, color.g, color.b, 180};
    const SDL_Color core_color = {255, 255, 220, 255};
    const float half = 2.5f * thickness_scale;

    SDL_SetRenderDrawColor(renderer, glow_color.r, glow_color.g, glow_color.b, glow_color.a);
    SDL_RenderDrawLineF(renderer, origin_x, origin_y - half, end_x, origin_y - half);
    SDL_RenderDrawLineF(renderer, origin_x, origin_y + half, end_x, origin_y + half);
    SDL_SetRenderDrawColor(renderer, core_color.r, core_color.g, core_color.b, core_color.a);
    SDL_RenderDrawLineF(renderer, origin_x, origin_y, end_x, origin_y);

    if (metrics) {
        metrics->draw_calls += 3;
        metrics->vertices_rendered += 6;
    }
}

void space_render_lasers(const SpaceBenchState *state,
                         SDL_Renderer *renderer,
                         BenchMetrics *metrics)
{
    if (state->player_laser.is_firing) {
        const SDL_Color player_color = {140, 220, 255, 255};
        space_render_laser_beam(renderer, metrics,
                                state->player_laser.origin_x,
                                state->player_laser.origin_y,
                                (float)SPACE_SCREEN_W,
                                player_color,
                                state->weapon_upgrades.beam_scale);
    }

    for (int i = 0; i < state->weapon_upgrades.drone_count; ++i) {
        const SpaceLaserBeam *laser = &state->drone_lasers[i];
        if (!laser->is_firing) {
            continue;
        }

        const SDL_Color drone_color = {200, 160, 255, 255};
        space_render_laser_beam(renderer, metrics,
                                laser->origin_x,
                                laser->origin_y,
                                (float)SPACE_SCREEN_W,
                                drone_color,
                                0.75f);
    }
}

void space_render_bullets(const SpaceBenchState *state,
                          SDL_Renderer *renderer,
                          BenchMetrics *metrics)
{
    SDL_Texture *bolt_texture = space_gl_effect_bolt_texture();
    if (bolt_texture) {
        const float bolt_size = 22.0f;
        const float trail_size = bolt_size * 0.55f;
        const float trail_offset = 9.0f;
        int draw_calls = 0;

        for (int i = 0; i < SPACE_MAX_BULLETS; ++i) {
            const SpaceBullet *bullet = &state->bullets[i];
            if (!bullet->active) {
                continue;
            }

            float dir_x = 0.0f;
            float dir_y = -1.0f;
            const float speed_sq = bullet->vx * bullet->vx + bullet->vy * bullet->vy;
            if (speed_sq > 0.01f) {
                const float inv_speed = 1.0f / SDL_sqrtf(speed_sq);
                dir_x = bullet->vx * inv_speed;
                dir_y = bullet->vy * inv_speed;
            }

            // Single small, faded trail segment behind the bolt.
            const float trail_x = bullet->x - dir_x * trail_offset;
            const float trail_y = bullet->y - dir_y * trail_offset;
            SDL_SetTextureColorMod(bolt_texture, 255, 255, 160);
            SDL_SetTextureAlphaMod(bolt_texture, 110);
            const SDL_FRect trail_dst = {trail_x - trail_size * 0.5f, trail_y - trail_size * 0.5f, trail_size, trail_size};
            SDL_RenderCopyF(renderer, bolt_texture, NULL, &trail_dst);

            SDL_SetTextureAlphaMod(bolt_texture, 255);
            const SDL_FRect dst = {bullet->x - bolt_size * 0.5f, bullet->y - bolt_size * 0.5f, bolt_size, bolt_size};
            SDL_RenderCopyF(renderer, bolt_texture, NULL, &dst);

            draw_calls += 2;
        }
        if (metrics && draw_calls > 0) {
            metrics->draw_calls += draw_calls;
            metrics->vertices_rendered += draw_calls * 4;
        }
        return;
    }

    /* Fallback if GL effects failed to initialise. */
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
