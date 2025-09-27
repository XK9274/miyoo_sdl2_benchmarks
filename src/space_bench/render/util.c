#include "space_bench/render/internal.h"

#include <math.h>

SpaceVec3 space_rotate_roll(SpaceVec3 v, float roll)
{
    const float c = cosf(roll);
    const float s = sinf(roll);
    const float old_y = v.y;
    const float old_z = v.z;
    v.y = old_y * c - old_z * s;
    v.z = old_y * s + old_z * c;
    return v;
}

SDL_FPoint space_project_point(SpaceVec3 v, float origin_x, float origin_y)
{
    SDL_FPoint p = {origin_x + v.x, origin_y + v.y};
    return p;
}

void space_render_trail(const SpaceTrail *trail,
                        SDL_Renderer *renderer,
                        BenchMetrics *metrics,
                        SDL_Color primary,
                        SDL_Color secondary)
{
    (void)secondary;
    if (!trail || trail->count < 2) {
        return;
    }

    for (int i = 1; i < trail->count; ++i) {
        const SpaceTrailPoint *prev = &trail->points[i - 1];
        const SpaceTrailPoint *curr = &trail->points[i];
        const float t = (float)(trail->count - i) / (float)trail->count;

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
