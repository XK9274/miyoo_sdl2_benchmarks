#include "render_suite/scenes/pixels.h"

#include <stdlib.h>
#include <math.h>
#include <string.h>

#define RS_PI 3.14159265358979323846f
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

static inline Uint8 rs_clamp_u8(int value)
{
    if (value < 0) return 0;
    if (value > 255) return 255;
    return (Uint8)value;
}

// Fast sine approximation using lookup table
static float rs_sin_table[256];
static SDL_bool sin_table_initialized = SDL_FALSE;

static void rs_init_sin_table(void)
{
    if (sin_table_initialized) return;
    for (int i = 0; i < 256; i++) {
        rs_sin_table[i] = sinf((float)i * 2.0f * RS_PI / 256.0f);
    }
    sin_table_initialized = SDL_TRUE;
}

static inline float rs_fast_sin(float x)
{
    int index = (int)(x * 256.0f / (2.0f * RS_PI)) & 255;
    return rs_sin_table[index];
}

static inline float rs_fast_cos(float x)
{
    return rs_fast_sin(x + RS_PI * 0.5f);
}

static void rs_generate_plasma(Pixel32 *pixels, int width, int height, float phase)
{
    const float scale = 0.02f;
    const float time_scale = 0.1f;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float fx = (float)x * scale;
            float fy = (float)y * scale;

            float v1 = rs_fast_sin(fx * 4.0f + phase * time_scale);
            float v2 = rs_fast_sin(fy * 3.0f + phase * time_scale * 1.3f);
            float v3 = rs_fast_sin((fx + fy) * 2.0f + phase * time_scale * 0.7f);
            float v4 = rs_fast_sin(sqrtf(fx * fx + fy * fy) * 5.0f + phase * time_scale * 1.5f);

            float intensity = (v1 + v2 + v3 + v4) * 0.25f;
            intensity = (intensity + 1.0f) * 0.5f; // Normalize to 0-1

            // Create color cycling
            float r_phase = intensity * 2.0f * RS_PI;
            float g_phase = intensity * 2.0f * RS_PI + RS_PI * 0.66f;
            float b_phase = intensity * 2.0f * RS_PI + RS_PI * 1.33f;

            pixels[y * width + x].r = rs_clamp_u8((int)((rs_fast_sin(r_phase) + 1.0f) * 127.5f));
            pixels[y * width + x].g = rs_clamp_u8((int)((rs_fast_sin(g_phase) + 1.0f) * 127.5f));
            pixels[y * width + x].b = rs_clamp_u8((int)((rs_fast_sin(b_phase) + 1.0f) * 127.5f));
            pixels[y * width + x].a = 255;
        }
    }
}

