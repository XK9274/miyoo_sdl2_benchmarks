#include "pixels_bench/render.h"
#include "common/render_neon.h"

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "common/memory_opt.h"

#define PIXELS_BENCH_PI 3.14159265358979323846f
#define PIXEL_SURFACE_WIDTH 160
#define PIXEL_SURFACE_HEIGHT 120
#define FIRE_HEIGHT 60
#define FIRE_WIDTH 80

typedef enum {
    PIXEL_MODE_PLASMA = 0,
    PIXEL_MODE_FIRE,
    PIXEL_MODE_MANDELBROT,
    PIXEL_MODE_CELLULAR,
    PIXEL_MODE_MAX
} PixelMode;

typedef struct {
    Uint8 r, g, b, a;
} Pixel32;

static inline float pixels_clampf(float value, float min_val, float max_val)
{
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

static inline int pixels_clampi(int value, int min_val, int max_val)
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

/* Fast sine approximation using lookup table */
static float g_pixels_sin_table[256];
static SDL_bool g_pixels_sin_table_initialized = SDL_FALSE;

static void pixels_init_sin_table(void)
{
    if (g_pixels_sin_table_initialized) return;
    for (int i = 0; i < 256; i++) {
        g_pixels_sin_table[i] = sinf((float)i * 2.0f * PIXELS_BENCH_PI / 256.0f);
    }
    g_pixels_sin_table_initialized = SDL_TRUE;
}

static inline float pixels_fast_sin(float x)
{
    int index = (int)(x * 256.0f / (2.0f * PIXELS_BENCH_PI)) & 255;
    return g_pixels_sin_table[index];
}

static inline float pixels_fast_cos(float x)
{
    return pixels_fast_sin(x + PIXELS_BENCH_PI * 0.5f);
}

static void pixels_generate_plasma(Pixel32 *pixels, int width, int height, float phase)
{
    const float scale = 0.02f;
    const float time_scale = 0.1f;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float fx = (float)x * scale;
            float fy = (float)y * scale;

            float v1 = pixels_fast_sin(fx * 4.0f + phase * time_scale);
            float v2 = pixels_fast_sin(fy * 3.0f + phase * time_scale * 1.3f);
            float v3 = pixels_fast_sin((fx + fy) * 2.0f + phase * time_scale * 0.7f);
            float v4 = pixels_fast_sin(sqrtf(fx * fx + fy * fy) * 5.0f + phase * time_scale * 1.5f);

            float intensity = (v1 + v2 + v3 + v4) * 0.25f;
            intensity = (intensity + 1.0f) * 0.5f;

            float r_phase = intensity * 2.0f * PIXELS_BENCH_PI;
            float g_phase = intensity * 2.0f * PIXELS_BENCH_PI + PIXELS_BENCH_PI * 0.66f;
            float b_phase = intensity * 2.0f * PIXELS_BENCH_PI + PIXELS_BENCH_PI * 1.33f;

            pixels[y * width + x].r = pixels_clamp_u8((int)((pixels_fast_sin(r_phase) + 1.0f) * 127.5f));
            pixels[y * width + x].g = pixels_clamp_u8((int)((pixels_fast_sin(g_phase) + 1.0f) * 127.5f));
            pixels[y * width + x].b = pixels_clamp_u8((int)((pixels_fast_sin(b_phase) + 1.0f) * 127.5f));
            pixels[y * width + x].a = 255;
        }
    }
}

