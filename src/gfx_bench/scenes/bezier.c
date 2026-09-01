#include "gfx_bench/scenes/bezier.h"

#include <SDL2/SDL2_gfxPrimitives.h>

#define GB_BEZIER_MAX_CURVES 8
#define GB_BEZIER_CONTROL_POINTS 4

void gb_scene_bezier(GfxBenchState *state,
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

    /* bezierColor's `segments` is a 1:1 multiplier of unbatched draw calls per curve. */
    const int curve_count = SDL_clamp((int)(1.0f * factor), 1, GB_BEZIER_MAX_CURVES);
    const int segments = SDL_clamp((int)(6.0f + 1.5f * factor), 6, 16);

    const float row_h = (float)region_height / (float)curve_count;
    const float orbit_radius = row_h * 0.4f;

    Sint16 vx[GB_BEZIER_CONTROL_POINTS];
    Sint16 vy[GB_BEZIER_CONTROL_POINTS];

    for (int i = 0; i < curve_count; ++i) {
        const float baseline_y = (float)start_y + (i + 0.5f) * row_h;
        const float curve_phase = state->phase * (1.1f + 0.1f * (float)i) + (float)i * 1.3f;

        /* Endpoints anchored at the row edges; control points orbit the row's vertical center. */
        vx[0] = (Sint16)(region_width * 0.05f);
        vy[0] = (Sint16)baseline_y;
        vx[1] = (Sint16)(region_width * 0.35f);
        vy[1] = (Sint16)(baseline_y + orbit_radius * gb_state_sin_rad(state, curve_phase));
        vx[2] = (Sint16)(region_width * 0.65f);
        vy[2] = (Sint16)(baseline_y + orbit_radius * gb_state_cos_rad(state, curve_phase * 1.3f));
        vx[3] = (Sint16)(region_width * 0.95f);
        vy[3] = (Sint16)baseline_y;

        const float hue_phase = state->phase * 0.5f + (float)i * 0.6f;
        const Uint8 r = (Uint8)(130 + 100 * gb_state_sin_rad(state, hue_phase));
        const Uint8 g = (Uint8)(130 + 100 * gb_state_sin_rad(state, hue_phase + 2.0943f));
        const Uint8 b = (Uint8)(130 + 100 * gb_state_sin_rad(state, hue_phase + 4.1888f));

        bezierColor(renderer, vx, vy, GB_BEZIER_CONTROL_POINTS, segments,
                   gb_pack_color(r, g, b, 255));

        if (metrics) {
            metrics->draw_calls++;
        }
    }
}
