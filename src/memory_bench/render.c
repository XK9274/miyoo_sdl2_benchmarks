#include "memory_bench/render.h"
#include "common/memory_opt.h"
#include "common/render_neon.h"

#include <stdlib.h>
#include <math.h>
#include <string.h>

#define MEMORY_BENCH_PI 3.14159265358979323846f
#define MAX_DYNAMIC_TEXTURES 50
#define MIN_TEXTURE_SIZE 16
#define MAX_TEXTURE_SIZE 128
#define MEMORY_CHECKER_SIZE 8

typedef struct {
    SDL_Texture *texture;
    Uint32 *pixel_cache;
    size_t pixel_capacity;
    Uint32 *scratch_buffer;
    size_t scratch_capacity;
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
    int pool_update_cursor;
    Uint64 total_allocated_bytes;
    Uint64 peak_allocated_bytes;
    double total_allocation_time_ms;
    int allocation_count;
    int deallocation_count;
} ResourceManager;

static ResourceManager g_resource_manager = {0};

static const char *g_memory_pattern_names[MEMORY_PATTERN_MAX] = {
    "Gradient", "Checkerboard", "Plasma", "Noise"
};

const char *memory_render_pattern_name(int mode)
{
    if (mode < 0 || mode >= MEMORY_PATTERN_MAX) return "?";
    return g_memory_pattern_names[mode];
}

static const char *g_memory_alloc_names[MEMORY_ALLOC_MAX] = {
    "TexOnly", "Mixed", "MallocHeavy"
};

const char *memory_render_alloc_name(int mode)
{
    if (mode < 0 || mode >= MEMORY_ALLOC_MAX) return "?";
    return g_memory_alloc_names[mode];
}

static void memory_upload_texture_rows(void *dst, int dst_pitch, const Uint32 *src,
                                       int width, int height, SDL_bool use_neon)
{
    Uint8 *dst_row = (Uint8 *)dst;
    int row;

    for (row = 0; row < height; ++row) {
        if (use_neon) {
            bench_neon_copy_u32((uint32_t *)dst_row, src, (size_t)width);
        } else {
            rs_memcpy(dst_row, src, (size_t)width * sizeof(Uint32));
        }
        dst_row += dst_pitch;
        src += width;
    }
}

