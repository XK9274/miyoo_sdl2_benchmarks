#include "fill_bench/render.h"

#include <math.h>

#define FILL_MAX_RECTS 512

static float fill_clamp01(float value)
{
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

static Uint8 fill_float_to_u8(float value)
{
    return (Uint8)(fill_clamp01(value) * 255.0f);
}

static const char *g_fill_pattern_names[FILL_PATTERN_MAX] = {
    "Columns", "Rows", "Checker", "Rings"
};

const char *fill_render_pattern_name(int mode)
{
    if (mode < 0 || mode >= FILL_PATTERN_MAX) return "?";
    return g_fill_pattern_names[mode];
}

static const char *g_fill_draw_names[FILL_DRAW_MAX] = {
    "PerRect", "Batched"
};

const char *fill_render_draw_name(int mode)
{
    if (mode < 0 || mode >= FILL_DRAW_MAX) return "?";
    return g_fill_draw_names[mode];
}

static int fill_build_columns_rects(SDL_Rect *rects, int max_rects, int density,
                                    int start_y, int region_height, int width)
{
    const int count = SDL_min(density, max_rects);
    for (int col = 0; col < count; col++) {
        const int col_x = (col * width) / count;
        const int next_x = ((col + 1) * width) / count;
        const int col_w = SDL_max(1, next_x - col_x);
        SDL_Rect rect = {col_x, start_y, col_w, region_height};
        rects[col] = rect;
    }
    return count;
}

static int fill_build_rows_rects(SDL_Rect *rects, int max_rects, int density,
                                 int start_y, int region_height, int width)
{
    const int count = SDL_min(density, max_rects);
    for (int row = 0; row < count; row++) {
        const int row_y = start_y + (row * region_height) / count;
        const int next_y = start_y + ((row + 1) * region_height) / count;
        const int row_h = SDL_max(1, next_y - row_y);
        SDL_Rect rect = {0, row_y, width, row_h};
        rects[row] = rect;
    }
    return count;
}

static int fill_build_checker_rects(SDL_Rect *rects, int max_rects, int density,
                                    int start_y, int region_height, int width)
{
    const int grid = SDL_max(2, (int)sqrtf((float)density));
    const int block_w = SDL_max(1, width / grid);
    const int block_h = SDL_max(1, region_height / grid);

    int count = 0;
    for (int gy = 0; gy < grid && count < max_rects; gy++) {
        for (int gx = 0; gx < grid && count < max_rects; gx++) {
            if (((gx ^ gy) & 1) == 0) {
                continue;
            }
            SDL_Rect rect = {gx * block_w, start_y + gy * block_h, block_w, block_h};
            rects[count++] = rect;
        }
    }
    return count;
}

static int fill_build_ring_rects(SDL_Rect *rects, int max_rects, int density,
                                 int start_y, int region_height, int width)
{
    const int count = SDL_min(SDL_min(density, 24), max_rects);
    const int cx = width / 2;
    const int cy = start_y + region_height / 2;
    const int max_extent_w = width / 2;
    const int max_extent_h = region_height / 2;

    for (int i = 0; i < count; i++) {
        const float t = (float)i / (float)count;
        const int half_w = SDL_max(2, (int)((float)max_extent_w * (1.0f - t)));
        const int half_h = SDL_max(2, (int)((float)max_extent_h * (1.0f - t)));
        SDL_Rect rect = {cx - half_w, cy - half_h, half_w * 2, half_h * 2};
        rects[i] = rect;
    }
    return count;
}

void fill_render_scene(FillBenchState *state,
                       SDL_Renderer *renderer,
                       BenchMetrics *metrics,
                       double delta_seconds)
{
    if (!state || !renderer) {
        return;
    }

    const float factor = fill_state_stress_factor(state);
    const int start_y = (int)state->top_margin;
    const int region_height = SDL_max(1, bench_logical_h() - start_y);
    const int width = bench_logical_w();

    int density = (int)(16.0f * factor);
    if (density < 12) {
        density = 12;
    } else if (density > 360) {
        density = 360;
    }

    int passes = (int)(factor * 2.0f);
    if (passes < 1) {
        passes = 1;
    } else if (passes > 4) {
        passes = 4;
    }

    const float table_size = (float)state->sin_table.sin_table_size;
    const float quarter_turn = table_size * 0.25f;

    state->fill_phase_units += (float)(delta_seconds * 40.0f);
    while (state->fill_phase_units >= table_size) {
        state->fill_phase_units -= table_size;
    }

    state->mode_phase_seconds += (float)delta_seconds;

    const float mode_cycle_seconds = 3.0f;
    const int auto_pattern_mode = (int)(state->mode_phase_seconds / mode_cycle_seconds) % FILL_PATTERN_MAX;
    state->current_pattern_mode = (state->forced_pattern_mode >= 0) ? state->forced_pattern_mode : auto_pattern_mode;

    const int auto_draw_mode = (int)((state->mode_phase_seconds + mode_cycle_seconds * 0.5f) / mode_cycle_seconds) % FILL_DRAW_MAX;
    state->current_draw_mode = (state->forced_draw_mode >= 0) ? state->forced_draw_mode : auto_draw_mode;

    SDL_SetRenderDrawBlendMode(renderer, state->blend_enabled ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE);

    SDL_Rect rects[FILL_MAX_RECTS];
    int rect_count = 0;
    switch (state->current_pattern_mode) {
        case FILL_PATTERN_COLUMNS:
            rect_count = fill_build_columns_rects(rects, FILL_MAX_RECTS, density, start_y, region_height, width);
            break;
        case FILL_PATTERN_ROWS:
            rect_count = fill_build_rows_rects(rects, FILL_MAX_RECTS, density, start_y, region_height, width);
            break;
        case FILL_PATTERN_CHECKER_BLOCKS:
            rect_count = fill_build_checker_rects(rects, FILL_MAX_RECTS, density, start_y, region_height, width);
            break;
        case FILL_PATTERN_RINGS:
            rect_count = fill_build_ring_rects(rects, FILL_MAX_RECTS, density, start_y, region_height, width);
            break;
        default:
            break;
    }
    if (rect_count <= 0) {
        return;
    }

    const float rect_stride = table_size / (float)rect_count;

    Uint64 fill_start = SDL_GetPerformanceCounter();

    for (int pass = 0; pass < passes; ++pass) {
        const float pass_phase = state->fill_phase_units + (float)(pass * 37);
        const float freq_units = (1.5f + 0.35f * (float)pass) * rect_stride;

        if (state->current_draw_mode == FILL_DRAW_BATCHED) {
            const float sweep = fill_state_sin(state, pass_phase) * 0.5f + 0.5f;
            const float shimmer = fill_state_sin(state, pass_phase + quarter_turn) * 0.5f + 0.5f;

            const float r_mix = fill_clamp01(0.30f + sweep * 0.50f + shimmer * 0.10f);
            const float g_mix = fill_clamp01(0.25f + (1.0f - sweep) * 0.45f + shimmer * 0.20f);
            const float b_mix = fill_clamp01(0.38f + sweep * 0.30f + (1.0f - shimmer) * 0.25f);

            SDL_SetRenderDrawColor(renderer,
                                   fill_float_to_u8(r_mix),
                                   fill_float_to_u8(g_mix),
                                   fill_float_to_u8(b_mix),
                                   255);
            SDL_RenderFillRects(renderer, rects, rect_count);

            if (metrics) {
                metrics->draw_calls++;
                metrics->vertices_rendered += (Uint64)rect_count * 4;
                metrics->triangles_rendered += (Uint64)rect_count * 2;
            }
        } else {
            for (int i = 0; i < rect_count; ++i) {
                const float base_units = pass_phase + freq_units * (float)i;
                const float sweep = fill_state_sin(state, base_units) * 0.5f + 0.5f;
                const float shimmer = fill_state_sin(state, base_units + quarter_turn) * 0.5f + 0.5f;

                const float r_mix = fill_clamp01(0.30f + sweep * 0.50f + shimmer * 0.10f);
                const float g_mix = fill_clamp01(0.25f + (1.0f - sweep) * 0.45f + shimmer * 0.20f);
                const float b_mix = fill_clamp01(0.38f + sweep * 0.30f + (1.0f - shimmer) * 0.25f);

                SDL_SetRenderDrawColor(renderer,
                                       fill_float_to_u8(r_mix),
                                       fill_float_to_u8(g_mix),
                                       fill_float_to_u8(b_mix),
                                       255);
                SDL_RenderFillRect(renderer, &rects[i]);

                if (metrics) {
                    metrics->draw_calls++;
                    metrics->vertices_rendered += 4;
                    metrics->triangles_rendered += 2;
                }
            }
        }
    }

    Uint64 fill_end = SDL_GetPerformanceCounter();
    if (metrics) {
        metrics->stage_draw_ms += (double)(fill_end - fill_start) /
                                  (double)SDL_GetPerformanceFrequency() * 1000.0;
    }
}
