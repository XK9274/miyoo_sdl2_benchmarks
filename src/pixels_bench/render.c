#include "pixels_bench/render.h"
#include "common/render_neon.h"

#include <stdlib.h>
#include <math.h>
#include <string.h>

#define PIXELS_BASE_W 160
#define PIXELS_BASE_H 120
#define PIXELS_MAX_W 320
#define PIXELS_MAX_H 240
#define PIXELS_NOISE_CELL 20
#define PIXELS_BENCH_PI 3.14159265358979323846f

typedef struct {
    Uint8 r, g, b, a;
} Pixel32;

static inline float pixels_clampf(float value, float min_val, float max_val)
{
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

static inline Uint8 pixels_clamp_u8(int value)
{
    if (value < 0) return 0;
    if (value > 255) return 255;
    return (Uint8)value;
}

static const char *g_pixels_mode_names[PIXEL_MODE_MAX] = {
    "Plasma", "Mandelbrot", "Cellular", "Noise", "Dither", "Palette"
};

const char *pixels_render_mode_name(int mode)
{
    if (mode < 0 || mode >= PIXEL_MODE_MAX) return "?";
    return g_pixels_mode_names[mode];
}

static const char *g_pixels_blend_names[PIXELS_BLEND_MAX] = {
    "Alpha", "Additive", "ColorKey"
};

const char *pixels_render_blend_name(PixelsBlendMode mode)
{
    if (mode < 0 || mode >= PIXELS_BLEND_MAX) return "?";
    return g_pixels_blend_names[mode];
}

static const char *g_pixels_upload_names[PIXELS_UPLOAD_MAX] = {
    "Lock", "Update"
};

const char *pixels_render_upload_name(PixelsUploadPath path)
{
    if (path < 0 || path >= PIXELS_UPLOAD_MAX) return "?";
    return g_pixels_upload_names[path];
}

/* 4x4 ordered dither threshold matrix, scaled to 0-15. */
static const Uint8 g_bayer4x4[4][4] = {
    { 0,  8,  2, 10},
    {12,  4, 14,  6},
    { 3, 11,  1,  9},
    {15,  7, 13,  5}
};

static void pixels_generate_plasma(Pixel32 *pixels, int width, int height,
                                   float phase, const PixelsBenchState *state)
{
    const float scale = 0.02f;
    const float time_scale = 0.1f;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float fx = (float)x * scale;
            float fy = (float)y * scale;

            float v1 = pixels_state_sin_rad(state, fx * 4.0f + phase * time_scale);
            float v2 = pixels_state_sin_rad(state, fy * 3.0f + phase * time_scale * 1.3f);
            float v3 = pixels_state_sin_rad(state, (fx + fy) * 2.0f + phase * time_scale * 0.7f);
            float v4 = pixels_state_sin_rad(state, sqrtf(fx * fx + fy * fy) * 5.0f + phase * time_scale * 1.5f);

            float intensity = (v1 + v2 + v3 + v4) * 0.25f;
            intensity = (intensity + 1.0f) * 0.5f;

            float r_phase = intensity * 2.0f * PIXELS_BENCH_PI;
            float g_phase = intensity * 2.0f * PIXELS_BENCH_PI + PIXELS_BENCH_PI * 0.66f;
            float b_phase = intensity * 2.0f * PIXELS_BENCH_PI + PIXELS_BENCH_PI * 1.33f;

            pixels[y * width + x].r = pixels_clamp_u8((int)((pixels_state_sin_rad(state, r_phase) + 1.0f) * 127.5f));
            pixels[y * width + x].g = pixels_clamp_u8((int)((pixels_state_sin_rad(state, g_phase) + 1.0f) * 127.5f));
            pixels[y * width + x].b = pixels_clamp_u8((int)((pixels_state_sin_rad(state, b_phase) + 1.0f) * 127.5f));
            pixels[y * width + x].a = 255;
        }
    }
}