static void rs_generate_fire(Pixel32 *pixels, int width, int height, float phase, int *fire_buffer)
{
    // Initialize fire buffer if needed
    static SDL_bool fire_initialized = SDL_FALSE;
    if (!fire_initialized) {
        // Set bottom row to hot
        for (int x = 0; x < FIRE_WIDTH; x++) {
            fire_buffer[(FIRE_HEIGHT - 1) * FIRE_WIDTH + x] = 255;
        }
        fire_initialized = SDL_TRUE;
    }

    // Update fire simulation
    for (int y = 0; y < FIRE_HEIGHT - 1; y++) {
        for (int x = 0; x < FIRE_WIDTH; x++) {
            int sum = 0;
            int count = 0;

            // Sample surrounding pixels
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

            // Cool down and add some randomness
            int average = sum / count;
            int cooling = 2 + (rand() % 4);
            int new_value = average - cooling;
            if (new_value < 0) new_value = 0;

            fire_buffer[y * FIRE_WIDTH + x] = new_value;
        }
    }

    // Add disturbance to bottom row
    int disturbance = (int)(phase * 10.0f) % FIRE_WIDTH;
    for (int i = 0; i < 5; i++) {
        int x = (disturbance + i) % FIRE_WIDTH;
        fire_buffer[(FIRE_HEIGHT - 1) * FIRE_WIDTH + x] = 200 + (rand() % 56);
    }

    // Convert fire buffer to pixels
    memset(pixels, 0, width * height * sizeof(Pixel32));

    int start_x = (width - FIRE_WIDTH) / 2;
    int start_y = (height - FIRE_HEIGHT) / 2;

    for (int y = 0; y < FIRE_HEIGHT; y++) {
        for (int x = 0; x < FIRE_WIDTH; x++) {
            int px = start_x + x;
            int py = start_y + y;
            if (px >= 0 && px < width && py >= 0 && py < height) {
                int intensity = fire_buffer[y * FIRE_WIDTH + x];

                // Fire color palette
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

static void rs_generate_mandelbrot(Pixel32 *pixels, int width, int height, float phase)
{
    const float zoom = 1.0f + phase * 0.05f;
    const float center_x = -0.5f + rs_fast_cos(phase * 0.3f) * 0.2f;
    const float center_y = 0.0f + rs_fast_sin(phase * 0.2f) * 0.2f;
    const int max_iterations = 16; // Reduced from 32

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

            // Color based on iterations
            if (iterations == max_iterations) {
                pixels[y * width + x] = (Pixel32){0, 0, 0, 255};
            } else {
                float t = (float)iterations / (float)max_iterations;
                float hue = t * 6.0f + phase * 0.5f;

                // Simple HSV to RGB conversion
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

                pixels[y * width + x].r = rs_clamp_u8((int)(rf * 255));
                pixels[y * width + x].g = rs_clamp_u8((int)(gf * 255));
                pixels[y * width + x].b = rs_clamp_u8((int)(bf * 255));
                pixels[y * width + x].a = 255;
            }
        }
    }
}

static void rs_generate_cellular(Pixel32 *pixels, int width, int height, float phase)
{
    static Uint8 *cells = NULL;
    static Uint8 *new_cells = NULL;
    static SDL_bool initialized = SDL_FALSE;

    if (!initialized) {
        cells = malloc(width * height);
        new_cells = malloc(width * height);

        // Initialize with random cells
        for (int i = 0; i < width * height; i++) {
            cells[i] = (rand() % 100) < 30 ? 1 : 0;
        }
        initialized = SDL_TRUE;
    }

    // Conway's Game of Life rules with modifications
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

            // Modified rules for more dynamic behavior
            if (current) {
                new_cells[y * width + x] = (neighbors == 2 || neighbors == 3) ? 1 : 0;
            } else {
                new_cells[y * width + x] = (neighbors == 3) ? 1 : 0;
            }
        }
    }

    // Add some randomness based on phase
    if ((int)(phase * 10.0f) % 60 == 0) {
        for (int i = 0; i < 10; i++) {
            int x = rand() % width;
            int y = rand() % height;
            new_cells[y * width + x] = 1;
        }
    }

    // Swap buffers
    Uint8 *temp = cells;
    cells = new_cells;
    new_cells = temp;

    // Convert to pixels with color based on phase
    float color_phase = phase * 0.5f;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (cells[y * width + x]) {
                float fx = (float)x / (float)width;
                float fy = (float)y / (float)height;

                pixels[y * width + x].r = rs_clamp_u8((int)((rs_fast_sin(color_phase + fx) + 1.0f) * 127.5f));
                pixels[y * width + x].g = rs_clamp_u8((int)((rs_fast_sin(color_phase + fy + 2.0f) + 1.0f) * 127.5f));
                pixels[y * width + x].b = rs_clamp_u8((int)((rs_fast_sin(color_phase + fx + fy + 4.0f) + 1.0f) * 127.5f));
                pixels[y * width + x].a = 255;
            } else {
                pixels[y * width + x] = (Pixel32){0, 0, 0, 255};
            }
        }
    }
}

void rs_scene_pixels_init(RenderSuiteState *state, SDL_Renderer *renderer)
{
    (void)renderer; // Unused parameter
    if (!state) return;

    rs_init_sin_table();

    // Create surface for pixel manipulation
    state->pixel_surface = SDL_CreateRGBSurface(0, PIXEL_SURFACE_WIDTH, PIXEL_SURFACE_HEIGHT, 32,
                                               0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

    if (state->pixel_surface) {
        state->pixel_buffer = malloc(PIXEL_SURFACE_WIDTH * PIXEL_SURFACE_HEIGHT * sizeof(Pixel32));
    }

    state->pixel_phase = 0.0f;
    state->pixel_plasma_offset = 0;
}

void rs_scene_pixels_cleanup(RenderSuiteState *state)
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
}

