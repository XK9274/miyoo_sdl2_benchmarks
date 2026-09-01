#include "gfx_bench/scenes/aa_shapes.h"

#include <math.h>

#include <SDL2/SDL2_gfxPrimitives.h>

void gb_scene_aa_shapes(GfxBenchState *state,
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

    /* aacircleColor/aaellipseColor plot ~8*radius unbatched draw calls each, so both stay small. */
    const int instance_count = SDL_clamp((int)(4.0f * factor), 4, 24);

    /* Roughly square grid sized to fill the play area. */
    int cols = (int)ceilf(sqrtf((float)instance_count * (float)region_width / (float)region_height));
    cols = SDL_max(1, cols);
    int rows = (instance_count + cols - 1) / cols;
    rows = SDL_max(1, rows);

    const float cell_w = (float)region_width / (float)cols;
    const float cell_h = (float)region_height / (float)rows;
    const float base_radius = SDL_min(SDL_min(cell_w, cell_h) * 0.32f, 34.0f);

    int drawn = 0;
    for (int row = 0; row < rows && drawn < instance_count; ++row) {
        for (int col = 0; col < cols && drawn < instance_count; ++col, ++drawn) {
            const float cx = (col + 0.5f) * cell_w;
            const float cy = (float)start_y + (row + 0.5f) * cell_h;

            const float wobble_phase = state->phase * 2.2f + (float)drawn * 0.6f;
            const float pulse = 0.75f + 0.25f * gb_state_sin_rad(state, wobble_phase);
            const float radius = base_radius * pulse;

            const float hue_phase = state->phase * 0.7f + (float)drawn * 0.35f;
            const Uint8 r = (Uint8)(128 + 127 * gb_state_sin_rad(state, hue_phase));
            const Uint8 g = (Uint8)(128 + 127 * gb_state_sin_rad(state, hue_phase + 2.0943f));
            const Uint8 b = (Uint8)(128 + 127 * gb_state_sin_rad(state, hue_phase + 4.1888f));
            const Uint32 color = gb_pack_color(r, g, b, 255);

            /* Alternate AA/plain variants to halve the average per-instance draw cost. */
            switch (drawn % 4) {
                case 0:
                    aacircleColor(renderer, (Sint16)cx, (Sint16)cy, (Sint16)radius, color);
                    break;
                case 1:
                    circleColor(renderer, (Sint16)cx, (Sint16)cy, (Sint16)radius, color);
                    break;
                case 2: {
                    const float squash = 0.6f + 0.4f * gb_state_cos_rad(state, wobble_phase * 0.5f);
                    aaellipseColor(renderer, (Sint16)cx, (Sint16)cy,
                                   (Sint16)radius, (Sint16)(radius * squash), color);
                    break;
                }
                default: {
                    const float squash = 0.6f + 0.4f * gb_state_cos_rad(state, wobble_phase * 0.5f);
                    ellipseColor(renderer, (Sint16)cx, (Sint16)cy,
                                 (Sint16)radius, (Sint16)(radius * squash), color);
                    break;
                }
            }

            if (metrics) {
                metrics->draw_calls++;
            }
        }
    }
}