static inline float memory_clampf(float value, float min_val, float max_val)
{
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

static inline int memory_clampi(int value, int min_val, int max_val)
{
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

static Uint32 memory_calculate_texture_bytes(int width, int height, Uint32 format)
{
    int bytes_per_pixel = 4;
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

static void memory_generate_gradient(Uint32 *pixels, int width, int height, float phase)
{
    for (int y = 0; y < height; y++) {
        float fy = (float)y / (float)height;
        for (int x = 0; x < width; x++) {
            float fx = (float)x / (float)width;
            Uint8 r = (Uint8)(fx * 255);
            Uint8 g = (Uint8)(fy * 255);
            Uint8 b = (Uint8)(sinf(phase + fx * MEMORY_BENCH_PI) * 128 + 127);
            pixels[y * width + x] = (255u << 24) | ((Uint32)r << 16) | ((Uint32)g << 8) | b;
        }
    }
}

static void memory_generate_checkerboard(Uint32 *pixels, int width, int height,
                                         float phase, SDL_bool use_neon)
{
    for (int by = 0; by < height; by += MEMORY_CHECKER_SIZE) {
        const int block_h = SDL_min(MEMORY_CHECKER_SIZE, height - by);
        const int check_y = (by / MEMORY_CHECKER_SIZE) % 2;

        for (int bx = 0; bx < width; bx += MEMORY_CHECKER_SIZE) {
            const int block_w = SDL_min(MEMORY_CHECKER_SIZE, width - bx);
            const int check_x = (bx / MEMORY_CHECKER_SIZE) % 2;
            const Uint8 intensity = (check_x ^ check_y) ? 255 : 64;
            const Uint8 g = (Uint8)(intensity * sinf(phase) * 0.5f + intensity * 0.5f);
            const Uint8 b = (Uint8)(intensity * cosf(phase) * 0.5f + intensity * 0.5f);
            const Uint32 packed = (255u << 24) | ((Uint32)intensity << 16) | ((Uint32)g << 8) | b;

            for (int row = 0; row < block_h; row++) {
                Uint32 *dst = pixels + (size_t)(by + row) * width + bx;
                if (use_neon) {
                    bench_neon_fill_u32(dst, packed, (size_t)block_w);
                } else {
                    for (int col = 0; col < block_w; col++) {
                        dst[col] = packed;
                    }
                }
            }
        }
    }
}

static void memory_generate_plasma(Uint32 *pixels, int width, int height, float phase)
{
    for (int y = 0; y < height; y++) {
        float fy = (float)y / (float)height;
        for (int x = 0; x < width; x++) {
            float fx = (float)x / (float)width;
            float v1 = sinf(fx * 10.0f + phase);
            float v2 = sinf(fy * 10.0f + phase * 1.3f);
            float v3 = sinf((fx + fy) * 8.0f + phase * 0.8f);
            float intensity = (v1 + v2 + v3) / 3.0f;
            Uint8 r = (Uint8)((intensity + 1.0f) * 127.5f);
            Uint8 g = (Uint8)((sinf(intensity * MEMORY_BENCH_PI + phase) + 1.0f) * 127.5f);
            Uint8 b = (Uint8)((cosf(intensity * MEMORY_BENCH_PI + phase * 1.5f) + 1.0f) * 127.5f);
            pixels[y * width + x] = (255u << 24) | ((Uint32)r << 16) | ((Uint32)g << 8) | b;
        }
    }
}

static void memory_generate_noise_like(Uint32 *pixels, int width, int height, float phase)
{
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int seed = (x * 73 + y * 137 + (int)(phase * 100)) % 255;
            Uint8 r = (Uint8)(seed % 256);
            Uint8 g = (Uint8)((seed * 17) % 256);
            Uint8 b = (Uint8)((seed * 31) % 256);
            pixels[y * width + x] = (255u << 24) | ((Uint32)r << 16) | ((Uint32)g << 8) | b;
        }
    }
}

static void memory_generate_pattern(Uint32 *pixels, int width, int height, float phase,
                                    int pattern_mode, SDL_bool use_neon)
{
    switch (pattern_mode) {
        case MEMORY_PATTERN_GRADIENT:
            memory_generate_gradient(pixels, width, height, phase);
            break;
        case MEMORY_PATTERN_CHECKERBOARD:
            memory_generate_checkerboard(pixels, width, height, phase, use_neon);
            break;
        case MEMORY_PATTERN_PLASMA:
            memory_generate_plasma(pixels, width, height, phase);
            break;
        case MEMORY_PATTERN_NOISE:
            memory_generate_noise_like(pixels, width, height, phase);
            break;
        default:
            break;
    }
}

static SDL_bool memory_create_dynamic_texture(ResourceTexture *res,
                                              SDL_Renderer *renderer,
                                              int width,
                                              int height,
                                              float phase,
                                              int pattern_mode,
                                              int alloc_mode,
                                              int slot_index,
                                              SDL_bool use_neon,
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

    res->scratch_buffer = NULL;
    res->scratch_capacity = 0;
    const SDL_bool wants_scratch = (alloc_mode == MEMORY_ALLOC_MALLOC_HEAVY) ||
                                   (alloc_mode == MEMORY_ALLOC_MIXED && (slot_index % 2) != 0);
    if (wants_scratch) {
        res->scratch_capacity = pixel_count * 4;
        res->scratch_buffer = malloc(sizeof(Uint32) * res->scratch_capacity);
        if (!res->scratch_buffer) {
            res->scratch_capacity = 0;
        }
    }

    memory_generate_pattern(res->pixel_cache, width, height, phase, pattern_mode, use_neon);

    void *texture_pixels;
    int pitch;
    if (SDL_LockTexture(res->texture, NULL, &texture_pixels, &pitch) == 0) {
        memory_upload_texture_rows(texture_pixels, pitch, res->pixel_cache, width, height, use_neon);
        SDL_UnlockTexture(res->texture);
    }

    Uint64 end_time = SDL_GetPerformanceCounter();
    if (metrics) {
        double allocation_time = (double)(end_time - start_time) /
                               (double)SDL_GetPerformanceFrequency() * 1000.0;
        metrics->allocation_time_ms += allocation_time;
        metrics->resource_allocations++;

        Uint32 texture_bytes = memory_calculate_texture_bytes(width, height, SDL_PIXELFORMAT_RGBA8888);
        Uint32 scratch_bytes = (Uint32)(res->scratch_capacity * sizeof(Uint32));
        Uint32 total_bytes = texture_bytes + scratch_bytes;
        metrics->memory_allocated_bytes += total_bytes;
        if (metrics->memory_allocated_bytes > metrics->memory_peak_bytes) {
            metrics->memory_peak_bytes = metrics->memory_allocated_bytes;
        }

        g_resource_manager.total_allocation_time_ms += allocation_time;
        g_resource_manager.total_allocated_bytes += total_bytes;
        g_resource_manager.allocation_count++;

        if (g_resource_manager.total_allocated_bytes > g_resource_manager.peak_allocated_bytes) {
            g_resource_manager.peak_allocated_bytes = g_resource_manager.total_allocated_bytes;
        }
    }

    return SDL_TRUE;
}

static void memory_destroy_resource_texture(ResourceTexture *res, BenchMetrics *metrics)
{
    if (!res || !res->texture) {
        return;
    }

    if (metrics) {
        Uint32 texture_bytes = memory_calculate_texture_bytes(res->width, res->height,
                                                              SDL_PIXELFORMAT_RGBA8888);
        Uint32 scratch_bytes = (Uint32)(res->scratch_capacity * sizeof(Uint32));
        Uint32 total_bytes = texture_bytes + scratch_bytes;
        metrics->memory_allocated_bytes -= total_bytes;
        metrics->resource_deallocations++;

        g_resource_manager.total_allocated_bytes -= total_bytes;
        g_resource_manager.deallocation_count++;
    }

    SDL_DestroyTexture(res->texture);
    res->texture = NULL;
    if (res->pixel_cache) {
        free(res->pixel_cache);
        res->pixel_cache = NULL;
    }
    res->pixel_capacity = 0;
    if (res->scratch_buffer) {
        free(res->scratch_buffer);
        res->scratch_buffer = NULL;
    }
    res->scratch_capacity = 0;
    res->in_use = SDL_FALSE;
    res->dirty = SDL_FALSE;
}

static void memory_update_resource_pool(SDL_Renderer *renderer, float stress_factor,
                                        float phase, float delta_seconds,
                                        int pattern_mode, int alloc_mode, SDL_bool use_neon,
                                        BenchMetrics *metrics)
{
    const int min_textures = 5;
    const int max_textures = memory_clampi((int)(min_textures + stress_factor * 30), min_textures, MAX_DYNAMIC_TEXTURES);

    for (int i = 0; i < MAX_DYNAMIC_TEXTURES; i++) {
        ResourceTexture *res = &g_resource_manager.textures[i];
        if (res->in_use) {
            res->life_remaining -= delta_seconds;
            if (res->life_remaining <= 0.0f) {
                memory_destroy_resource_texture(res, metrics);
                g_resource_manager.active_count--;
            }
        }
    }

    while (g_resource_manager.active_count < max_textures) {
        int slot = -1;
        for (int i = 0; i < MAX_DYNAMIC_TEXTURES; i++) {
            if (!g_resource_manager.textures[i].in_use) {
                slot = i;
                break;
            }
        }

        if (slot == -1) {
            break;
        }

        int width = MIN_TEXTURE_SIZE + (rand() % (MAX_TEXTURE_SIZE - MIN_TEXTURE_SIZE));
        int height = MIN_TEXTURE_SIZE + (rand() % (MAX_TEXTURE_SIZE - MIN_TEXTURE_SIZE));

        float lifetime = 1.0f + (float)rand() / RAND_MAX * (3.0f + stress_factor * 2.0f);

        ResourceTexture *res = &g_resource_manager.textures[slot];
        if (memory_create_dynamic_texture(res, renderer, width, height, phase,
                                          pattern_mode, alloc_mode, slot, use_neon, metrics)) {
            res->width = width;
            res->height = height;
            res->format = SDL_PIXELFORMAT_RGBA8888;
            res->allocation_time = SDL_GetPerformanceCounter();
            res->life_remaining = lifetime;
            res->in_use = SDL_TRUE;
            res->dirty = SDL_FALSE;
            g_resource_manager.active_count++;
        } else {
            break;
        }
    }
}

static void memory_render_resource_textures(SDL_Renderer *renderer, float phase,
                                            int region_height, float top_margin,
                                            BenchMetrics *metrics)
{
    const int columns = 5;
    const int rows = 4;
    const float cell_width = (float)bench_logical_w() / (float)columns;
    const float cell_height = (float)region_height / (float)rows;

    int texture_index = 0;
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < columns; col++) {
            while (texture_index < MAX_DYNAMIC_TEXTURES &&
                   !g_resource_manager.textures[texture_index].in_use) {
                texture_index++;
            }

            if (texture_index >= MAX_DYNAMIC_TEXTURES) {
                return;
            }

            ResourceTexture *res = &g_resource_manager.textures[texture_index];
            if (!res->texture) {
                texture_index++;
                continue;
            }

            float x = col * cell_width + sinf(phase + (float)texture_index * 0.1f) * 5.0f;
            float y = top_margin + row * cell_height + cosf(phase + (float)texture_index * 0.15f) * 3.0f;

            float scale = memory_clampf(res->life_remaining, 0.1f, 1.0f);
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

static void memory_touch_scratch_buffers(float phase, SDL_bool use_neon)
{
    const Uint32 fill_value = (Uint32)(phase * 1000.0f);

    for (int i = 0; i < MAX_DYNAMIC_TEXTURES; i++) {
        ResourceTexture *res = &g_resource_manager.textures[i];
        if (!res->in_use || !res->scratch_buffer || res->scratch_capacity == 0) {
            continue;
        }

        if (use_neon) {
            bench_neon_fill_u32(res->scratch_buffer, fill_value ^ (Uint32)i, res->scratch_capacity);
        } else {
            for (size_t j = 0; j < res->scratch_capacity; j++) {
                res->scratch_buffer[j] = fill_value ^ (Uint32)i;
            }
        }
    }
}

void memory_render_init(MemoryBenchState *state, SDL_Renderer *renderer)
{
    (void)state;
    (void)renderer;
    memset(&g_resource_manager, 0, sizeof(g_resource_manager));
}

void memory_render_cleanup(MemoryBenchState *state)
{
    (void)state;
    for (int i = 0; i < MAX_DYNAMIC_TEXTURES; i++) {
        if (g_resource_manager.textures[i].in_use) {
            memory_destroy_resource_texture(&g_resource_manager.textures[i], NULL);
        }
    }
    memset(&g_resource_manager, 0, sizeof(g_resource_manager));
}

void memory_render_scene(MemoryBenchState *state,
                        SDL_Renderer *renderer,
                        BenchMetrics *metrics,
                        double delta_seconds)
{
    if (!state || !renderer) {
        return;
    }

    const float factor = memory_state_stress_factor(state);
    const int region_height = SDL_max(1, bench_logical_h() - (int)state->top_margin);

    state->resources_phase += (float)(delta_seconds * (1.0f + factor * 2.0f));
    state->mode_phase_seconds += (float)delta_seconds;

    const float mode_cycle_seconds = 3.0f;
    const int auto_pattern_mode = (int)(state->mode_phase_seconds / mode_cycle_seconds) % MEMORY_PATTERN_MAX;
    state->current_pattern_mode = (state->forced_pattern_mode >= 0) ? state->forced_pattern_mode : auto_pattern_mode;

    const int auto_alloc_mode = (int)((state->mode_phase_seconds + mode_cycle_seconds * 0.5f) / mode_cycle_seconds) % MEMORY_ALLOC_MAX;
    state->current_alloc_mode = (state->forced_alloc_mode >= 0) ? state->forced_alloc_mode : auto_alloc_mode;

    memory_update_resource_pool(renderer, factor, state->resources_phase, (float)delta_seconds,
                                state->current_pattern_mode, state->current_alloc_mode,
                                state->neon_enabled, metrics);

    const int updates_per_frame = memory_clampi((int)(1 + factor * 2), 1, 3);
    for (int i = 0; i < updates_per_frame; i++) {
        int update_index = (g_resource_manager.pool_update_cursor + i) % MAX_DYNAMIC_TEXTURES;
        ResourceTexture *res = &g_resource_manager.textures[update_index];

        if (res->in_use && res->texture && res->pixel_cache) {
            Uint64 start_time = SDL_GetPerformanceCounter();

            void *pixels;
            int pitch;
            if (SDL_LockTexture(res->texture, NULL, &pixels, &pitch) == 0) {
                memory_generate_pattern(res->pixel_cache,
                                        res->width,
                                        res->height,
                                        state->resources_phase + (float)i,
                                        state->current_pattern_mode,
                                        state->neon_enabled);
                memory_upload_texture_rows(pixels, pitch, res->pixel_cache,
                                           res->width, res->height, state->neon_enabled);
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
    g_resource_manager.pool_update_cursor = (g_resource_manager.pool_update_cursor + updates_per_frame) % MAX_DYNAMIC_TEXTURES;

    memory_touch_scratch_buffers(state->resources_phase, state->neon_enabled);

    memory_render_resource_textures(renderer, state->resources_phase, region_height,
                                    state->top_margin, metrics);
}
