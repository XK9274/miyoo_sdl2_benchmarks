#include "space_bench/render/internal.h"

void space_render_drones(const SpaceBenchState *state,
                         SDL_Renderer *renderer,
                         BenchMetrics *metrics)
{
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

        SDL_SetRenderDrawColor(renderer, 140, 220, 255, 230);

        SDL_RenderDrawLineF(renderer, apex_pt.x, apex_pt.y, base_a_pt.x, base_a_pt.y);
        SDL_RenderDrawLineF(renderer, apex_pt.x, apex_pt.y, base_b_pt.x, base_b_pt.y);
        SDL_RenderDrawLineF(renderer, apex_pt.x, apex_pt.y, base_c_pt.x, base_c_pt.y);
        SDL_RenderDrawLineF(renderer, apex_pt.x, apex_pt.y, base_d_pt.x, base_d_pt.y);

        SDL_FPoint base_strip[5] = {base_a_pt, base_b_pt, base_c_pt, base_d_pt, base_a_pt};
        SDL_RenderDrawLinesF(renderer, base_strip, 5);

        if (metrics) {
            metrics->draw_calls += 5;
            metrics->vertices_rendered += 16;
        }
    }
}
