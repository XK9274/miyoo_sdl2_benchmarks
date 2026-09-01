#include "gfx_bench/scenes/rounded_rects.h"

#include <math.h>

#include <SDL2/SDL2_gfxPrimitives.h>

void gb_scene_rounded_rects(GfxBenchState *state,
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

    /* Rounded corners cost O(radius) unbatched draw calls each, so tile count and radius stay small. */
    int cols = SDL_clamp((int)(2.0f * factor), 2, 8);
    int rows = SDL_clamp((int)(2.0f * factor), 2, 6);

    const float cell_w = (float)region_width / (float)cols;
    const float cell_h = (float)region_height / (float)rows;
    const float pad = SDL_min(cell_w, cell_h) * 0.12f;

    int index = 0;
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col, ++index) {
            const float x1 = col * cell_w + pad;
            const float y1 = (float)start_y + row * cell_h + pad;
            const float x2 = (col + 1) * cell_w - pad;
            const float y2 = (float)start_y + (row + 1) * cell_h - pad;

            const float max_rad = SDL_min(SDL_min(x2 - x1, y2 - y1) * 0.5f, 22.0f);
            const float rad_phase = state->phase * 1.6f + (float)index * 0.45f;
            const float radius = max_rad * (0.2f + 0.55f * (0.5f + 0.5f * gb_state_sin_rad(state, rad_phase)));

            const float hue_phase = state->phase * 0.5f + (float)index * 0.5f;
            const Uint8 r = (Uint8)(110 + 110 * gb_state_sin_rad(state, hue_phase));
            const Uint8 g = (Uint8)(110 + 110 * gb_state_sin_rad(state, hue_phase + 2.0943f));
            const Uint8 b = (Uint8)(110 + 110 * gb_state_sin_rad(state, hue_phase + 4.1888f));

            if (index % 2 == 0) {
                const Uint32 fill_color = gb_pack_color(r, g, b, 255);
                roundedBoxColor(renderer, (Sint16)x1, (Sint16)y1, (Sint16)x2, (Sint16)y2,
                                (Sint16)radius, fill_color);
            } else {
                const Uint32 outline_color = gb_pack_color(r, g, b, 255);
                roundedRectangleColor(renderer, (Sint16)x1, (Sint16)y1, (Sint16)x2, (Sint16)y2,
                                      (Sint16)radius, outline_color);
            }

            if (metrics) {
                metrics->draw_calls++;
            }
        }
    }
}
