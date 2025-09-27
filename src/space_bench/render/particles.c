#include "space_bench/render/internal.h"

#include <math.h>

void space_render_particles(const SpaceBenchState *state,
                            SDL_Renderer *renderer,
                            BenchMetrics *metrics)
{
    SDL_BlendMode old_mode;
    SDL_GetRenderDrawBlendMode(renderer, &old_mode);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);

    for (int i = 0; i < SPACE_MAX_PARTICLES; ++i) {
        const SpaceParticle *particle = &state->particles[i];
        if (!particle->active) {
            continue;
        }

        const float life_ratio = particle->life / particle->max_life;
        const float fade = SDL_clamp(life_ratio, 0.0f, 1.0f);
        const Uint8 alpha = (Uint8)(255 * fade);
        const Uint8 r = particle->r;
        const Uint8 g = particle->g;
        const Uint8 b = particle->b;

        SDL_SetRenderDrawColor(renderer, r, g, b, alpha);
        SDL_RenderDrawPointF(renderer, particle->x, particle->y);

        if (metrics) {
            metrics->vertices_rendered += 1;
        }
    }
    if (metrics) {
        metrics->draw_calls += 1;
    }

    SDL_SetRenderDrawBlendMode(renderer, old_mode);
}

void space_render_explosions(const SpaceBenchState *state,
                             SDL_Renderer *renderer,
                             BenchMetrics *metrics)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);

    for (int i = 0; i < SPACE_MAX_EXPLOSIONS; ++i) {
        const SpaceExplosion *exp = &state->explosions[i];
        if (!exp->active) {
            continue;
        }
        const float t = SDL_clamp(exp->timer / exp->lifetime, 0.0f, 1.0f);
        const float fade = 1.0f - t;
        const float radius = exp->radius * (0.7f + 0.3f * sinf(t * (float)M_PI));
        const int segments = 20;

        SDL_SetRenderDrawColor(renderer, (Uint8)(255 * fade), (Uint8)(200 * fade), (Uint8)(120 * fade), (Uint8)(220 * fade));
        for (int s = 0; s < segments; ++s) {
            const float angle1 = (float)(M_PI * 2.0) * (float)s / (float)segments;
            const float angle2 = (float)(M_PI * 2.0) * (float)(s + 1) / (float)segments;
            const float x1 = exp->x + cosf(angle1) * radius;
            const float y1 = exp->y + sinf(angle1) * radius;
            const float x2 = exp->x + cosf(angle2) * radius;
            const float y2 = exp->y + sinf(angle2) * radius;
            SDL_RenderDrawLineF(renderer, x1, y1, x2, y2);
        }

        if (metrics) {
            metrics->draw_calls += segments;
            metrics->vertices_rendered += segments * 2;
        }
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}
