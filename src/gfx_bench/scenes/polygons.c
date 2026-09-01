#include "gfx_bench/scenes/polygons.h"

#include <math.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#define GB_POLY_PI 3.14159265358979323846f
#define GB_POLY_MAX_VERTS 14

void gb_scene_polygons(GfxBenchState *state,
                       SDL_Renderer *renderer,
                       BenchMetrics *metrics,
                       double delta_seconds)
{
    if (!state || !renderer) {
        return;
    }
    (void)delta_seconds;

    const float factor = gb_state_stress_factor(state);
    const int start_y = 0;
    const int region_height = SDL_max(1, bench_logical_h() - start_y);
    const int region_width = bench_logical_w();

    /* filledPolygonColor/aapolygonColor are both O(radius)/O(perimeter) unbatched draw calls. */
    const int instance_count = SDL_clamp((int)(2.0f * factor), 2, 10);
    const int sides = SDL_clamp((int)(3.0f + factor * 0.6f), 3, 10);

    int cols = (int)ceilf(sqrtf((float)instance_count));
    cols = SDL_max(1, cols);
    int rows = (instance_count + cols - 1) / cols;
    rows = SDL_max(1, rows);

    const float cell_w = (float)region_width / (float)cols;
    const float cell_h = (float)region_height / (float)rows;
    const float radius = SDL_min(SDL_min(cell_w, cell_h) * 0.35f, 30.0f);

    Sint16 vx[GB_POLY_MAX_VERTS];
    Sint16 vy[GB_POLY_MAX_VERTS];

    int drawn = 0;
    for (int row = 0; row < rows && drawn < instance_count; ++row) {
        for (int col = 0; col < cols && drawn < instance_count; ++col, ++drawn) {
            const float cx = (col + 0.5f) * cell_w;
            const float cy = (float)start_y + (row + 0.5f) * cell_h;
            const float spin = state->phase * (1.0f + 0.15f * (float)drawn) + (float)drawn * 0.9f;

            for (int v = 0; v < sides; ++v) {
                const float angle = spin + (2.0f * GB_POLY_PI * (float)v) / (float)sides;
                vx[v] = (Sint16)(cx + radius * gb_state_cos_rad(state, angle));
                vy[v] = (Sint16)(cy + radius * gb_state_sin_rad(state, angle));
            }

            const float hue_phase = state->phase * 0.6f + (float)drawn * 0.4f;
            const Uint8 r = (Uint8)(120 + 100 * gb_state_sin_rad(state, hue_phase));
            const Uint8 g = (Uint8)(120 + 100 * gb_state_sin_rad(state, hue_phase + 2.0943f));
            const Uint8 b = (Uint8)(120 + 100 * gb_state_sin_rad(state, hue_phase + 4.1888f));

            if (drawn % 2 == 0) {
                filledPolygonColor(renderer, vx, vy, sides, gb_pack_color(r, g, b, 255));
            } else {
                aapolygonColor(renderer, vx, vy, sides, gb_pack_color(r, g, b, 255));
            }

            if (metrics) {
                metrics->draw_calls++;
            }
        }
    }
}