void rs_scene_pixels(RenderSuiteState *state,
                     SDL_Renderer *renderer,
                     BenchMetrics *metrics,
                     double delta_seconds)
{
    if (!state || !renderer || !state->pixel_surface || !state->pixel_buffer) {
        return;
    }

    const float factor = rs_state_stress_factor(state);
    const int region_height = SDL_max(1, BENCH_SCREEN_H - (int)state->top_margin);

    // Update animation phase
    state->pixel_phase += (float)(delta_seconds * (1.0f + factor * 2.0f));

    // Cycle through different pixel modes
    const int mode_duration = 300; // frames per mode
    const int current_mode = ((int)(state->pixel_phase * 60.0f) / mode_duration) % PIXEL_MODE_MAX;

    // Number of operations per frame based on stress level
    const int operations_per_frame = rs_clampi((int)(1 + factor * 2), 1, 3);

    for (int op = 0; op < operations_per_frame; op++) {
        Uint64 start_time = SDL_GetPerformanceCounter();

        // Lock surface for pixel access
        if (SDL_LockSurface(state->pixel_surface) < 0) {
            continue;
        }

        Pixel32 *pixels = (Pixel32 *)state->pixel_buffer;
        static int fire_buffer[FIRE_HEIGHT * FIRE_WIDTH] = {0};

        float op_phase = state->pixel_phase + (float)op * 0.1f;

        // Generate different pixel effects
        switch (current_mode) {
            case PIXEL_MODE_PLASMA:
                rs_generate_plasma(pixels, PIXEL_SURFACE_WIDTH, PIXEL_SURFACE_HEIGHT, op_phase);
                break;
            case PIXEL_MODE_FIRE:
                rs_generate_fire(pixels, PIXEL_SURFACE_WIDTH, PIXEL_SURFACE_HEIGHT, op_phase, fire_buffer);
                break;
            case PIXEL_MODE_MANDELBROT:
                rs_generate_mandelbrot(pixels, PIXEL_SURFACE_WIDTH, PIXEL_SURFACE_HEIGHT, op_phase);
                break;
            case PIXEL_MODE_CELLULAR:
                rs_generate_cellular(pixels, PIXEL_SURFACE_WIDTH, PIXEL_SURFACE_HEIGHT, op_phase);
                break;
        }

        // Copy pixel data to surface
        memcpy(state->pixel_surface->pixels, pixels, PIXEL_SURFACE_WIDTH * PIXEL_SURFACE_HEIGHT * sizeof(Pixel32));

        SDL_UnlockSurface(state->pixel_surface);

        Uint64 end_time = SDL_GetPerformanceCounter();
        if (metrics) {
            double lock_time = (double)(end_time - start_time) /
                             (double)SDL_GetPerformanceFrequency() * 1000.0;
            metrics->lock_unlock_overhead_ms += lock_time;
            metrics->pixel_operations++;
        }
    }

    // Create texture from surface and render it
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, state->pixel_surface);
    if (texture) {
        // Render a single, clearly visible copy
        const float scale = 2.0f + rs_fast_sin(state->pixel_phase * 0.5f) * 0.5f;

        SDL_FRect dest = {
            BENCH_SCREEN_W * 0.5f - PIXEL_SURFACE_WIDTH * scale * 0.5f,
            state->top_margin + region_height * 0.5f - PIXEL_SURFACE_HEIGHT * scale * 0.5f,
            PIXEL_SURFACE_WIDTH * scale,
            PIXEL_SURFACE_HEIGHT * scale
        };

        SDL_RenderCopyF(renderer, texture, NULL, &dest);

        if (metrics) {
            metrics->draw_calls++;
            metrics->vertices_rendered += 4;
            metrics->triangles_rendered += 2;
            metrics->texture_switches++;
        }

        SDL_DestroyTexture(texture);
    }

    // Draw mode indicator
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    // Simple mode indicator as rectangles
    const int indicator_width = 20 + current_mode * 30;
    SDL_Rect indicator = {10, (int)state->top_margin + 10, indicator_width, 6};
    SDL_RenderFillRect(renderer, &indicator);

    // Performance indicator
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