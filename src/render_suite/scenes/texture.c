#include "render_suite/scenes/texture.h"

#include <math.h>
#include <stdlib.h>

#define RS_TEXTURE_PI 3.14159265358979323846f
#define RS_TEXTURE_POOL_SIZE 6
#define RS_TEXTURE_TEXEL_SIZE 40
#define RS_TEXTURE_INSTANCE_SIZE 40
#define RS_TEXTURE_MAX_INSTANCES 256

typedef struct {
    SDL_Texture *texture;
    int pattern;
    float phase;
    float phase_speed;
    SDL_BlendMode blend;
} RSTexturePoolSlot;

typedef struct {
    float x, y;
    float vx, vy;
    float rotation;
    float rotation_speed;
    int pool_index;
} RSTextureInstance;

static RSTexturePoolSlot g_texture_pool[RS_TEXTURE_POOL_SIZE];
static RSTextureInstance g_texture_instances[RS_TEXTURE_MAX_INSTANCES];
static int g_texture_active_count = 0;
static SDL_bool g_texture_initialized = SDL_FALSE;

static void rs_texture_generate_pattern(Uint32 *pixels, int size, float phase, int pattern)
{
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const float fx = (float)x / (float)size;
            const float fy = (float)y / (float)size;
            Uint8 r = 0, g = 0, b = 0;

            switch (pattern % 4) {
                case 0:
                    r = (Uint8)(fx * 255.0f);
                    g = (Uint8)(fy * 255.0f);
                    b = (Uint8)(sinf(phase + fx * RS_TEXTURE_PI) * 128.0f + 127.0f);
                    break;
                case 1: {
                    const int check_size = 5;
                    const int check_x = (x / check_size) % 2;
                    const int check_y = (y / check_size) % 2;
                    const Uint8 intensity = (check_x ^ check_y) ? 255 : 64;
                    r = intensity;
                    g = (Uint8)((float)intensity * (sinf(phase) * 0.5f + 0.5f));
                    b = (Uint8)((float)intensity * (cosf(phase) * 0.5f + 0.5f));
                    break;
                }
                case 2: {
                    const float v1 = sinf(fx * 9.0f + phase);
                    const float v2 = sinf(fy * 9.0f + phase * 1.3f);
                    const float v3 = sinf((fx + fy) * 7.0f + phase * 0.8f);
                    const float intensity = (v1 + v2 + v3) / 3.0f;
                    r = (Uint8)((intensity + 1.0f) * 127.5f);
                    g = (Uint8)((sinf(intensity * RS_TEXTURE_PI + phase) + 1.0f) * 127.5f);
                    b = (Uint8)((cosf(intensity * RS_TEXTURE_PI + phase * 1.5f) + 1.0f) * 127.5f);
                    break;
                }
                default: {
                    const int seed = (x * 71 + y * 131 + (int)(phase * 97.0f)) % 255;
                    r = (Uint8)(seed % 256);
                    g = (Uint8)((seed * 19) % 256);
                    b = (Uint8)((seed * 29) % 256);
                    break;
                }
            }

            pixels[y * size + x] = (0xFFu << 24) | ((Uint32)r << 16) | ((Uint32)g << 8) | (Uint32)b;
        }
    }
}

static float rs_texture_randf(float min_val, float max_val)
{
    const float t = (float)rand() / (float)RAND_MAX;
    return min_val + t * (max_val - min_val);
}

static void rs_texture_spawn_instance(RSTextureInstance *inst, int index, float top_margin)
{
    const float max_x = (float)(bench_logical_w() - RS_TEXTURE_INSTANCE_SIZE);
    const float max_y = (float)bench_logical_h() - RS_TEXTURE_INSTANCE_SIZE;
    inst->x = rs_texture_randf(0.0f, SDL_max(1.0f, max_x));
    inst->y = rs_texture_randf(top_margin, SDL_max(top_margin + 1.0f, max_y));
    const float speed = rs_texture_randf(40.0f, 140.0f);
    const float angle = rs_texture_randf(0.0f, 2.0f * RS_TEXTURE_PI);
    inst->vx = cosf(angle) * speed;
    inst->vy = sinf(angle) * speed;
    inst->rotation = rs_texture_randf(0.0f, 360.0f);
    inst->rotation_speed = rs_texture_randf(-90.0f, 90.0f);
    inst->pool_index = index % RS_TEXTURE_POOL_SIZE;
}

void rs_scene_texture_init(RenderSuiteState *state, SDL_Renderer *renderer)
{
    for (int i = 0; i < RS_TEXTURE_POOL_SIZE; ++i) {
        RSTexturePoolSlot *slot = &g_texture_pool[i];
        slot->texture = SDL_CreateTexture(renderer,
                                          SDL_PIXELFORMAT_RGBA8888,
                                          SDL_TEXTUREACCESS_STREAMING,
                                          RS_TEXTURE_TEXEL_SIZE,
                                          RS_TEXTURE_TEXEL_SIZE);
        slot->pattern = i % 4;
        slot->phase = rs_texture_randf(0.0f, 2.0f * RS_TEXTURE_PI);
        slot->phase_speed = rs_texture_randf(0.8f, 2.2f);
        slot->blend = (i % 2 == 0) ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_ADD;
        if (slot->texture) {
            SDL_SetTextureBlendMode(slot->texture, slot->blend);
        }
    }

    const float top_margin = state ? state->top_margin : 0.0f;
    for (int i = 0; i < RS_TEXTURE_MAX_INSTANCES; ++i) {
        rs_texture_spawn_instance(&g_texture_instances[i], i, top_margin);
    }

    g_texture_active_count = 0;
    g_texture_initialized = SDL_TRUE;
}

