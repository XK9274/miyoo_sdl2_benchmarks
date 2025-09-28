#include "render_suite/scenes/scaling.h"

#include <stdlib.h>
#include <math.h>

#define RS_PI 3.14159265358979323846f
#define MAX_SCALING_TARGETS 8

// Different resolution test modes
typedef enum {
    SCALING_MODE_LOGICAL = 0,
    SCALING_MODE_VIEWPORT,
    SCALING_MODE_TEXTURE_TARGET,
    SCALING_MODE_MAX
} ScalingMode;

// Resolution configurations to test
static const struct {
    int width;
    int height;
    const char *name;
} scaling_resolutions[] = {
    {160, 120, "160x120"},
    {240, 180, "240x180"},
    {320, 240, "320x240"},
    {480, 360, "480x360"},
    {640, 480, "640x480"}   // Native
};

static const int scaling_resolution_count = sizeof(scaling_resolutions) / sizeof(scaling_resolutions[0]);

static inline float rs_clampf(float value, float min_val, float max_val)
{
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

static inline int rs_clampi(int value, int min_val, int max_val)
{
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

static void rs_draw_test_content(const RenderSuiteState *state,
                                 SDL_Renderer *renderer,
                                 int width,
                                 int height,
                                 float phase,
                                 BenchMetrics *metrics)
{
    // Draw gradient background
    const int bar_count = 8; // Reduced from 16
    const int bar_width = width / bar_count;

    for (int i = 0; i < bar_count; i++) {
        float t = (float)i / (float)(bar_count - 1);
        float wave = rs_state_sin_rad(state, phase + t * RS_PI * 4.0f) * 0.5f + 0.5f;

        Uint8 r = (Uint8)(128 + wave * 127);
        Uint8 g = (Uint8)(64 + t * 191);
        Uint8 b = (Uint8)(192 - t * 127);

        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
        SDL_Rect rect = {i * bar_width, 0, bar_width + 1, height};
        SDL_RenderFillRect(renderer, &rect);

        if (metrics) {
            metrics->draw_calls++;
            metrics->vertices_rendered += 4;
            metrics->triangles_rendered += 2;
        }
    }

    // Draw simple moving shapes
    const int shape_count = 4; // Reduced from 8
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    for (int i = 0; i < shape_count; i++) {
        float angle = phase + (float)i * RS_PI * 2.0f / (float)shape_count;
        float radius = (float)SDL_min(width, height) * 0.2f;
        int cx = (int)((float)width * 0.5f + rs_state_cos_rad(state, angle) * radius * 0.5f);
        int cy = (int)((float)height * 0.5f + rs_state_sin_rad(state, angle) * radius * 0.5f);
        int size = (int)(8.0f + 8.0f * rs_state_sin_rad(state, phase * 2.0f + (float)i));

        // Draw simple rectangles instead of filled circles
        SDL_Rect rect = {cx - size/2, cy - size/2, size, size};
        SDL_RenderFillRect(renderer, &rect);

        if (metrics) {
            metrics->draw_calls++;
            metrics->vertices_rendered += 4;
            metrics->triangles_rendered += 2;
        }
    }
}

static void rs_test_logical_scaling(const RenderSuiteState *state,
                                    SDL_Renderer *renderer,
                                    int target_width,
                                    int target_height,
                                    float phase,
                                    BenchMetrics *metrics)
{
    Uint64 start_time = SDL_GetPerformanceCounter();

    // Set logical size
    SDL_RenderSetLogicalSize(renderer, target_width, target_height);

    // Draw test content
    rs_draw_test_content(state, renderer, target_width, target_height, phase, metrics);

    // Reset logical size
    SDL_RenderSetLogicalSize(renderer, BENCH_SCREEN_W, BENCH_SCREEN_H);

    Uint64 end_time = SDL_GetPerformanceCounter();
    if (metrics) {
        metrics->scaling_operations++;
        metrics->scaling_overhead_ms +=
            (double)(end_time - start_time) / (double)SDL_GetPerformanceFrequency() * 1000.0;
    }
}

static void rs_test_viewport_scaling(const RenderSuiteState *state,
                                     SDL_Renderer *renderer,
                                     int target_width,
                                     int target_height,
                                     float center_x,
                                     float center_y,
                                     float phase,
                                     BenchMetrics *metrics)
{
    Uint64 start_time = SDL_GetPerformanceCounter();

    // Calculate centered viewport
    SDL_Rect viewport = {
        (int)(center_x - target_width * 0.5f),
        (int)(center_y - target_height * 0.5f),
        target_width,
        target_height
    };

    // Clamp viewport to screen bounds
    if (viewport.x < 0) {
        viewport.w += viewport.x;
        viewport.x = 0;
    }
    if (viewport.y < 0) {
        viewport.h += viewport.y;
        viewport.y = 0;
    }
    if (viewport.x + viewport.w > BENCH_SCREEN_W) {
        viewport.w = BENCH_SCREEN_W - viewport.x;
    }
    if (viewport.y + viewport.h > BENCH_SCREEN_H) {
        viewport.h = BENCH_SCREEN_H - viewport.y;
    }

    SDL_RenderSetViewport(renderer, &viewport);
    rs_draw_test_content(state, renderer, viewport.w, viewport.h, phase, metrics);
    SDL_RenderSetViewport(renderer, NULL);

    Uint64 end_time = SDL_GetPerformanceCounter();
    if (metrics) {
        metrics->scaling_operations++;
        metrics->scaling_overhead_ms +=
            (double)(end_time - start_time) / (double)SDL_GetPerformanceFrequency() * 1000.0;
    }
}

static void rs_test_texture_target_scaling(SDL_Renderer *renderer, RenderSuiteState *state,
                                           int target_width, int target_height,
                                           float center_x, float center_y, float phase,
                                           BenchMetrics *metrics)
{
    if (!state->scaling_targets || state->scaling_target_count == 0) {
        return;
    }

    Uint64 start_time = SDL_GetPerformanceCounter();

    // Find appropriate render target
    int target_index = 0;
    for (int i = 0; i < state->scaling_target_count; i++) {
        int w, h;
        if (SDL_QueryTexture(state->scaling_targets[i], NULL, NULL, &w, &h) == 0) {
            if (w >= target_width && h >= target_height) {
                target_index = i;
                break;
            }
        }
    }

    SDL_Texture *target = state->scaling_targets[target_index];
    if (!target) {
        return;
    }

    // Render to texture
    SDL_SetRenderTarget(renderer, target);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    rs_draw_test_content(state, renderer, target_width, target_height, phase, metrics);

    SDL_SetRenderTarget(renderer, NULL);

    // Render texture to screen with scaling
    SDL_FRect dest_rect = {
        center_x - target_width * 0.25f,
        center_y - target_height * 0.25f,
        target_width * 0.5f,
        target_height * 0.5f
    };

    SDL_Rect src_rect = {0, 0, target_width, target_height};
    SDL_RenderCopyF(renderer, target, &src_rect, &dest_rect);

    Uint64 end_time = SDL_GetPerformanceCounter();
    if (metrics) {
        metrics->scaling_operations++;
        metrics->texture_switches++;
        metrics->scaling_overhead_ms +=
            (double)(end_time - start_time) / (double)SDL_GetPerformanceFrequency() * 1000.0;
        metrics->draw_calls++;
        metrics->vertices_rendered += 4;
        metrics->triangles_rendered += 2;
    }
}

void rs_scene_scaling_init(RenderSuiteState *state, SDL_Renderer *renderer)
{
    if (!state || !renderer) {
        return;
    }

    // Create render targets for texture scaling tests
    state->scaling_target_count = rs_clampi(scaling_resolution_count, 1, MAX_SCALING_TARGETS);
    state->scaling_targets = malloc(sizeof(SDL_Texture*) * state->scaling_target_count);

    if (state->scaling_targets) {
        for (int i = 0; i < state->scaling_target_count; i++) {
            int w = scaling_resolutions[i].width;
            int h = scaling_resolutions[i].height;

            state->scaling_targets[i] = SDL_CreateTexture(renderer,
                                                         SDL_PIXELFORMAT_RGBA8888,
                                                         SDL_TEXTUREACCESS_TARGET,
                                                         w, h);
        }
    }

    state->scaling_current_width = BENCH_SCREEN_W;
    state->scaling_current_height = BENCH_SCREEN_H;
    state->scaling_phase = 0.0f;
}

void rs_scene_scaling_cleanup(RenderSuiteState *state)
{
    if (!state) {
        return;
    }

    if (state->scaling_targets) {
        for (int i = 0; i < state->scaling_target_count; i++) {
            if (state->scaling_targets[i]) {
                SDL_DestroyTexture(state->scaling_targets[i]);
            }
        }
        free(state->scaling_targets);
        state->scaling_targets = NULL;
    }
    state->scaling_target_count = 0;
}

void rs_scene_scaling(RenderSuiteState *state,
                      SDL_Renderer *renderer,
                      BenchMetrics *metrics,
                      double delta_seconds)
{
    if (!state || !renderer) {
        return;
    }

    const float factor = rs_state_stress_factor(state);
    const int region_height = SDL_max(1, BENCH_SCREEN_H - (int)state->top_margin);
    const float center_x = BENCH_SCREEN_W * 0.5f;
    const float center_y = (float)state->top_margin + (float)region_height * 0.5f;

    // Update animation phase
    state->scaling_phase += (float)(delta_seconds * (1.0f + factor));

    // Cycle through different resolution modes
    const float cycle_speed = 0.3f + factor * 0.2f;
    const int current_mode = ((int)(state->scaling_phase * cycle_speed)) % SCALING_MODE_MAX;
    const int resolution_index = ((int)(state->scaling_phase * cycle_speed * 0.5f)) % scaling_resolution_count;

    int target_width = scaling_resolutions[resolution_index].width;
    int target_height = scaling_resolutions[resolution_index].height;

    // Apply stress factor to increase test complexity
    const int tests_per_frame = rs_clampi((int)(1 + factor * 2), 1, 3);

    for (int test = 0; test < tests_per_frame; test++) {
        float test_phase = state->scaling_phase + (float)test * 0.1f;
        int test_resolution = (resolution_index + test) % scaling_resolution_count;
        int test_width = scaling_resolutions[test_resolution].width;
        int test_height = scaling_resolutions[test_resolution].height;

        switch (current_mode) {
            case SCALING_MODE_LOGICAL:
                rs_test_logical_scaling(state, renderer, test_width, test_height, test_phase, metrics);
                break;

            case SCALING_MODE_VIEWPORT:
                rs_test_viewport_scaling(state, renderer, test_width, test_height,
                                       center_x, center_y, test_phase, metrics);
                break;

            case SCALING_MODE_TEXTURE_TARGET:
                rs_test_texture_target_scaling(renderer, state, test_width, test_height,
                                              center_x, center_y, test_phase, metrics);
                break;
        }
    }

    // Update state for overlay display
    state->scaling_current_width = target_width;
    state->scaling_current_height = target_height;

    // Draw mode indicator
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    // Simple text substitute - draw mode indicator as rectangles
    const int indicator_x = 10;
    const int indicator_y = (int)state->top_margin + 10;
    const int mode_width = 80 + current_mode * 20;

    SDL_Rect mode_rect = {indicator_x, indicator_y, mode_width, 8};
    SDL_RenderFillRect(renderer, &mode_rect);

    // Draw resolution indicator
    const int res_width = (resolution_index + 1) * 10;
    SDL_Rect res_rect = {indicator_x, indicator_y + 15, res_width, 6};
    SDL_RenderFillRect(renderer, &res_rect);

    if (metrics) {
        metrics->draw_calls += 2;
        metrics->vertices_rendered += 8;
        metrics->triangles_rendered += 4;
    }
}