static void pixels_generate_fire(Pixel32 *pixels, int width, int height, float phase, int *fire_buffer)
{
    static SDL_bool fire_initialized = SDL_FALSE;
    if (!fire_initialized) {
        for (int x = 0; x < FIRE_WIDTH; x++) {
            fire_buffer[(FIRE_HEIGHT - 1) * FIRE_WIDTH + x] = 255;
        }
        fire_initialized = SDL_TRUE;
    }

    for (int y = 0; y < FIRE_HEIGHT - 1; y++) {
        for (int x = 0; x < FIRE_WIDTH; x++) {
            int sum = 0;
            int count = 0;

            for (int dy = 0; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    int nx = x + dx;
                    int ny = y + dy;
                    if (nx >= 0 && nx < FIRE_WIDTH && ny >= 0 && ny < FIRE_HEIGHT) {
                        sum += fire_buffer[ny * FIRE_WIDTH + nx];
                        count++;
                    }
                }
            }

            int average = sum / count;
            int cooling = 2 + (rand() % 4);
            int new_value = average - cooling;
            if (new_value < 0) new_value = 0;

            fire_buffer[y * FIRE_WIDTH + x] = new_value;
        }
    }

    int disturbance = (int)(phase * 10.0f) % FIRE_WIDTH;
    for (int i = 0; i < 5; i++) {
        int x = (disturbance + i) % FIRE_WIDTH;
        fire_buffer[(FIRE_HEIGHT - 1) * FIRE_WIDTH + x] = 200 + (rand() % 56);
    }

    memset(pixels, 0, width * height * sizeof(Pixel32));

    int start_x = (width - FIRE_WIDTH) / 2;
    int start_y = (height - FIRE_HEIGHT) / 2;

    for (int y = 0; y < FIRE_HEIGHT; y++) {
        for (int x = 0; x < FIRE_WIDTH; x++) {
            int px = start_x + x;
            int py = start_y + y;
            if (px >= 0 && px < width && py >= 0 && py < height) {
                int intensity = fire_buffer[y * FIRE_WIDTH + x];

                Uint8 r, g, b;
                if (intensity < 64) {
                    r = intensity * 4;
                    g = 0;
                    b = 0;
                } else if (intensity < 128) {
                    r = 255;
                    g = (intensity - 64) * 4;
                    b = 0;
                } else if (intensity < 192) {
                    r = 255;
                    g = 255;
                    b = (intensity - 128) * 4;
                } else {
                    r = 255;
                    g = 255;
                    b = 255;
                }

                pixels[py * width + px].r = r;
                pixels[py * width + px].g = g;
                pixels[py * width + px].b = b;
                pixels[py * width + px].a = 255;
            }
        }
    }
}

static void pixels_generate_mandelbrot(Pixel32 *pixels, int width, int height, float phase)
{
    const float zoom = 1.0f + phase * 0.05f;
    const float center_x = -0.5f + pixels_fast_cos(phase * 0.3f) * 0.2f;
    const float center_y = 0.0f + pixels_fast_sin(phase * 0.2f) * 0.2f;
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

static void pixels_generate_cellular(Pixel32 *pixels, int width, int height, float phase)
{
    static Uint8 *cells = NULL;
    static Uint8 *new_cells = NULL;
    static SDL_bool initialized = SDL_FALSE;

    if (!initialized) {
        cells = malloc(width * height);
        new_cells = malloc(width * height);

        for (int i = 0; i < width * height; i++) {
            cells[i] = (rand() % 100) < 30 ? 1 : 0;
        }
        initialized = SDL_TRUE;
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

    Uint8 *temp = cells;
    cells = new_cells;
    new_cells = temp;

    float color_phase = phase * 0.5f;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (cells[y * width + x]) {
                float fx = (float)x / (float)width;
                float fy = (float)y / (float)height;

                pixels[y * width + x].r = pixels_clamp_u8((int)((pixels_fast_sin(color_phase + fx) + 1.0f) * 127.5f));
                pixels[y * width + x].g = pixels_clamp_u8((int)((pixels_fast_sin(color_phase + fy + 2.0f) + 1.0f) * 127.5f));
                pixels[y * width + x].b = pixels_clamp_u8((int)((pixels_fast_sin(color_phase + fx + fy + 4.0f) + 1.0f) * 127.5f));
                pixels[y * width + x].a = 255;
            } else {
                pixels[y * width + x] = (Pixel32){0, 0, 0, 255};
            }
        }
    }
}

void pixels_render_init(PixelsBenchState *state, SDL_Renderer *renderer)
{
    if (!state) return;

    pixels_init_sin_table();

    state->pixel_surface = SDL_CreateRGBSurface(0,
                                               PIXEL_SURFACE_WIDTH,
                                               PIXEL_SURFACE_HEIGHT,
                                               32,
                                               0x00FF0000,
                                               0x0000FF00,
                                               0x000000FF,
                                               0xFF000000);

    state->pixel_buffer = malloc(PIXEL_SURFACE_WIDTH * PIXEL_SURFACE_HEIGHT * sizeof(Pixel32));

    if (renderer) {
        state->pixel_texture = SDL_CreateTexture(renderer,
                                                SDL_PIXELFORMAT_RGBA8888,
                                                SDL_TEXTUREACCESS_STREAMING,
                                                PIXEL_SURFACE_WIDTH,
                                                PIXEL_SURFACE_HEIGHT);
    }

    state->pixel_phase = 0.0f;
    state->pixel_plasma_offset = 0;
}