static void pixels_generate_mandelbrot(Pixel32 *pixels, int width, int height,
                                       float phase, const PixelsBenchState *state)
{
    const float zoom = 1.0f + phase * 0.05f;
    const float center_x = -0.5f + pixels_state_cos_rad(state, phase * 0.3f) * 0.2f;
    const float center_y = 0.0f + pixels_state_sin_rad(state, phase * 0.2f) * 0.2f;
    const int max_iterations = 16;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float real = center_x + ((float)x / (float)width - 0.5f) * 4.0f / zoom;
            float imag = center_y + ((float)y / (float)height - 0.5f) * 4.0f / zoom;

            float zr = 0.0f, zi = 0.0f;
            int iterations = 0;

            while (iterations < max_iterations && (zr * zr + zi * zi) < 4.0f) {
                float temp = zr * zr - zi * zi + real;
                zi = 2.0f * zr * zi + imag;
                zr = temp;
                iterations++;
            }

            if (iterations == max_iterations) {
                pixels[y * width + x] = (Pixel32){0, 0, 0, 255};
            } else {
                float t = (float)iterations / (float)max_iterations;
                float hue = t * 6.0f + phase * 0.5f;

                int hi = (int)hue % 6;
                float f = hue - (float)hi;
                float sat = 1.0f;
                float val = t;

                float p = val * (1.0f - sat);
                float q = val * (1.0f - sat * f);
                float r = val * (1.0f - sat * (1.0f - f));

                float rf, gf, bf;
                switch (hi) {
                    case 0: rf = val; gf = r; bf = p; break;
                    case 1: rf = q; gf = val; bf = p; break;
                    case 2: rf = p; gf = val; bf = r; break;
                    case 3: rf = p; gf = q; bf = val; break;
                    case 4: rf = r; gf = p; bf = val; break;
                    default: rf = val; gf = p; bf = q; break;
                }

                pixels[y * width + x].r = pixels_clamp_u8((int)(rf * 255));
                pixels[y * width + x].g = pixels_clamp_u8((int)(gf * 255));
                pixels[y * width + x].b = pixels_clamp_u8((int)(bf * 255));
                pixels[y * width + x].a = 255;
            }
        }
    }
}

static void pixels_generate_cellular(Pixel32 *pixels, int width, int height,
                                     float phase, PixelsBenchState *state)
{
    Uint8 *cells = state->cellular_cells;
    Uint8 *new_cells = state->cellular_new_cells;

    if (!state->cellular_seeded) {
        for (int i = 0; i < width * height; i++) {
            cells[i] = (rand() % 100) < 30 ? 1 : 0;
        }
        state->cellular_seeded = SDL_TRUE;
    }

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int neighbors = 0;

            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    if (dx == 0 && dy == 0) continue;

                    int nx = (x + dx + width) % width;
                    int ny = (y + dy + height) % height;

                    if (cells[ny * width + nx]) neighbors++;
                }
            }

            int current = cells[y * width + x];

            if (current) {
                new_cells[y * width + x] = (neighbors == 2 || neighbors == 3) ? 1 : 0;
            } else {
                new_cells[y * width + x] = (neighbors == 3) ? 1 : 0;
            }
        }
    }

    if ((int)(phase * 10.0f) % 60 == 0) {
        for (int i = 0; i < 10; i++) {
            int x = rand() % width;
            int y = rand() % height;
            new_cells[y * width + x] = 1;
        }
    }

    state->cellular_cells = new_cells;
    state->cellular_new_cells = cells;
    cells = state->cellular_cells;

    float color_phase = phase * 0.5f;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (cells[y * width + x]) {
                float fx = (float)x / (float)width;
                float fy = (float)y / (float)height;

                pixels[y * width + x].r = pixels_clamp_u8((int)((pixels_state_sin_rad(state, color_phase + fx) + 1.0f) * 127.5f));
                pixels[y * width + x].g = pixels_clamp_u8((int)((pixels_state_sin_rad(state, color_phase + fy + 2.0f) + 1.0f) * 127.5f));
                pixels[y * width + x].b = pixels_clamp_u8((int)((pixels_state_sin_rad(state, color_phase + fx + fy + 4.0f) + 1.0f) * 127.5f));
                pixels[y * width + x].a = 255;
            } else {
                pixels[y * width + x] = (Pixel32){0, 0, 0, 255};
            }
        }
    }
}

static inline float pixels_noise_hash(int x, int y, int seed)
{
    Uint32 h = (Uint32)(x * 374761393 + y * 668265263 + seed * 2147483647u);
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= (h >> 16);
    return (float)(h & 0xFFFFu) / 65535.0f;
}