void rs_scene_texture_cleanup(RenderSuiteState *state)
{
    (void)state;
    for (int i = 0; i < RS_TEXTURE_POOL_SIZE; ++i) {
        if (g_texture_pool[i].texture) {
            SDL_DestroyTexture(g_texture_pool[i].texture);
            g_texture_pool[i].texture = NULL;
        }
    }
    g_texture_active_count = 0;
    g_texture_initialized = SDL_FALSE;
}

int rs_scene_texture_instance_count(void)
{
    return g_texture_active_count;
}

void rs_scene_texture(RenderSuiteState *state,
                      SDL_Renderer *renderer,
                      BenchMetrics *metrics,
                      double delta_seconds)
{
    if (!state || !renderer || !g_texture_initialized) {
        return;
    }

    const Uint64 perf_freq = SDL_GetPerformanceFrequency();
    const Uint64 transform_start = SDL_GetPerformanceCounter();

    const float factor = rs_state_stress_factor(state);
    int target_count = (int)(24.0f * factor);
    if (target_count < 16) {
        target_count = 16;
    } else if (target_count > RS_TEXTURE_MAX_INSTANCES) {
        target_count = RS_TEXTURE_MAX_INSTANCES;
    }
    g_texture_active_count = target_count;

    static Uint32 pixels[RS_TEXTURE_TEXEL_SIZE * RS_TEXTURE_TEXEL_SIZE];
    if (state->texture_streaming) {
        for (int i = 0; i < RS_TEXTURE_POOL_SIZE; ++i) {
            RSTexturePoolSlot *slot = &g_texture_pool[i];
            slot->phase += (float)delta_seconds * slot->phase_speed;
            rs_texture_generate_pattern(pixels, RS_TEXTURE_TEXEL_SIZE, slot->phase, slot->pattern);
            SDL_UpdateTexture(slot->texture, NULL, pixels, RS_TEXTURE_TEXEL_SIZE * (int)sizeof(Uint32));
        }
    }

    const float max_x = (float)(bench_logical_w() - RS_TEXTURE_INSTANCE_SIZE);
    const float max_y = (float)bench_logical_h() - RS_TEXTURE_INSTANCE_SIZE;

    for (int i = 0; i < g_texture_active_count; ++i) {
        RSTextureInstance *inst = &g_texture_instances[i];
        inst->x += inst->vx * (float)delta_seconds;
        inst->y += inst->vy * (float)delta_seconds;

        if (inst->x < 0.0f) {
            inst->x = 0.0f;
            inst->vx = -inst->vx;
        } else if (inst->x > max_x) {
            inst->x = max_x;
            inst->vx = -inst->vx;
        }
        if (inst->y < state->top_margin) {
            inst->y = state->top_margin;
            inst->vy = -inst->vy;
        } else if (inst->y > max_y) {
            inst->y = max_y;
            inst->vy = -inst->vy;
        }

        if (state->texture_transform_variety) {
            inst->rotation += inst->rotation_speed * (float)delta_seconds;
        }
    }

    const Uint64 transform_end = SDL_GetPerformanceCounter();
    metrics->stage_transform_ms = (double)(transform_end - transform_start) * 1000.0 / (double)perf_freq;

    const Uint64 draw_start = transform_end;
    for (int i = 0; i < g_texture_active_count; ++i) {
        const RSTextureInstance *inst = &g_texture_instances[i];
        RSTexturePoolSlot *slot = &g_texture_pool[inst->pool_index];
        const SDL_BlendMode blend = state->texture_blend_variety ? slot->blend : SDL_BLENDMODE_BLEND;
        SDL_SetTextureBlendMode(slot->texture, blend);

        if (state->texture_transform_variety) {
            const float scale = 0.85f + 0.30f * sinf(inst->rotation * 0.5f * (RS_TEXTURE_PI / 180.0f));
            SDL_FRect dest = {
                inst->x - (RS_TEXTURE_INSTANCE_SIZE * (scale - 1.0f)) * 0.5f,
                inst->y - (RS_TEXTURE_INSTANCE_SIZE * (scale - 1.0f)) * 0.5f,
                RS_TEXTURE_INSTANCE_SIZE * scale,
                RS_TEXTURE_INSTANCE_SIZE * scale
            };
            SDL_RenderCopyExF(renderer, slot->texture, NULL, &dest, inst->rotation, NULL, SDL_FLIP_NONE);
        } else {
            SDL_Rect dest = {(int)inst->x, (int)inst->y, RS_TEXTURE_INSTANCE_SIZE, RS_TEXTURE_INSTANCE_SIZE};
            SDL_RenderCopy(renderer, slot->texture, NULL, &dest);
        }

        if (metrics) {
            metrics->draw_calls++;
            metrics->vertices_rendered += 4;
            metrics->triangles_rendered += 2;
            metrics->texture_switches++;
        }
    }

    metrics->stage_draw_ms = (double)(SDL_GetPerformanceCounter() - draw_start) * 1000.0 / (double)perf_freq;
}