void pixels_render_cleanup(PixelsBenchState *state)
{
    if (!state) return;

    if (state->pixel_surface) {
        SDL_FreeSurface(state->pixel_surface);
        state->pixel_surface = NULL;
    }

    if (state->pixel_buffer) {
        free(state->pixel_buffer);
        state->pixel_buffer = NULL;
    }

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

    state->pixel_phase += (float)(delta_seconds * (1.0f + factor * 2.0f));

    const int mode_duration = 300;
    const int current_mode = ((int)(state->pixel_phase * 60.0f) / mode_duration) % PIXEL_MODE_MAX;

    const int operations_per_frame = pixels_clampi((int)(1 + factor * 2), 1, 3);

    for (int op = 0; op < operations_per_frame; op++) {
        Uint64 start_time = SDL_GetPerformanceCounter();

        Pixel32 *pixels = (Pixel32 *)state->pixel_buffer;
        if (!pixels) {
            continue;
        }

        static int fire_buffer[FIRE_HEIGHT * FIRE_WIDTH] = {0};

        float op_phase = state->pixel_phase + (float)op * 0.1f;

        switch (current_mode) {
            case PIXEL_MODE_PLASMA:
                pixels_generate_plasma(pixels, PIXEL_SURFACE_WIDTH, PIXEL_SURFACE_HEIGHT, op_phase);
                break;
            case PIXEL_MODE_FIRE:
                pixels_generate_fire(pixels, PIXEL_SURFACE_WIDTH, PIXEL_SURFACE_HEIGHT, op_phase, fire_buffer);
                break;
            case PIXEL_MODE_MANDELBROT:
                pixels_generate_mandelbrot(pixels, PIXEL_SURFACE_WIDTH, PIXEL_SURFACE_HEIGHT, op_phase);
                break;
            case PIXEL_MODE_CELLULAR:
                pixels_generate_cellular(pixels, PIXEL_SURFACE_WIDTH, PIXEL_SURFACE_HEIGHT, op_phase);
                break;
        }

        if (state->pixel_texture) {
            void *tex_pixels = NULL;
            int pitch = 0;
            if (SDL_LockTexture(state->pixel_texture, NULL, &tex_pixels, &pitch) == 0) {
                (void)pitch;
                const size_t pixel_count = (size_t)PIXEL_SURFACE_WIDTH * (size_t)PIXEL_SURFACE_HEIGHT;
                bench_neon_copy_u32((uint32_t *)tex_pixels, (uint32_t *)pixels, pixel_count);
                SDL_UnlockTexture(state->pixel_texture);
            }
        }

        if (state->pixel_surface) {
            memcpy(state->pixel_surface->pixels,
                   pixels,
                   PIXEL_SURFACE_WIDTH * PIXEL_SURFACE_HEIGHT * sizeof(Pixel32));
        }

        Uint64 end_time = SDL_GetPerformanceCounter();
        if (metrics) {
            double lock_time = (double)(end_time - start_time) /
                             (double)SDL_GetPerformanceFrequency() * 1000.0;
            metrics->lock_unlock_overhead_ms += lock_time;
            metrics->pixel_operations++;
        }
    }

    if (state->pixel_texture) {
        const float scale = 2.0f + pixels_fast_sin(state->pixel_phase * 0.5f) * 0.5f;

        SDL_FRect dest = {
            bench_logical_w() * 0.5f - PIXEL_SURFACE_WIDTH * scale * 0.5f,
            state->top_margin + region_height * 0.5f - PIXEL_SURFACE_HEIGHT * scale * 0.5f,
            PIXEL_SURFACE_WIDTH * scale,
            PIXEL_SURFACE_HEIGHT * scale
        };

        SDL_RenderCopyF(renderer, state->pixel_texture, NULL, &dest);

        if (metrics) {
            metrics->draw_calls++;
            metrics->vertices_rendered += 4;
            metrics->triangles_rendered += 2;
            metrics->texture_switches++;
        }
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
