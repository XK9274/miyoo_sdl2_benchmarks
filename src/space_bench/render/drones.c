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
        const SDL_Color color = {140, 220, 255, 230};

        space_render_wire_pyramid(renderer, metrics, roll, origin_x, origin_y,
                                  8.0f, -4.0f, 4.0f, ship->z * 0.08f, color, NULL);
    }
}
