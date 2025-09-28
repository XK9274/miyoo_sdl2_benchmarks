#include "render_suite/scenes/memory.h"
#include "render_suite/render_neon.h"

#include <stdlib.h>
#include <math.h>

#define RS_PI 3.14159265358979323846f
#define MAX_DYNAMIC_TEXTURES 50
#define TEXTURE_POOL_SIZE 20
#define MIN_TEXTURE_SIZE 16
#define MAX_TEXTURE_SIZE 128

typedef struct {
    SDL_Texture *texture;
    Uint32 *pixel_cache;
    size_t pixel_capacity;
    int width;
    int height;
    Uint32 format;
    Uint64 allocation_time;
    float life_remaining;
    SDL_bool in_use;
    SDL_bool dirty;
} ResourceTexture;

typedef struct {
    ResourceTexture textures[MAX_DYNAMIC_TEXTURES];
    int active_count;
    int pool_index;
    Uint64 total_allocated_bytes;
    Uint64 peak_allocated_bytes;
    double total_allocation_time_ms;
    int allocation_count;
    int deallocation_count;
} ResourceManager;

static ResourceManager g_resource_manager = {0};

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

static Uint32 rs_calculate_texture_bytes(int width, int height, Uint32 format)
{
    int bytes_per_pixel = 4; // Assume RGBA8888 for simplicity
    switch (format) {
        case SDL_PIXELFORMAT_RGB888:
        case SDL_PIXELFORMAT_BGR888:
            bytes_per_pixel = 3;
            break;
        case SDL_PIXELFORMAT_RGB565:
        case SDL_PIXELFORMAT_BGR565:
        case SDL_PIXELFORMAT_ARGB4444:
        case SDL_PIXELFORMAT_RGBA4444:
        case SDL_PIXELFORMAT_ABGR4444:
        case SDL_PIXELFORMAT_BGRA4444:
            bytes_per_pixel = 2;
            break;
        case SDL_PIXELFORMAT_INDEX8:
            bytes_per_pixel = 1;
            break;
        default:
            bytes_per_pixel = 4;
            break;
    }
    return (Uint32)(width * height * bytes_per_pixel);
}

static void rs_generate_texture_data(Uint32 *pixels, int width, int height, float phase, int pattern)
{
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float fx = (float)x / (float)width;
            float fy = (float)y / (float)height;
            Uint8 r = 0, g = 0, b = 0, a = 255;

            switch (pattern % 4) {
                case 0: // Gradient
                    r = (Uint8)(fx * 255);
                    g = (Uint8)(fy * 255);
                    b = (Uint8)(sinf(phase + fx * RS_PI) * 128 + 127);
                    break;
                case 1: // Checkerboard
                    {
                        int check_size = 8;
                        int check_x = (x / check_size) % 2;
                        int check_y = (y / check_size) % 2;
                        Uint8 intensity = (check_x ^ check_y) ? 255 : 64;
                        r = intensity;
                        g = (Uint8)(intensity * sinf(phase) * 0.5f + intensity * 0.5f);
                        b = (Uint8)(intensity * cosf(phase) * 0.5f + intensity * 0.5f);
                    }
                    break;
                case 2: // Plasma
                    {
                        float v1 = sinf(fx * 10.0f + phase);
                        float v2 = sinf(fy * 10.0f + phase * 1.3f);
                        float v3 = sinf((fx + fy) * 8.0f + phase * 0.8f);
                        float intensity = (v1 + v2 + v3) / 3.0f;
                        r = (Uint8)((intensity + 1.0f) * 127.5f);
                        g = (Uint8)((sinf(intensity * RS_PI + phase) + 1.0f) * 127.5f);
                        b = (Uint8)((cosf(intensity * RS_PI + phase * 1.5f) + 1.0f) * 127.5f);
                    }
                    break;
                case 3: // Noise-like
                    {
                        // Simple pseudo-random pattern
                        int seed = (x * 73 + y * 137 + (int)(phase * 100)) % 255;
                        r = (Uint8)(seed % 256);
                        g = (Uint8)((seed * 17) % 256);
                        b = (Uint8)((seed * 31) % 256);
                    }
                    break;
            }

            pixels[y * width + x] = (a << 24) | (r << 16) | (g << 8) | b;
        }
    }
}

