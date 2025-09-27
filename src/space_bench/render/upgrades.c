#include "space_bench/render/internal.h"

#include <math.h>

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

void space_render_upgrades(const SpaceBenchState *state,
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

        const SDL_Color color = space_upgrade_color(upgrade->type);
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

        for (int s = 0; s < 12; ++s) {
            const float angle = (float)(M_PI * 2.0) * (float)s / 12.0f;
            const float next_angle = (float)(M_PI * 2.0) * (float)(s + 1) / 12.0f;
            const float radius = 7.0f + sinf(angle * 2.0f + state->time_accumulator * 4.0f) * 2.0f;
            const float x1 = upgrade->x + cosf(angle) * radius;
            const float y1 = upgrade->y + sinf(angle) * radius;
            const float x2 = upgrade->x + cosf(next_angle) * radius;
            const float y2 = upgrade->y + sinf(next_angle) * radius;
            SDL_RenderDrawLineF(renderer, x1, y1, x2, y2);
        }

        SDL_FRect core = {upgrade->x - 2.0f, upgrade->y - 2.0f, 4.0f, 4.0f};
        SDL_RenderFillRectF(renderer, &core);

        if (metrics) {
            metrics->draw_calls += 13;
            metrics->vertices_rendered += 30;
        }
    }
}
