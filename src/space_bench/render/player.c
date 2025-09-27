#include "space_bench/render/internal.h"

#include <math.h>

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
}