static SDL_bool rs_create_dynamic_texture(ResourceTexture *res,
                                          SDL_Renderer *renderer,
                                          int width,
                                          int height,
                                          float phase,
                                          int pattern,
                                          BenchMetrics *metrics)
{
    Uint64 start_time = SDL_GetPerformanceCounter();

    res->texture = SDL_CreateTexture(renderer,
                                     SDL_PIXELFORMAT_RGBA8888,
                                     SDL_TEXTUREACCESS_STREAMING,
                                     width,
                                     height);
    if (!res->texture) {
        return SDL_FALSE;
    }

    const size_t pixel_count = (size_t)width * (size_t)height;
    res->pixel_cache = malloc(sizeof(Uint32) * pixel_count);
    if (!res->pixel_cache) {
        SDL_DestroyTexture(res->texture);
        res->texture = NULL;
        return SDL_FALSE;
    }
    res->pixel_capacity = pixel_count;
    res->format = SDL_PIXELFORMAT_RGBA8888;
    res->dirty = SDL_TRUE;

    rs_generate_texture_data(res->pixel_cache, width, height, phase, pattern);

    // Update texture
    void *texture_pixels;
    int pitch;
    if (SDL_LockTexture(res->texture, NULL, &texture_pixels, &pitch) == 0) {
        (void)pitch;
        rs_neon_copy_u32((uint32_t *)texture_pixels,
                         res->pixel_cache,
                         res->pixel_capacity);
        SDL_UnlockTexture(res->texture);
    }

    Uint64 end_time = SDL_GetPerformanceCounter();
    if (metrics) {
        double allocation_time = (double)(end_time - start_time) /
                               (double)SDL_GetPerformanceFrequency() * 1000.0;
        metrics->allocation_time_ms += allocation_time;
        metrics->resource_allocations++;

        Uint32 texture_bytes = rs_calculate_texture_bytes(width, height, SDL_PIXELFORMAT_RGBA8888);
        metrics->memory_allocated_bytes += texture_bytes;
        if (metrics->memory_allocated_bytes > metrics->memory_peak_bytes) {
            metrics->memory_peak_bytes = metrics->memory_allocated_bytes;
        }

        g_resource_manager.total_allocation_time_ms += allocation_time;
        g_resource_manager.total_allocated_bytes += texture_bytes;
        g_resource_manager.allocation_count++;

        if (g_resource_manager.total_allocated_bytes > g_resource_manager.peak_allocated_bytes) {
            g_resource_manager.peak_allocated_bytes = g_resource_manager.total_allocated_bytes;
        }
    }

    return SDL_TRUE;
}

static void rs_destroy_resource_texture(ResourceTexture *res, BenchMetrics *metrics)
{
    if (!res || !res->texture) {
        return;
    }

    if (metrics) {
        Uint32 texture_bytes = rs_calculate_texture_bytes(res->width, res->height,
                                                         SDL_PIXELFORMAT_RGBA8888);
        metrics->memory_allocated_bytes -= texture_bytes;
        metrics->resource_deallocations++;

        g_resource_manager.total_allocated_bytes -= texture_bytes;
        g_resource_manager.deallocation_count++;
    }

    SDL_DestroyTexture(res->texture);
    res->texture = NULL;
    if (res->pixel_cache) {
        free(res->pixel_cache);
        res->pixel_cache = NULL;
    }
    res->pixel_capacity = 0;
    res->in_use = SDL_FALSE;
    res->dirty = SDL_FALSE;
}

