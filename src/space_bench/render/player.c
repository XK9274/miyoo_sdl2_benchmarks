#include "space_bench/render/internal.h"
#include "space_bench/gl_effects.h"

#include <math.h>

static void space_render_shield_fallback(SDL_Renderer *renderer,
                                         BenchMetrics *metrics,
                                         float origin_x,
                                         float origin_y,
                                         float shield_radius,
                                         float pulse_phase,
                                         float strength_frac)
{
    const float pulse = sinf(pulse_phase * 10.0f) * 0.3f + 0.7f;
    const float noise_factor = sinf(pulse_phase * 15.0f) * 2.0f;

    const int segments = 16;
    SDL_FPoint shield_points[segments + 1];
    for (int i = 0; i <= segments; ++i) {
        const float angle = (float)(M_PI * 2.0) * (float)i / (float)segments;
        const float radius_variation = shield_radius + noise_factor * sinf(angle * 3.0f);
        shield_points[i].x = origin_x + cosf(angle) * radius_variation;
        shield_points[i].y = origin_y + sinf(angle) * radius_variation;
    }

    const Uint8 shield_alpha = (Uint8)(120 * pulse * strength_frac);
    SDL_SetRenderDrawColor(renderer, 100, 255, 150, shield_alpha);
    SDL_RenderDrawLinesF(renderer, shield_points, segments + 1);

    if (metrics) {
        metrics->draw_calls++;
        metrics->vertices_rendered += segments + 1;
    }
}

