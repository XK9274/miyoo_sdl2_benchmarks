#include "space_bench/render/internal.h"

#include <math.h>

void space_render_enemies(const SpaceBenchState *state,
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

        Uint8 red = 255 - (i * 17) % 100;
        Uint8 green = 100 + (i * 31) % 155;
        Uint8 blue = 150 + (i * 23) % 105;
        SDL_SetRenderDrawColor(renderer, red, green, blue, 220);

        SDL_RenderDrawLineF(renderer, apex_pt.x, apex_pt.y, base_a_pt.x, base_a_pt.y);
        SDL_RenderDrawLineF(renderer, apex_pt.x, apex_pt.y, base_b_pt.x, base_b_pt.y);
        SDL_RenderDrawLineF(renderer, apex_pt.x, apex_pt.y, base_c_pt.x, base_c_pt.y);
        SDL_RenderDrawLineF(renderer, apex_pt.x, apex_pt.y, base_d_pt.x, base_d_pt.y);

        SDL_FPoint base_strip[5] = {base_a_pt, base_b_pt, base_c_pt, base_d_pt, base_a_pt};
        SDL_RenderDrawLinesF(renderer, base_strip, 5);

        if (metrics) {
            metrics->draw_calls += 5;  // 4 apex edges + base strip
            metrics->vertices_rendered += 16;
        }
    }
}