#if BENCH_HAS_NEON
static inline void pixels_noise_lerp_row4(float c00, float c10, float c01, float c11,
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

static void pixels_generate_noise(Pixel32 *pixels, int width, int height,
                                  float phase, const PixelsBenchState *state,
                                  SDL_bool use_neon)
{
    const int seed = (int)(phase * 0.05f);
    const float hue_shift = phase * 0.3f;

    for (int y = 0; y < height; y++) {
        int gy = y / PIXELS_NOISE_CELL;
        float fy = (float)(y % PIXELS_NOISE_CELL) / (float)PIXELS_NOISE_CELL;

        int x = 0;
        while (x < width) {
            int gx = x / PIXELS_NOISE_CELL;
            float c00 = pixels_noise_hash(gx, gy, seed);
            float c10 = pixels_noise_hash(gx + 1, gy, seed);
            float c01 = pixels_noise_hash(gx, gy + 1, seed);
            float c11 = pixels_noise_hash(gx + 1, gy + 1, seed);

            int cell_start_x = gx * PIXELS_NOISE_CELL;
            int cell_end_x = SDL_min(cell_start_x + PIXELS_NOISE_CELL, width);

#if BENCH_HAS_NEON
            if (use_neon) {
                int run_x = x;
                while (run_x + 4 <= cell_end_x) {
                    float fx0 = (float)(run_x - cell_start_x) / (float)PIXELS_NOISE_CELL;
                    float fx_step = 1.0f / (float)PIXELS_NOISE_CELL;
                    float values[4];
                    pixels_noise_lerp_row4(c00, c10, c01, c11, fx0, fx_step, fy, values);
                    for (int i = 0; i < 4; i++) {
                        float intensity = pixels_clampf(values[i], 0.0f, 1.0f);
                        float hue = intensity * 2.0f * PIXELS_BENCH_PI + hue_shift;
                        Pixel32 *px = &pixels[y * width + run_x + i];
                        px->r = pixels_clamp_u8((int)((pixels_state_sin_rad(state, hue) + 1.0f) * 127.5f));
                        px->g = pixels_clamp_u8((int)(intensity * 255.0f));
                        px->b = pixels_clamp_u8((int)((pixels_state_cos_rad(state, hue) + 1.0f) * 127.5f));
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
                float fx = (float)(x - cell_start_x) / (float)PIXELS_NOISE_CELL;
                float top = c00 + (c10 - c00) * fx;
                float bottom = c01 + (c11 - c01) * fx;
                float intensity = pixels_clampf(top + (bottom - top) * fy, 0.0f, 1.0f);
                float hue = intensity * 2.0f * PIXELS_BENCH_PI + hue_shift;

                Pixel32 *px = &pixels[y * width + x];
                px->r = pixels_clamp_u8((int)((pixels_state_sin_rad(state, hue) + 1.0f) * 127.5f));
                px->g = pixels_clamp_u8((int)(intensity * 255.0f));
                px->b = pixels_clamp_u8((int)((pixels_state_cos_rad(state, hue) + 1.0f) * 127.5f));
                px->a = 255;
            }
        }
    }
}

static void pixels_generate_dither(Pixel32 *pixels, int width, int height,
                                   float phase, const PixelsBenchState *state)
{
    const float sweep_scale = 0.015f;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float g = (pixels_state_sin_rad(state, (float)x * sweep_scale + phase * 0.2f) +
                       pixels_state_sin_rad(state, (float)y * sweep_scale * 1.4f - phase * 0.15f) + 2.0f) * 0.25f;
            g = pixels_clampf(g, 0.0f, 1.0f);

            const Uint8 threshold = g_bayer4x4[y & 3][x & 3];
            const Uint8 level = (Uint8)(g * 16.0f);
            const SDL_bool lit = level > threshold;

            Pixel32 *px = &pixels[y * width + x];
            if (lit) {
                px->r = pixels_clamp_u8((int)(g * 255.0f) + 40);
                px->g = pixels_clamp_u8((int)(g * 200.0f) + 55);
                px->b = 255;
            } else {
                px->r = 8;
                px->g = 8;
                px->b = pixels_clamp_u8((int)(g * 80.0f));
            }
            px->a = 255;
        }
    }
}

static void pixels_build_palette_lut(Pixel32 lut[256], float phase, const PixelsBenchState *state)
{
    for (int i = 0; i < 256; i++) {
        float t = (float)i / 255.0f;
        float hue = t * 2.0f * PIXELS_BENCH_PI + phase * 0.4f;
        lut[i].r = pixels_clamp_u8((int)((pixels_state_sin_rad(state, hue) + 1.0f) * 127.5f));
        lut[i].g = pixels_clamp_u8((int)((pixels_state_sin_rad(state, hue + 2.09f) + 1.0f) * 127.5f));
        lut[i].b = pixels_clamp_u8((int)((pixels_state_sin_rad(state, hue + 4.19f) + 1.0f) * 127.5f));
        lut[i].a = 255;
    }
}

static void pixels_generate_palette(Pixel32 *pixels, int width, int height,
                                    float phase, const PixelsBenchState *state)
{
    Pixel32 lut[256];
    pixels_build_palette_lut(lut, phase, state);

    const float cx = width * 0.5f;
    const float cy = height * 0.5f;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float dx = (float)x - cx;
            float dy = (float)y - cy;
            float dist = sqrtf(dx * dx + dy * dy);
            Uint8 index = (Uint8)((int)(dist * 3.0f - phase * 40.0f) & 0xFF);
            pixels[y * width + x] = lut[index];
        }
    }
}

