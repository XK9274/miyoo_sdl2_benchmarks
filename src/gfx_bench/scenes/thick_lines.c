#include "gfx_bench/scenes/thick_lines.h"

#define GB_THICK_PI 3.14159265358979323846f
#define GB_THICK_MAX_LINES 24

#include <SDL2/SDL2_gfxPrimitives.h>

void gb_scene_thick_lines(GfxBenchState *state,
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
    const float center_x = (float)region_width * 0.5f;
    const float center_y = (float)start_y + (float)region_height * 0.5f;
    const float outer_radius = SDL_min((float)region_width, (float)region_height) * 0.42f;

    /* line_count and width both directly multiply thickLineColor's unbatched draw calls. */
    const int line_count = SDL_clamp((int)(4.0f * factor), 4, GB_THICK_MAX_LINES);
    const Uint8 width = (Uint8)SDL_clamp((int)(2.0f + factor * 0.4f), 2, 5);

    const float spin = state->phase * 0.6f;

    for (int i = 0; i < line_count; ++i) {
        const float angle = spin + (2.0f * GB_THICK_PI * (float)i) / (float)line_count;
        const float length_pulse = 0.6f + 0.4f * gb_state_sin_rad(state, angle * 3.0f + state->phase * 2.0f);
        const float radius = outer_radius * length_pulse;

        const float x2 = center_x + radius * gb_state_cos_rad(state, angle);
        const float y2 = center_y + radius * gb_state_sin_rad(state, angle);

        const float hue_phase = state->phase * 0.8f + (float)i * 0.3f;
        const Uint8 r = (Uint8)(140 + 100 * gb_state_sin_rad(state, hue_phase));
        const Uint8 g = (Uint8)(140 + 100 * gb_state_sin_rad(state, hue_phase + 2.0943f));
        const Uint8 b = (Uint8)(140 + 100 * gb_state_sin_rad(state, hue_phase + 4.1888f));

        thickLineColor(renderer, (Sint16)center_x, (Sint16)center_y, (Sint16)x2, (Sint16)y2,
                       width, gb_pack_color(r, g, b, 255));

        if (metrics) {
            metrics->draw_calls++;
        }
    }
}
