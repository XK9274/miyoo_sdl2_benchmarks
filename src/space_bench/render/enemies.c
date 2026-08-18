#include "space_bench/render/internal.h"

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
        const Uint8 red = 255 - (i * 17) % 100;
        const Uint8 green = 100 + (i * 31) % 155;
        const Uint8 blue = 150 + (i * 23) % 105;
        const SDL_Color color = {red, green, blue, 220};

        space_render_wire_pyramid(renderer, metrics, roll, origin_x, origin_y,
                                  -scale * 1.2f, scale, scale, 0.0f, color, NULL);
    }
}