static void pixels_apply_colorkey(Pixel32 *pixels, size_t count)
{
    for (size_t i = 0; i < count; i++) {
        if (pixels[i].r == 0 && pixels[i].g == 0 && pixels[i].b == 0) {
            pixels[i].a = 0;
        }
    }
}

static SDL_bool pixels_resize_buffers(PixelsBenchState *state, SDL_Renderer *renderer,
                                      int width, int height)
{
    void *new_buffer = malloc((size_t)width * (size_t)height * sizeof(Pixel32));
    Uint8 *new_cells = malloc((size_t)width * (size_t)height);
    Uint8 *new_next_cells = malloc((size_t)width * (size_t)height);
    if (!new_buffer || !new_cells || !new_next_cells) {
        free(new_buffer);
        free(new_cells);
        free(new_next_cells);
        return SDL_FALSE;
    }

    free(state->pixel_buffer);
    free(state->cellular_cells);
    free(state->cellular_new_cells);
    state->pixel_buffer = new_buffer;
    state->cellular_cells = new_cells;
    state->cellular_new_cells = new_next_cells;
    state->cellular_seeded = SDL_FALSE;

    if (state->pixel_texture) {
        SDL_DestroyTexture(state->pixel_texture);
        state->pixel_texture = NULL;
    }
    if (renderer) {
        state->pixel_texture = SDL_CreateTexture(renderer,
                                                  SDL_PIXELFORMAT_RGBA8888,
                                                  SDL_TEXTUREACCESS_STREAMING,
                                                  width, height);
    }

    state->buffer_width = width;
    state->buffer_height = height;
    return SDL_TRUE;
}

void pixels_render_init(PixelsBenchState *state, SDL_Renderer *renderer)
{
    if (!state) return;

    pixels_resize_buffers(state, renderer, PIXELS_BASE_W, PIXELS_BASE_H);
    state->pixel_phase = 0.0f;
}

void pixels_render_cleanup(PixelsBenchState *state)
{
    if (!state) return;

    free(state->pixel_buffer);
    state->pixel_buffer = NULL;

    free(state->cellular_cells);
    state->cellular_cells = NULL;
    free(state->cellular_new_cells);
    state->cellular_new_cells = NULL;

    if (state->pixel_texture) {
        SDL_DestroyTexture(state->pixel_texture);
        state->pixel_texture = NULL;
    }
}

