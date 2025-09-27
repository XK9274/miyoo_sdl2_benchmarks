#include "space_bench/render/internal.h"

#include <math.h>

void space_render_anomaly(const SpaceBenchState *state,
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
        SDL_FPoint core_pts[20];
        for (int i = 0; i < 20; ++i) {
            const float angle = anomaly->core_points[i].angle + anomaly->core_rotation;
            core_pts[i].x = anomaly->x + cosf(angle) * anomaly->core_points[i].radius;
            core_pts[i].y = anomaly->y + sinf(angle) * anomaly->core_points[i].radius;
        }
        SDL_SetRenderDrawColor(renderer, 255, 255, 100, 200);
        SDL_RenderDrawPointsF(renderer, core_pts, 20);
        draw_calls++;
        vertices += 20;
    }

    // Inner layer - lasers
    if (anomaly->inner_points[0].active) {
        SDL_FRect inner_rects[4];
        int inner_count = 0;
        for (int i = 0; i < 4; ++i) {
            if (!anomaly->inner_points[i].active) continue;
            const float angle = anomaly->inner_points[i].angle + anomaly->inner_rotation;
            const float x = anomaly->x + cosf(angle) * anomaly->inner_points[i].radius;
            const float y = anomaly->y + sinf(angle) * anomaly->inner_points[i].radius;
            inner_rects[inner_count++] = (SDL_FRect){x - 1.0f, y - 1.0f, 2.0f, 2.0f};
        }
        if (inner_count > 0) {
            SDL_SetRenderDrawColor(renderer, 100, 100, 255, 220);
            SDL_RenderFillRectsF(renderer, inner_rects, inner_count);
            draw_calls++;
            vertices += inner_count * 4;
        }
    }

    // Mid1 layer - heavy shots
    if (anomaly->mid1_points[0].active) {
        SDL_FRect mid1_rects[6];
        int mid1_count = 0;
        for (int i = 0; i < 6; ++i) {
            if (!anomaly->mid1_points[i].active) continue;
            const float angle = anomaly->mid1_points[i].angle + anomaly->mid1_rotation;
            const float x = anomaly->x + cosf(angle) * anomaly->mid1_points[i].radius;
            const float y = anomaly->y + sinf(angle) * anomaly->mid1_points[i].radius;
            mid1_rects[mid1_count++] = (SDL_FRect){x - 1.0f, y - 1.0f, 2.0f, 2.0f};
        }
        if (mid1_count > 0) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 100, 200);
            SDL_RenderFillRectsF(renderer, mid1_rects, mid1_count);
            draw_calls++;
            vertices += mid1_count * 4;
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
        SDL_FRect mid2_rects[8];
        int mid2_count = 0;
        for (int i = 0; i < 8; ++i) {
            if (!anomaly->mid2_points[i].active) continue;
            const float angle = anomaly->mid2_points[i].angle + anomaly->mid2_rotation;
            const float x = anomaly->x + cosf(angle) * anomaly->mid2_points[i].radius;
            const float y = anomaly->y + sinf(angle) * anomaly->mid2_points[i].radius;
            mid2_rects[mid2_count++] = (SDL_FRect){x - 1.0f, y - 1.0f, 2.0f, 2.0f};
        }
        if (mid2_count > 0) {
            SDL_SetRenderDrawColor(renderer, 255, 180, 100, 200);
            SDL_RenderFillRectsF(renderer, mid2_rects, mid2_count);
            draw_calls++;
            vertices += mid2_count * 4;
        }
    }

    // Outer layer - missile launchers
    if (anomaly->outer_points[0].active) {
        SDL_FRect outer_rects[10];
        int outer_count = 0;
        for (int i = 0; i < 10; ++i) {
            if (!anomaly->outer_points[i].active) continue;
            const float angle = anomaly->outer_points[i].angle + anomaly->outer_rotation;
            const float x = anomaly->x + cosf(angle) * anomaly->outer_points[i].radius;
            const float y = anomaly->y + sinf(angle) * anomaly->outer_points[i].radius;
            outer_rects[outer_count++] = (SDL_FRect){x - 1.0f, y - 1.0f, 2.0f, 2.0f};
        }
        if (outer_count > 0) {
            SDL_SetRenderDrawColor(renderer, 255, 80, 80, 220);
            SDL_RenderFillRectsF(renderer, outer_rects, outer_count);
            draw_calls++;
            vertices += outer_count * 4;
        }
    }

    // Render orbital points around anomaly center
    SDL_FPoint orbital_bins[4][64];
    int orbital_counts[4] = {0, 0, 0, 0};
    for (int i = 0; i < 64; ++i) {
        const AnomalyOrbitalPoint *point = &anomaly->orbital_points[i];
        if (!point->active) continue;
        const float x = anomaly->x + cosf(point->angle) * point->radius;
        const float y = anomaly->y + sinf(point->angle) * point->radius;
        int bucket = (int)SDL_clamp(point->radius / (anomaly->scale * 1.5f) * 3.0f, 0.0f, 3.0f);
        orbital_bins[bucket][orbital_counts[bucket]++] = (SDL_FPoint){x, y};
    }
    const SDL_Color orbital_colors[4] = {
        {100, 150, 255, 180},
        {150, 130, 220, 180},
        {200, 110, 200, 180},
        {240, 90, 180, 180},
    };
    for (int bucket = 0; bucket < 4; ++bucket) {
        if (orbital_counts[bucket] == 0) continue;
        SDL_SetRenderDrawColor(renderer,
                               orbital_colors[bucket].r,
                               orbital_colors[bucket].g,
                               orbital_colors[bucket].b,
                               orbital_colors[bucket].a);
        SDL_RenderDrawPointsF(renderer, orbital_bins[bucket], orbital_counts[bucket]);
        draw_calls++;
        vertices += orbital_counts[bucket];
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
