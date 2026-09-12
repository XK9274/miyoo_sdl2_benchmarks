#include "scaling_bench/render.h"
#include "common/render_neon.h"

#include <stdlib.h>
#include <math.h>
#include <string.h>

#define SCALING_BENCH_PI 3.14159265358979323846f
#define SCALING_MIN_W 160
#define SCALING_MIN_H 120
#define SCALING_NOISE_CELL 20

typedef struct {
    Uint8 r, g, b, a;
} Pixel32;

static inline float scaling_clampf(float value, float min_val, float max_val)
{
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

static inline Uint8 scaling_clamp_u8(int value)
{
    if (value < 0) return 0;
    if (value > 255) return 255;
    return (Uint8)value;
}

static const char *g_scaling_mode_names[SCALING_MODE_MAX] = {
    "Logical", "Viewport", "TextureTarget"
};

const char *scaling_render_mode_name(int mode)
{
    if (mode < 0 || mode >= SCALING_MODE_MAX) return "?";
    return g_scaling_mode_names[mode];
}

static const char *g_scaling_content_names[SCALING_CONTENT_MAX] = {
    "Gradient", "Checkerboard", "Noise"
};

const char *scaling_render_content_name(int mode)
{
    if (mode < 0 || mode >= SCALING_CONTENT_MAX) return "?";
    return g_scaling_content_names[mode];
}

static void scaling_generate_gradient(Pixel32 *pixels, int width, int height,
                                      float phase, const ScalingBenchState *state)
{
    const float cx = (float)width * 0.5f;
    const float cy = (float)height * 0.5f;
    const float max_dist = sqrtf(cx * cx + cy * cy);

    for (int y = 0; y < height; y++) {
        float fy = (float)y / (float)height;
        for (int x = 0; x < width; x++) {
            float fx = (float)x / (float)width;

            float dx = (float)x - cx;
            float dy = (float)y - cy;
            float radial = sqrtf(dx * dx + dy * dy) / max_dist;
            float linear = fx * 0.6f + fy * 0.4f;
            float t = linear * 0.5f + radial * 0.5f;

            float hue = t * 2.0f * SCALING_BENCH_PI + phase * 0.3f;
            Pixel32 *px = &pixels[y * width + x];
            px->r = scaling_clamp_u8((int)((scaling_state_sin_rad(state, hue) + 1.0f) * 127.5f));
            px->g = scaling_clamp_u8((int)((scaling_state_sin_rad(state, hue + 2.09f) + 1.0f) * 127.5f));
            px->b = scaling_clamp_u8((int)((scaling_state_sin_rad(state, hue + 4.19f) + 1.0f) * 127.5f));
            px->a = 255;
        }
    }
}

static void scaling_generate_checkerboard(Pixel32 *pixels, int width, int height,
                                          float phase, const ScalingBenchState *state)
{
    const int tile_size = 6 + (int)((scaling_state_sin_rad(state, phase * 0.2f) + 1.0f) * 5.0f);
    const int shift = (int)(phase * 6.0f);

    const Uint8 hue_a = scaling_clamp_u8((int)((scaling_state_sin_rad(state, phase * 0.5f) + 1.0f) * 127.5f));
    const Uint8 hue_b = scaling_clamp_u8((int)((scaling_state_cos_rad(state, phase * 0.5f) + 1.0f) * 127.5f));

    for (int y = 0; y < height; y++) {
        int ty = y / tile_size;
        for (int x = 0; x < width; x++) {
            int tx = (x + shift) / tile_size;
            SDL_bool parity = ((tx ^ ty) & 1) != 0;

            Pixel32 *px = &pixels[y * width + x];
            if (parity) {
                px->r = hue_a;
                px->g = 40;
                px->b = scaling_clamp_u8(255 - hue_a);
            } else {
                px->r = scaling_clamp_u8(255 - hue_b);
                px->g = 40;
                px->b = hue_b;
            }
            px->a = 255;
        }
    }
}

static inline float scaling_noise_hash(int x, int y, int seed)
{
    Uint32 h = (Uint32)(x * 1274126177 + y * 2246822519u + seed * 3266489917u);
    h = (h ^ (h >> 15)) * 2654435761u;
    h ^= (h >> 13);
    return (float)(h & 0xFFFFu) / 65535.0f;
}

#if BENCH_HAS_NEON
static inline void scaling_noise_lerp_row4(float c00, float c10, float c01, float c11,
                                           float fx0, float fx_step, float fy,
                                           float out[4])
{
    float32x4_t vfx = {fx0, fx0 + fx_step, fx0 + 2.0f * fx_step, fx0 + 3.0f * fx_step};
    float32x4_t vtop = vmlaq_f32(vdupq_n_f32(c00), vdupq_n_f32(c10 - c00), vfx);
    float32x4_t vbot = vmlaq_f32(vdupq_n_f32(c01), vdupq_n_f32(c11 - c01), vfx);
    float32x4_t vval = vmlaq_f32(vtop, vsubq_f32(vbot, vtop), vdupq_n_f32(fy));
    vst1q_f32(out, vval);
}
#endif

static void scaling_generate_noise(Pixel32 *pixels, int width, int height,
                                   float phase, SDL_bool use_neon)
{
    const int seed = (int)(phase * 0.08f);
    const float depth_shift = phase * 0.4f;

    for (int y = 0; y < height; y++) {
        int gy = y / SCALING_NOISE_CELL;
        float fy = (float)(y % SCALING_NOISE_CELL) / (float)SCALING_NOISE_CELL;

        int x = 0;
        while (x < width) {
            int gx = x / SCALING_NOISE_CELL;
            float c00 = scaling_noise_hash(gx, gy, seed);
            float c10 = scaling_noise_hash(gx + 1, gy, seed);
            float c01 = scaling_noise_hash(gx, gy + 1, seed);
            float c11 = scaling_noise_hash(gx + 1, gy + 1, seed);

            int cell_start_x = gx * SCALING_NOISE_CELL;
            int cell_end_x = SDL_min(cell_start_x + SCALING_NOISE_CELL, width);

#if BENCH_HAS_NEON
            if (use_neon) {
                int run_x = x;
                while (run_x + 4 <= cell_end_x) {
                    float fx0 = (float)(run_x - cell_start_x) / (float)SCALING_NOISE_CELL;
                    float fx_step = 1.0f / (float)SCALING_NOISE_CELL;
                    float values[4];
                    scaling_noise_lerp_row4(c00, c10, c01, c11, fx0, fx_step, fy, values);
                    for (int i = 0; i < 4; i++) {
                        float v = scaling_clampf(values[i], 0.0f, 1.0f);
                        Pixel32 *px = &pixels[y * width + run_x + i];
                        px->r = scaling_clamp_u8((int)(v * 255.0f));
                        px->g = scaling_clamp_u8((int)(v * 200.0f + depth_shift * 10.0f));
                        px->b = scaling_clamp_u8((int)((1.0f - v) * 255.0f));
                        px->a = 255;
                    }
                    run_x += 4;
                }
                x = run_x;
                if (x >= cell_end_x) {
                    continue;
                }
            }
#else
            (void)use_neon;
#endif

            for (; x < cell_end_x; x++) {
                float fx = (float)(x - cell_start_x) / (float)SCALING_NOISE_CELL;
                float top = c00 + (c10 - c00) * fx;
                float bottom = c01 + (c11 - c01) * fx;
                float v = scaling_clampf(top + (bottom - top) * fy, 0.0f, 1.0f);

                Pixel32 *px = &pixels[y * width + x];
                px->r = scaling_clamp_u8((int)(v * 255.0f));
                px->g = scaling_clamp_u8((int)(v * 200.0f + depth_shift * 10.0f));
                px->b = scaling_clamp_u8((int)((1.0f - v) * 255.0f));
                px->a = 255;
            }
        }
    }
}

static SDL_bool scaling_resize_content(ScalingBenchState *state, SDL_Renderer *renderer,
                                       int width, int height)
{
    void *new_buffer = malloc((size_t)width * (size_t)height * sizeof(Pixel32));
    if (!new_buffer) {
        return SDL_FALSE;
    }

    free(state->content_buffer);
    state->content_buffer = new_buffer;

    if (state->content_texture) {
        SDL_DestroyTexture(state->content_texture);
        state->content_texture = NULL;
    }
    if (state->target_texture) {
        SDL_DestroyTexture(state->target_texture);
        state->target_texture = NULL;
    }

    if (renderer) {
        state->content_texture = SDL_CreateTexture(renderer,
                                                    SDL_PIXELFORMAT_RGBA8888,
                                                    SDL_TEXTUREACCESS_STREAMING,
                                                    width, height);
        state->target_texture = SDL_CreateTexture(renderer,
                                                   SDL_PIXELFORMAT_RGBA8888,
                                                   SDL_TEXTUREACCESS_TARGET,
                                                   width, height);
    }

    state->scaling_current_width = width;
    state->scaling_current_height = height;
    return SDL_TRUE;
}

static void scaling_test_logical_scaling(SDL_Renderer *renderer, ScalingBenchState *state,
                                         int target_width, int target_height,
                                         BenchMetrics *metrics)
{
    Uint64 start_time = SDL_GetPerformanceCounter();

    SDL_RenderSetLogicalSize(renderer, target_width, target_height);
    SDL_Rect dest = {0, 0, target_width, target_height};
    SDL_RenderCopy(renderer, state->content_texture, NULL, &dest);
    SDL_RenderSetLogicalSize(renderer, bench_logical_w(), bench_logical_h());

    Uint64 end_time = SDL_GetPerformanceCounter();
    if (metrics) {
        metrics->scaling_operations++;
        metrics->scaling_overhead_ms +=
            (double)(end_time - start_time) / (double)SDL_GetPerformanceFrequency() * 1000.0;
        metrics->draw_calls++;
        metrics->vertices_rendered += 4;
        metrics->triangles_rendered += 2;
        metrics->texture_switches++;
    }
}

static void scaling_test_viewport_scaling(SDL_Renderer *renderer, ScalingBenchState *state,
                                          int target_width, int target_height,
                                          float center_x, float center_y,
                                          BenchMetrics *metrics)
{
    Uint64 start_time = SDL_GetPerformanceCounter();

    SDL_Rect viewport = {
        (int)(center_x - target_width * 0.5f),
        (int)(center_y - target_height * 0.5f),
        target_width,
        target_height
    };

    if (viewport.x < 0) {
        viewport.w += viewport.x;
        viewport.x = 0;
    }
    if (viewport.y < 0) {
        viewport.h += viewport.y;
        viewport.y = 0;
    }
    if (viewport.x + viewport.w > bench_logical_w()) {
        viewport.w = bench_logical_w() - viewport.x;
    }
    if (viewport.y + viewport.h > bench_logical_h()) {
        viewport.h = bench_logical_h() - viewport.y;
    }

    SDL_RenderSetViewport(renderer, &viewport);
    SDL_Rect dest = {0, 0, viewport.w, viewport.h};
    SDL_RenderCopy(renderer, state->content_texture, NULL, &dest);
    SDL_RenderSetViewport(renderer, NULL);

    Uint64 end_time = SDL_GetPerformanceCounter();
    if (metrics) {
        metrics->scaling_operations++;
        metrics->scaling_overhead_ms +=
            (double)(end_time - start_time) / (double)SDL_GetPerformanceFrequency() * 1000.0;
        metrics->draw_calls++;
        metrics->vertices_rendered += 4;
        metrics->triangles_rendered += 2;
        metrics->texture_switches++;
    }
}

static void scaling_test_texture_target_scaling(SDL_Renderer *renderer, ScalingBenchState *state,
                                                int target_width, int target_height,
                                                float center_x, float center_y,
                                                BenchMetrics *metrics)
{
    if (!state->target_texture) {
        return;
    }

    Uint64 start_time = SDL_GetPerformanceCounter();

    SDL_SetRenderTarget(renderer, state->target_texture);
    SDL_RenderCopy(renderer, state->content_texture, NULL, NULL);
    SDL_SetRenderTarget(renderer, NULL);

    SDL_FRect dest_rect = {
        center_x - (float)target_width * 0.25f,
        center_y - (float)target_height * 0.25f,
        (float)target_width * 0.5f,
        (float)target_height * 0.5f
    };

    SDL_RenderCopyF(renderer, state->target_texture, NULL, &dest_rect);

    Uint64 end_time = SDL_GetPerformanceCounter();
    if (metrics) {
        metrics->scaling_operations++;
        metrics->texture_switches += 2;
        metrics->scaling_overhead_ms +=
            (double)(end_time - start_time) / (double)SDL_GetPerformanceFrequency() * 1000.0;
        metrics->draw_calls += 2;
        metrics->vertices_rendered += 8;
        metrics->triangles_rendered += 4;
    }
}

void scaling_render_init(ScalingBenchState *state, SDL_Renderer *renderer)
{
    if (!state || !renderer) {
        return;
    }

    scaling_resize_content(state, renderer, SCALING_MIN_W, SCALING_MIN_H);
    state->scaling_phase = 0.0f;
}

void scaling_render_cleanup(ScalingBenchState *state)
{
    if (!state) {
        return;
    }

    free(state->content_buffer);
    state->content_buffer = NULL;

    if (state->content_texture) {
        SDL_DestroyTexture(state->content_texture);
        state->content_texture = NULL;
    }
    if (state->target_texture) {
        SDL_DestroyTexture(state->target_texture);
        state->target_texture = NULL;
    }
}

void scaling_render_scene(ScalingBenchState *state,
                         SDL_Renderer *renderer,
                         BenchMetrics *metrics,
                         double delta_seconds)
{
    if (!state || !renderer) {
        return;
    }

    const float factor = scaling_state_stress_factor(state);
    const int region_height = SDL_max(1, bench_logical_h() - (int)state->top_margin);
    const float center_x = bench_logical_w() * 0.5f;
    const float center_y = (float)state->top_margin + (float)region_height * 0.5f;

    state->scaling_phase += (float)(delta_seconds * (1.0f + factor));

    const float t = scaling_clampf((factor - 0.5f) / 6.5f, 0.0f, 1.0f);
    const int target_width = SCALING_MIN_W + (int)(t * (float)(bench_logical_w() - SCALING_MIN_W));
    const int target_height = SCALING_MIN_H + (int)(t * (float)(bench_logical_h() - SCALING_MIN_H));

    if (target_width != state->scaling_current_width || target_height != state->scaling_current_height) {
        scaling_resize_content(state, renderer, target_width, target_height);
    }

    const int width = state->scaling_current_width;
    const int height = state->scaling_current_height;
    if (!state->content_buffer || !state->content_texture) {
        return;
    }

    const int content_mode_duration = 300;
    const int auto_content_mode = ((int)(state->scaling_phase * 60.0f) / content_mode_duration) % SCALING_CONTENT_MAX;
    const int current_content_mode = (state->forced_content_mode >= 0) ? state->forced_content_mode : auto_content_mode;
    state->content_mode = current_content_mode;

    const int scaling_mode_duration = 240;
    const int auto_scaling_mode = ((int)(state->scaling_phase * 60.0f) / scaling_mode_duration) % SCALING_MODE_MAX;
    const int current_scaling_mode = (state->forced_scaling_mode >= 0) ? state->forced_scaling_mode : auto_scaling_mode;
    state->scaling_mode = current_scaling_mode;

    Pixel32 *pixels = (Pixel32 *)state->content_buffer;

    Uint64 gen_start = SDL_GetPerformanceCounter();
    switch (current_content_mode) {
        case SCALING_CONTENT_GRADIENT:
            scaling_generate_gradient(pixels, width, height, state->scaling_phase, state);
            break;
        case SCALING_CONTENT_CHECKERBOARD:
            scaling_generate_checkerboard(pixels, width, height, state->scaling_phase, state);
            break;
        case SCALING_CONTENT_NOISE:
            scaling_generate_noise(pixels, width, height, state->scaling_phase, state->neon_enabled);
            break;
        default:
            break;
    }
    Uint64 gen_end = SDL_GetPerformanceCounter();
    if (metrics) {
        metrics->stage_transform_ms += (double)(gen_end - gen_start) /
                                       (double)SDL_GetPerformanceFrequency() * 1000.0;
    }

    Uint64 upload_start = SDL_GetPerformanceCounter();
    const size_t pixel_count = (size_t)width * (size_t)height;
    void *tex_pixels = NULL;
    int pitch = 0;
    if (SDL_LockTexture(state->content_texture, NULL, &tex_pixels, &pitch) == 0) {
        (void)pitch;
        if (state->neon_enabled) {
            bench_neon_copy_u32((uint32_t *)tex_pixels, (uint32_t *)pixels, pixel_count);
        } else {
            memcpy(tex_pixels, pixels, pixel_count * sizeof(Pixel32));
        }
        SDL_UnlockTexture(state->content_texture);
    }
    Uint64 upload_end = SDL_GetPerformanceCounter();
    if (metrics) {
        metrics->lock_unlock_overhead_ms += (double)(upload_end - upload_start) /
                                            (double)SDL_GetPerformanceFrequency() * 1000.0;
        metrics->pixel_operations++;
    }

    switch (current_scaling_mode) {
        case SCALING_MODE_LOGICAL:
            scaling_test_logical_scaling(renderer, state, width, height, metrics);
            break;
        case SCALING_MODE_VIEWPORT:
            scaling_test_viewport_scaling(renderer, state, width, height, center_x, center_y, metrics);
            break;
        case SCALING_MODE_TEXTURE_TARGET:
            scaling_test_texture_target_scaling(renderer, state, width, height, center_x, center_y, metrics);
            break;
        default:
            break;
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    const int indicator_x = 10;
    const int indicator_y = (int)state->top_margin + 10;
    const int mode_width = 80 + current_scaling_mode * 20;

    SDL_Rect mode_rect = {indicator_x, indicator_y, mode_width, 8};
    SDL_RenderFillRect(renderer, &mode_rect);

    const int content_width_indicator = 40 + current_content_mode * 20;
    SDL_Rect content_rect = {indicator_x, indicator_y + 15, content_width_indicator, 6};
    SDL_RenderFillRect(renderer, &content_rect);

    if (metrics) {
        metrics->draw_calls += 2;
        metrics->vertices_rendered += 8;
        metrics->triangles_rendered += 4;
    }
}