void pixels_render_scene(PixelsBenchState *state,
                        SDL_Renderer *renderer,
                        BenchMetrics *metrics,
                        double delta_seconds)
{
    if (!state || !renderer || !state->pixel_buffer) {
        return;
    }

    const float factor = pixels_state_stress_factor(state);
    const int region_height = SDL_max(1, bench_logical_h() - (int)state->top_margin);

    const float t = pixels_clampf((factor - 0.5f) / 6.5f, 0.0f, 1.0f);
    const int target_w = PIXELS_BASE_W + (int)(t * (float)(PIXELS_MAX_W - PIXELS_BASE_W));
    const int target_h = PIXELS_BASE_H + (int)(t * (float)(PIXELS_MAX_H - PIXELS_BASE_H));
    if (target_w != state->buffer_width || target_h != state->buffer_height) {
        pixels_resize_buffers(state, renderer, target_w, target_h);
    }

    const int width = state->buffer_width;
    const int height = state->buffer_height;

    state->pixel_phase += (float)(delta_seconds * (1.0f + factor));

    const int mode_duration = 300;
    const int auto_mode = ((int)(state->pixel_phase * 60.0f) / mode_duration) % PIXEL_MODE_MAX;
    const int current_mode = (state->forced_mode >= 0) ? state->forced_mode : auto_mode;
    state->current_mode = current_mode;

    Pixel32 *pixels = (Pixel32 *)state->pixel_buffer;

    Uint64 gen_start = SDL_GetPerformanceCounter();
    switch (current_mode) {
        case PIXEL_MODE_PLASMA:
            pixels_generate_plasma(pixels, width, height, state->pixel_phase, state);
            break;
        case PIXEL_MODE_MANDELBROT:
            pixels_generate_mandelbrot(pixels, width, height, state->pixel_phase, state);
            break;
        case PIXEL_MODE_CELLULAR:
            pixels_generate_cellular(pixels, width, height, state->pixel_phase, state);
            break;
        case PIXEL_MODE_NOISE:
            pixels_generate_noise(pixels, width, height, state->pixel_phase, state, state->neon_copy_enabled);
            break;
        case PIXEL_MODE_DITHER:
            pixels_generate_dither(pixels, width, height, state->pixel_phase, state);
            break;
        case PIXEL_MODE_PALETTE:
            pixels_generate_palette(pixels, width, height, state->pixel_phase, state);
            break;
        default:
            break;
    }
    Uint64 gen_end = SDL_GetPerformanceCounter();
    if (metrics) {
        metrics->stage_transform_ms += (double)(gen_end - gen_start) /
                                       (double)SDL_GetPerformanceFrequency() * 1000.0;
    }

    if (state->blend_mode == PIXELS_BLEND_COLORKEY) {
        pixels_apply_colorkey(pixels, (size_t)width * (size_t)height);
    }

    Uint64 draw_start = SDL_GetPerformanceCounter();

    if (state->pixel_texture) {
        const size_t pixel_count = (size_t)width * (size_t)height;
        if (state->upload_path == PIXELS_UPLOAD_LOCK) {
            void *tex_pixels = NULL;
            int pitch = 0;
            if (SDL_LockTexture(state->pixel_texture, NULL, &tex_pixels, &pitch) == 0) {
                (void)pitch;
                if (state->neon_copy_enabled) {
                    bench_neon_copy_u32((uint32_t *)tex_pixels, (uint32_t *)pixels, pixel_count);
                } else {
                    memcpy(tex_pixels, pixels, pixel_count * sizeof(Pixel32));
                }
                SDL_UnlockTexture(state->pixel_texture);
            }
        } else {
            SDL_UpdateTexture(state->pixel_texture, NULL, pixels, width * (int)sizeof(Pixel32));
        }
    }

    Uint64 upload_end = SDL_GetPerformanceCounter();
    if (metrics) {
        metrics->lock_unlock_overhead_ms += (double)(upload_end - draw_start) /
                                            (double)SDL_GetPerformanceFrequency() * 1000.0;
        metrics->pixel_operations++;
    }

    if (state->pixel_texture) {
        SDL_BlendMode blend = (state->blend_mode == PIXELS_BLEND_ADDITIVE)
                              ? SDL_BLENDMODE_ADD : SDL_BLENDMODE_BLEND;
        SDL_SetTextureBlendMode(state->pixel_texture, blend);

        const float scale = 2.0f + pixels_state_sin_rad(state, state->pixel_phase * 0.5f) * 0.5f;

        SDL_FRect dest = {
            bench_logical_w() * 0.5f - (float)width * scale * 0.5f,
            state->top_margin + region_height * 0.5f - (float)height * scale * 0.5f,
            (float)width * scale,
            (float)height * scale
        };

        SDL_RenderCopyF(renderer, state->pixel_texture, NULL, &dest);

        if (metrics) {
            metrics->draw_calls++;
            metrics->vertices_rendered += 4;
            metrics->triangles_rendered += 2;
            metrics->texture_switches++;
        }
    }

    Uint64 draw_end = SDL_GetPerformanceCounter();
    if (metrics) {
        metrics->stage_draw_ms += (double)(draw_end - draw_start) /
                                  (double)SDL_GetPerformanceFrequency() * 1000.0;
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    const int indicator_width = 20 + current_mode * 30;
    SDL_Rect indicator = {10, (int)state->top_margin + 10, indicator_width, 6};
    SDL_RenderFillRect(renderer, &indicator);

    const int perf_width = (int)(factor * 100.0f);
    SDL_Rect perf_indicator = {10, (int)state->top_margin + 25, perf_width, 4};
    SDL_SetRenderDrawColor(renderer, 255, (Uint8)(255 * (1.0f - factor)), 0, 255);
    SDL_RenderFillRect(renderer, &perf_indicator);

    if (metrics) {
        metrics->draw_calls += 2;
        metrics->vertices_rendered += 8;
        metrics->triangles_rendered += 4;
    }
}
