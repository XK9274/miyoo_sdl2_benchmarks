#include <math.h>

#include "render_suite/scenes/lines.h"

#define RS_PI 3.14159265358979323846f

static inline float rs_clampf(float value, float min_val, float max_val)
{
    if (value < min_val) {
        return min_val;
    }
    if (value > max_val) {
        return max_val;
    }
    return value;
}

static inline int rs_clampi(int value, int min_val, int max_val)
{
    if (value < min_val) {
        return min_val;
    }
    if (value > max_val) {
        return max_val;
    }
    return value;
}

static inline float rs_wrap_angle(float value)
{
    const float two_pi = RS_PI * 2.0f;
    while (value < 0.0f) {
        value += two_pi;
    }
    while (value >= two_pi) {
        value -= two_pi;
    }
    return value;
}

static Uint8 rs_color_component(float n)
{
    const float v = 0.5f + 0.5f * sinf(n);
    const float clamped = (v < 0.0f) ? 0.0f : (v > 1.0f ? 1.0f : v);
    return (Uint8)(clamped * 255.0f);
}

static void rs_draw_radial_fan(SDL_Renderer *renderer,
                               float center_x,
                               float center_y,
                               float radius_min,
                               float radius_max,
                               float phase,
                               int segments,
                               BenchMetrics *metrics)
{
    int lines_drawn = 0;

    for (int i = 0; i < segments; ++i) {
        const float t = (float)i / (float)segments;
        const float angle = rs_wrap_angle(t * RS_PI * 2.0f + phase);
        const float wobble = sinf(phase * 0.6f + t * 14.0f) * 0.5f + 0.5f;
        const float radius = radius_min + wobble * (radius_max - radius_min);
        const float x1 = center_x + cosf(angle) * radius;
        const float y1 = center_y + sinf(angle) * radius;

        const float hue = phase * 0.35f + t * 6.0f;
        const Uint8 r = rs_color_component(hue);
        const Uint8 g = rs_color_component(hue + RS_PI * 0.66f);
        const Uint8 b = rs_color_component(hue + RS_PI * 1.33f);

        SDL_SetRenderDrawColor(renderer, r, g, b, 220);
        SDL_RenderDrawLineF(renderer, center_x, center_y, x1, y1);
        ++lines_drawn;
    }

    if (metrics) {
        metrics->draw_calls += lines_drawn;
        metrics->vertices_rendered += lines_drawn * 2;
    }
}

static void rs_draw_sweeps(SDL_Renderer *renderer,
                           int start_x,
                           int start_y,
                           int length,
                           int rows,
                           float amplitude,
                           float phase,
                           SDL_bool horizontal,
                           Uint8 base_r,
                           Uint8 base_g,
                           Uint8 base_b,
                           BenchMetrics *metrics)
{
    int lines_drawn = 0;

    if (rows < 1) {
        return;
    }

    for (int i = 0; i < rows; ++i) {
        const float t = (float)i / (float)(rows - 1 ? rows - 1 : 1);
        const float oscillate = sinf(phase + t * RS_PI * 2.0f);
        Uint8 r = (Uint8)rs_clampi((int)base_r + (int)(oscillate * 40.0f), 0, 255);
        Uint8 g = (Uint8)rs_clampi((int)base_g + (int)(oscillate * 40.0f), 0, 255);
        Uint8 b = (Uint8)rs_clampi((int)base_b + (int)(oscillate * 40.0f), 0, 255);

        SDL_SetRenderDrawColor(renderer, r, g, b, 200);

        if (horizontal) {
            const float base_y = (float)start_y + t * (float)length;
            const float offset = oscillate * amplitude;
            SDL_RenderDrawLineF(renderer,
                                0.0f - amplitude,
                                base_y + offset,
                                (float)BENCH_SCREEN_W + amplitude,
                                base_y - offset);
        } else {
            const float base_x = (float)start_x + t * (float)length;
            const float offset = oscillate * amplitude;
            SDL_RenderDrawLineF(renderer,
                                base_x + offset,
                                (float)start_y,
                                base_x - offset,
                                (float)start_y + (float)length);
        }

        ++lines_drawn;
    }

    if (metrics) {
        metrics->draw_calls += lines_drawn;
        metrics->vertices_rendered += lines_drawn * 2;
    }
}

void rs_scene_lines(RenderSuiteState *state,
                    SDL_Renderer *renderer,
                    BenchMetrics *metrics,
                    double delta_seconds)
{
    if (!state || !renderer) {
        return;
    }

    const float factor = rs_state_stress_factor(state);
    const float stress_scale = rs_clampf(factor, 0.5f, 4.0f);
    const int region_height = SDL_max(1, BENCH_SCREEN_H - (int)state->top_margin);
    const float center_x = BENCH_SCREEN_W * 0.5f;
    const float center_y = (float)state->top_margin + (float)region_height * 0.5f;

    /* advance animation */
    const float advance_speed = 1.2f + stress_scale * 0.6f;
    state->lines_cursor_progress += (float)(delta_seconds * advance_speed);
    if (state->lines_cursor_progress >= 1.0f) {
        int steps = (int)state->lines_cursor_progress;
        state->lines_cursor_progress -= (float)steps;
        state->lines_cursor_index += steps;
    }

    const float phase = (float)state->lines_cursor_index * 0.175f + state->lines_cursor_progress * 0.175f;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);

    /* radial burst */
    const int radial_segments = rs_clampi((int)(stress_scale * 96.0f), 32, 240);
    const float radius_min = (float)SDL_min(BENCH_SCREEN_W, region_height) * 0.18f;
    const float radius_max = (float)SDL_min(BENCH_SCREEN_W, region_height) * (0.25f + stress_scale * 0.12f);
    rs_draw_radial_fan(renderer, center_x, center_y, radius_min, radius_max, phase, radial_segments, metrics);

    /* horizontal sweeps */
    const int horizontal_rows = rs_clampi((int)(stress_scale * 40.0f), 12, 160);
    rs_draw_sweeps(renderer,
                   0,
                   (int)state->top_margin,
                   region_height,
                   horizontal_rows,
                   18.0f + stress_scale * 6.0f,
                   phase * 1.35f,
                   SDL_TRUE,
                   110,
                   180,
                   220,
                   metrics);

    /* vertical sweeps */
    const int vertical_rows = rs_clampi((int)(stress_scale * 36.0f), 10, 140);
    rs_draw_sweeps(renderer,
                   0,
                   (int)state->top_margin,
                   region_height,
                   vertical_rows,
                   16.0f + stress_scale * 5.0f,
                   phase * 0.9f + RS_PI * 0.25f,
                   SDL_FALSE,
                   200,
                   130,
                   200,
                   metrics);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}
