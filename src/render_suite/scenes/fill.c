#include "render_suite/scenes/fill.h"

static float rs_clamp01(float value)
{
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

static Uint8 rs_float_to_u8(float value)
{
    return (Uint8)(rs_clamp01(value) * 255.0f);
}

void rs_scene_fill(RenderSuiteState *state,
                   SDL_Renderer *renderer,
                   BenchMetrics *metrics,
                   double delta_seconds)
{
    if (!state || !renderer) {
        return;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    const float factor = rs_state_stress_factor(state);
    const int start_y = (int)state->top_margin;
    const int region_height = SDL_max(1, BENCH_SCREEN_H - start_y);

    int columns = (int)(16.0f * factor);
    if (columns < 12) {
        columns = 12;
    } else if (columns > 360) {
        columns = 360;
    }

    int passes = (int)(factor * 2.0f);
    if (passes < 1) {
        passes = 1;
    } else if (passes > 4) {
        passes = 4;
    }

    const float table_size = (float)state->sin_table_size;
    const float quarter_turn = table_size * 0.25f;

    state->fill_phase_units += (float)(delta_seconds * 40.0f);
    while (state->fill_phase_units >= table_size) {
        state->fill_phase_units -= table_size;
    }

    const float column_stride = table_size / (float)columns;

    for (int pass = 0; pass < passes; ++pass) {
        const float pass_phase = state->fill_phase_units + (float)(pass * 37);
        const float freq_units = (1.5f + 0.35f * (float)pass) * column_stride;

        for (int col = 0; col < columns; ++col) {
            const int col_x = (col * BENCH_SCREEN_W) / columns;
            const int next_x = ((col + 1) * BENCH_SCREEN_W) / columns;
            const int col_w = SDL_max(1, next_x - col_x);

            const float base_units = pass_phase + freq_units * (float)col;
            const float sweep = rs_state_sin(state, base_units) * 0.5f + 0.5f;
            const float shimmer = rs_state_sin(state, base_units + quarter_turn) * 0.5f + 0.5f;

            const float r_mix = rs_clamp01(0.30f + sweep * 0.50f + shimmer * 0.10f);
            const float g_mix = rs_clamp01(0.25f + (1.0f - sweep) * 0.45f + shimmer * 0.20f);
            const float b_mix = rs_clamp01(0.38f + sweep * 0.30f + (1.0f - shimmer) * 0.25f);

            SDL_SetRenderDrawColor(renderer,
                                   rs_float_to_u8(r_mix),
                                   rs_float_to_u8(g_mix),
                                   rs_float_to_u8(b_mix),
                                   255);

            SDL_Rect rect = {col_x, start_y, col_w, region_height};
            SDL_RenderFillRect(renderer, &rect);
            if (metrics) {
                metrics->draw_calls++;
                metrics->vertices_rendered += 4;
                metrics->triangles_rendered += 2;
            }
        }
    }
}