static void rs_update_resource_pool(SDL_Renderer *renderer, float stress_factor,
                                    float phase, float delta_seconds, BenchMetrics *metrics)
{
    // Calculate target texture count based on stress level
    const int min_textures = 5;
    const int max_textures = rs_clampi((int)(min_textures + stress_factor * 30), min_textures, MAX_DYNAMIC_TEXTURES);

    // Age existing textures
    for (int i = 0; i < MAX_DYNAMIC_TEXTURES; i++) {
        ResourceTexture *res = &g_resource_manager.textures[i];
        if (res->in_use) {
            res->life_remaining -= delta_seconds;
            if (res->life_remaining <= 0.0f) {
                rs_destroy_resource_texture(res, metrics);
                g_resource_manager.active_count--;
            }
        }
    }

    // Create new textures if under target count
    while (g_resource_manager.active_count < max_textures) {
        // Find free slot
        int slot = -1;
        for (int i = 0; i < MAX_DYNAMIC_TEXTURES; i++) {
            if (!g_resource_manager.textures[i].in_use) {
                slot = i;
                break;
            }
        }

        if (slot == -1) {
            break; // No free slots
        }

        // Create new texture with random size and pattern
        int width = MIN_TEXTURE_SIZE + (rand() % (MAX_TEXTURE_SIZE - MIN_TEXTURE_SIZE));
        int height = MIN_TEXTURE_SIZE + (rand() % (MAX_TEXTURE_SIZE - MIN_TEXTURE_SIZE));
        int pattern = rand() % 4;

        // Vary texture lifetime based on stress
        float lifetime = 1.0f + (float)rand() / RAND_MAX * (3.0f + stress_factor * 2.0f);

        ResourceTexture *res = &g_resource_manager.textures[slot];
        if (rs_create_dynamic_texture(res, renderer, width, height, phase, pattern, metrics)) {
            res->width = width;
            res->height = height;
            res->format = SDL_PIXELFORMAT_RGBA8888;
            res->allocation_time = SDL_GetPerformanceCounter();
            res->life_remaining = lifetime;
            res->in_use = SDL_TRUE;
            res->dirty = SDL_FALSE;
            g_resource_manager.active_count++;
        } else {
            break; // Allocation failed
        }
    }
}

static void rs_render_resource_textures(SDL_Renderer *renderer, float phase,
                                        int region_height, float top_margin,
                                        BenchMetrics *metrics)
{
    const int columns = 5; // Reduced from 8
    const int rows = 4;    // Reduced from 6
    const float cell_width = (float)BENCH_SCREEN_W / (float)columns;
    const float cell_height = (float)region_height / (float)rows;

    int texture_index = 0;
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < columns; col++) {
            // Find next active texture
            while (texture_index < MAX_DYNAMIC_TEXTURES &&
                   !g_resource_manager.textures[texture_index].in_use) {
                texture_index++;
            }

            if (texture_index >= MAX_DYNAMIC_TEXTURES) {
                return; // No more textures
            }

            ResourceTexture *res = &g_resource_manager.textures[texture_index];
            if (!res->texture) {
                texture_index++;
                continue;
            }

            // Calculate position with slight animation
            float x = col * cell_width + sinf(phase + (float)texture_index * 0.1f) * 5.0f;
            float y = top_margin + row * cell_height + cosf(phase + (float)texture_index * 0.15f) * 3.0f;

            // Scale based on life remaining
            float scale = rs_clampf(res->life_remaining, 0.1f, 1.0f);
            float w = cell_width * 0.8f * scale;
            float h = cell_height * 0.8f * scale;

            SDL_FRect dest = {x + (cell_width - w) * 0.5f, y + (cell_height - h) * 0.5f, w, h};
            SDL_RenderCopyF(renderer, res->texture, NULL, &dest);

            if (metrics) {
                metrics->draw_calls++;
                metrics->vertices_rendered += 4;
                metrics->triangles_rendered += 2;
                metrics->texture_switches++;
            }

            texture_index++;
        }
    }
}

void rs_scene_memory_init(RenderSuiteState *state, SDL_Renderer *renderer)
{
    (void)state;
    (void)renderer;

    // Initialize resource manager
    memset(&g_resource_manager, 0, sizeof(g_resource_manager));
}

void rs_scene_memory_cleanup(RenderSuiteState *state)
{
    (void)state;

    // Clean up all resources
    for (int i = 0; i < MAX_DYNAMIC_TEXTURES; i++) {
        if (g_resource_manager.textures[i].in_use) {
            rs_destroy_resource_texture(&g_resource_manager.textures[i], NULL);
        }
    }
    memset(&g_resource_manager, 0, sizeof(g_resource_manager));
}