void space_render_player(const SpaceBenchState *state,
                                SDL_Renderer *renderer,
                                BenchMetrics *metrics)
{
    const float origin_x = state->player_x;
    const float origin_y = state->player_y;
    const float roll = state->player_roll;

    // Render shield first if active
    if (state->shield_active && state->shield_strength > 0.0f) {
        const float shield_radius = state->player_radius + 6.0f;
        const float strength_frac = state->shield_strength / state->shield_max_strength;
        SDL_Texture *shield_texture = space_gl_effect_shield_texture();

        if (shield_texture) {
            SDL_SetTextureColorMod(shield_texture, 100, 255, 150);
            SDL_SetTextureAlphaMod(shield_texture, (Uint8)SDL_clamp(255.0f * strength_frac, 0.0f, 255.0f));
            const float size = shield_radius * 3.0f;
            const SDL_FRect dst = {origin_x - size * 0.5f, origin_y - size * 0.5f, size, size};
            SDL_RenderCopyF(renderer, shield_texture, NULL, &dst);
            if (metrics) {
                metrics->draw_calls++;
                metrics->vertices_rendered += 4;
            }
        } else {
            space_render_shield_fallback(renderer, metrics, origin_x, origin_y,
                                         shield_radius, state->shield_pulse, strength_frac);
        }
    }

    if (state->weapon_upgrades.thumper_active) {
        const float pulse_time = state->weapon_upgrades.thumper_pulse_timer;
        if (pulse_time < 0.45f) {
            const float t = SDL_clamp(pulse_time / 0.3f, 0.0f, 1.0f);
            const float ring_radius = state->player_radius + 10.0f + t * 24.0f;
            SDL_Texture *thumper_texture = space_gl_effect_thumper_texture();

            if (thumper_texture) {
                SDL_SetTextureColorMod(thumper_texture, 160, 220, 255);
                const float size = ring_radius * 2.2f;
                const SDL_FRect dst = {origin_x - size * 0.5f, origin_y - size * 0.5f, size, size};
                SDL_RenderCopyF(renderer, thumper_texture, NULL, &dst);
                if (metrics) {
                    metrics->draw_calls++;
                    metrics->vertices_rendered += 4;
                }
            } else {
                static SDL_bool lut_ready = SDL_FALSE;
                static float ring_cos[SPACE_THUMPER_SEGMENTS + 1];
                static float ring_sin[SPACE_THUMPER_SEGMENTS + 1];

                if (!lut_ready) {
                    for (int i = 0; i <= SPACE_THUMPER_SEGMENTS; ++i) {
                        const float angle = (float)(M_PI * 2.0) * (float)i / (float)SPACE_THUMPER_SEGMENTS;
                        ring_cos[i] = cosf(angle);
                        ring_sin[i] = sinf(angle);
                    }
                    lut_ready = SDL_TRUE;
                }

                const Uint8 ring_alpha = (Uint8)(170 * (1.0f - t));
                SDL_SetRenderDrawColor(renderer, 160, 220, 255, ring_alpha);

                SDL_FPoint ring_points[SPACE_THUMPER_SEGMENTS + 1];
                for (int i = 0; i <= SPACE_THUMPER_SEGMENTS; ++i) {
                    ring_points[i].x = origin_x + ring_cos[i] * ring_radius;
                    ring_points[i].y = origin_y + ring_sin[i] * ring_radius;
                }
                SDL_RenderDrawLinesF(renderer, ring_points, SPACE_THUMPER_SEGMENTS + 1);

                if (metrics) {
                    metrics->draw_calls += 1;
                    metrics->vertices_rendered += SPACE_THUMPER_SEGMENTS + 1;
                }
            }
        }
    }

    // Wireframe pyramid geometry
    const SpaceVec3 local_apex = {14.0f, 0.0f, 0.0f};
    const SpaceVec3 local_base_a = {-8.0f, -8.0f, -8.0f};
    const SpaceVec3 local_base_b = {-8.0f, 8.0f, -8.0f};
    const SpaceVec3 local_base_c = {-8.0f, 8.0f, 8.0f};
    const SpaceVec3 local_base_d = {-8.0f, -8.0f, 8.0f};

    const float sin_roll = sinf(roll);
    const float cos_roll = cosf(roll);

    SpaceVec3 apex = space_apply_roll_cached(local_apex, sin_roll, cos_roll);
    SpaceVec3 base_a = space_apply_roll_cached(local_base_a, sin_roll, cos_roll);
    SpaceVec3 base_b = space_apply_roll_cached(local_base_b, sin_roll, cos_roll);
    SpaceVec3 base_c = space_apply_roll_cached(local_base_c, sin_roll, cos_roll);
    SpaceVec3 base_d = space_apply_roll_cached(local_base_d, sin_roll, cos_roll);

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
    SDL_FPoint base_strip[5] = {base_a_pt, base_b_pt, base_c_pt, base_d_pt, base_a_pt};
    SDL_RenderDrawLinesF(renderer, base_strip, 5);

    if (metrics) {
        metrics->draw_calls += 5;
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

    // Stacked HP/shield bars above the ship
    const float bar_width = 40.0f;
    const float bar_height = 5.0f;
    const float bar_gap = 2.0f;
    const float hp_bar_y = origin_y - state->player_radius - 14.0f;

    SDL_FRect hp_bg = {origin_x - bar_width * 0.5f, hp_bar_y, bar_width, bar_height};
    SDL_SetRenderDrawColor(renderer, 40, 10, 10, 200);
    SDL_RenderFillRectF(renderer, &hp_bg);
    if (state->player_max_health > 0.0f) {
        const float hp_frac = SDL_clamp(state->player_health / state->player_max_health, 0.0f, 1.0f);
        SDL_FRect hp_fill = {hp_bg.x, hp_bg.y, bar_width * hp_frac, bar_height};
        SDL_SetRenderDrawColor(renderer, 220, 60, 60, 230);
        SDL_RenderFillRectF(renderer, &hp_fill);
    }
    if (metrics) {
        metrics->draw_calls += 2;
    }

    if (state->shield_max_strength > 0.0f) {
        SDL_FRect shield_bg = {origin_x - bar_width * 0.5f, hp_bar_y - bar_height - bar_gap, bar_width, bar_height};
        SDL_SetRenderDrawColor(renderer, 10, 20, 40, 200);
        SDL_RenderFillRectF(renderer, &shield_bg);
        const float shield_frac = SDL_clamp(state->shield_strength / state->shield_max_strength, 0.0f, 1.0f);
        SDL_FRect shield_fill = {shield_bg.x, shield_bg.y, bar_width * shield_frac, bar_height};
        SDL_SetRenderDrawColor(renderer, 80, 180, 255, 230);
        SDL_RenderFillRectF(renderer, &shield_fill);
        if (metrics) {
            metrics->draw_calls += 2;
        }
    }
}