void rs_scene_memory(RenderSuiteState *state,
                     SDL_Renderer *renderer,
                     BenchMetrics *metrics,
                     double delta_seconds)
{
    if (!state || !renderer) {
        return;
    }

    const float factor = rs_state_stress_factor(state);
    const int region_height = SDL_max(1, BENCH_SCREEN_H - (int)state->top_margin);

    // Update animation phase
    state->resources_phase += (float)(delta_seconds * (1.0f + factor * 2.0f));

    // Update resource pool
    rs_update_resource_pool(renderer, factor, state->resources_phase, (float)delta_seconds, metrics);

    // Test texture streaming - update a few textures per frame
    const int updates_per_frame = rs_clampi((int)(1 + factor * 2), 1, 3);
    for (int i = 0; i < updates_per_frame; i++) {
        int update_index = (state->resource_allocation_index + i) % MAX_DYNAMIC_TEXTURES;
        ResourceTexture *res = &g_resource_manager.textures[update_index];

        if (res->in_use && res->texture && res->pixel_cache) {
            Uint64 start_time = SDL_GetPerformanceCounter();

            // Update texture with new pattern
            void *pixels;
            int pitch;
            if (SDL_LockTexture(res->texture, NULL, &pixels, &pitch) == 0) {
                (void)pitch;
                rs_generate_texture_data(res->pixel_cache,
                                         res->width,
                                         res->height,
                                         state->resources_phase + (float)i,
                                         (update_index + i) % 4);
                rs_neon_copy_u32((uint32_t *)pixels,
                                 res->pixel_cache,
                                 res->pixel_capacity);
                SDL_UnlockTexture(res->texture);

                Uint64 end_time = SDL_GetPerformanceCounter();
                if (metrics) {
                    double update_time = (double)(end_time - start_time) /
                                       (double)SDL_GetPerformanceFrequency() * 1000.0;
                    metrics->lock_unlock_overhead_ms += update_time;
                }
            }
        }
    }
    state->resource_allocation_index = (state->resource_allocation_index + updates_per_frame) % MAX_DYNAMIC_TEXTURES;

    // Render all active textures
    rs_render_resource_textures(renderer, state->resources_phase, region_height,
                               state->top_margin, metrics);

    // Draw resource statistics overlay
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 200);

    // Active texture count indicator
    const int bar_x = 10;
    const int bar_y = (int)state->top_margin + region_height - 30;
    const int bar_max_width = 200;
    const int bar_width = (g_resource_manager.active_count * bar_max_width) / MAX_DYNAMIC_TEXTURES;

    SDL_Rect bar_bg = {bar_x, bar_y, bar_max_width, 8};
    SDL_SetRenderDrawColor(renderer, 64, 64, 64, 200);
    SDL_RenderFillRect(renderer, &bar_bg);

    SDL_Rect bar_fill = {bar_x, bar_y, bar_width, 8};
    SDL_SetRenderDrawColor(renderer, 0, 255, 128, 255);
    SDL_RenderFillRect(renderer, &bar_fill);

    // Memory usage indicator
    const int mem_bar_x = bar_x + bar_max_width + 20;
    const int mem_bar_height = 100;
    const float memory_ratio = (float)g_resource_manager.total_allocated_bytes /
                              (float)(MAX_DYNAMIC_TEXTURES * MAX_TEXTURE_SIZE * MAX_TEXTURE_SIZE * 4);
    const int mem_fill_height = (int)(memory_ratio * mem_bar_height);

    SDL_Rect mem_bg = {mem_bar_x, bar_y - mem_bar_height, 12, mem_bar_height};
    SDL_SetRenderDrawColor(renderer, 64, 64, 64, 200);
    SDL_RenderFillRect(renderer, &mem_bg);

    SDL_Rect mem_fill = {mem_bar_x, bar_y - mem_fill_height, 12, mem_fill_height};
    SDL_SetRenderDrawColor(renderer, 255, 128, 0, 255);
    SDL_RenderFillRect(renderer, &mem_fill);

    if (metrics) {
        metrics->draw_calls += 4;
        metrics->vertices_rendered += 16;
        metrics->triangles_rendered += 8;
    }
}
