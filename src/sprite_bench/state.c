#include "sprite_bench/state.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define SPRITE_PI 3.14159265358979323846f

static SDL_Texture *sprite_render_text(SDL_Renderer *renderer,
                                       TTF_Font *font,
                                       const char *text,
                                       SDL_Color color,
                                       int *out_w,
                                       int *out_h);

static void sprite_generate_pattern(Uint32 *pixels, int size, float phase, int pattern)
{
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const float fx = (float)x / (float)size;
            const float fy = (float)y / (float)size;
            Uint8 r = 0, g = 0, b = 0, a = 255;

            switch (pattern % 4) {
                case 0: // Gradient
                    r = (Uint8)(fx * 255.0f);
                    g = (Uint8)(fy * 255.0f);
                    b = (Uint8)(sinf(phase + fx * SPRITE_PI) * 128.0f + 127.0f);
                    break;
                case 1: { // Checkerboard
                    const int check_size = 4;
                    const int check_x = (x / check_size) % 2;
                    const int check_y = (y / check_size) % 2;
                    const Uint8 intensity = (check_x ^ check_y) ? 255 : 64;
                    r = intensity;
                    g = (Uint8)((float)intensity * sinf(phase) * 0.5f + (float)intensity * 0.5f);
                    b = (Uint8)((float)intensity * cosf(phase) * 0.5f + (float)intensity * 0.5f);
                    break;
                }
                case 2: { // Plasma
                    const float v1 = sinf(fx * 10.0f + phase);
                    const float v2 = sinf(fy * 10.0f + phase * 1.3f);
                    const float v3 = sinf((fx + fy) * 8.0f + phase * 0.8f);
                    const float intensity = (v1 + v2 + v3) / 3.0f;
                    r = (Uint8)((intensity + 1.0f) * 127.5f);
                    g = (Uint8)((sinf(intensity * SPRITE_PI + phase) + 1.0f) * 127.5f);
                    b = (Uint8)((cosf(intensity * SPRITE_PI + phase * 1.5f) + 1.0f) * 127.5f);
                    break;
                }
                case 3: { // Noise-like
                    const int seed = (x * 73 + y * 137 + (int)(phase * 100.0f)) % 255;
                    r = (Uint8)(seed % 256);
                    g = (Uint8)((seed * 17) % 256);
                    b = (Uint8)((seed * 31) % 256);
                    break;
                }
            }

            pixels[y * size + x] = ((Uint32)a << 24) | ((Uint32)r << 16) | ((Uint32)g << 8) | (Uint32)b;
        }
    }
}

static float sprite_randf(float min_val, float max_val)
{
    const float t = (float)rand() / (float)RAND_MAX;
    return min_val + t * (max_val - min_val);
}

static void sprite_spawn_instance(SpriteInstance *inst, int index)
{
    inst->x = sprite_randf(0.0f, (float)(bench_logical_w() - SPRITE_SIZE));
    inst->y = sprite_randf(0.0f, (float)(bench_logical_h() - SPRITE_SIZE));
    const float speed = sprite_randf(40.0f, 160.0f);
    const float angle = sprite_randf(0.0f, 2.0f * SPRITE_PI);
    inst->vx = cosf(angle) * speed;
    inst->vy = sinf(angle) * speed;
    inst->pool_index = index % SPRITE_POOL_SIZE;
}

SDL_bool sprite_state_init(SpriteBenchState *state, SDL_Renderer *renderer)
{
    SDL_zerop(state);

    for (int i = 0; i < SPRITE_POOL_SIZE; ++i) {
        SpritePoolSlot *slot = &state->pool[i];
        slot->texture = SDL_CreateTexture(renderer,
                                          SDL_PIXELFORMAT_RGBA8888,
                                          SDL_TEXTUREACCESS_STREAMING,
                                          SPRITE_SIZE,
                                          SPRITE_SIZE);
        if (!slot->texture) {
            printf("sprite_state_init: failed to create pool texture %d: %s\n", i, SDL_GetError());
            sprite_state_destroy(state);
            return SDL_FALSE;
        }
        slot->pattern = i % 4;
        slot->phase = sprite_randf(0.0f, 2.0f * SPRITE_PI);
        slot->phase_speed = sprite_randf(0.8f, 2.2f);
        slot->blend = (i % 2 == 0) ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_ADD;
        SDL_SetTextureBlendMode(slot->texture, slot->blend);
    }

    state->instance_capacity = SPRITE_COUNT_CAP;
    state->instances = (SpriteInstance *)malloc((size_t)state->instance_capacity * sizeof(SpriteInstance));
    if (!state->instances) {
        printf("sprite_state_init: failed to allocate instance array\n");
        sprite_state_destroy(state);
        return SDL_FALSE;
    }
    state->initialized_count = 0;

    state->sprite_count = 1;
    state->step_size = SPRITE_STEP_DEFAULT;
    state->interval_seconds = SPRITE_INTERVAL_DEFAULT;
    state->direction = 1;
    state->ramp_timer_seconds = 0.0;
    state->static_mode = SDL_FALSE;

    state->font = bench_load_font(14);
    state->text_refresh_timer = 0.0;

    if (state->font) {
        state->line_height = TTF_FontHeight(state->font) + 4;
        state->controls_texture = sprite_render_text(
            renderer, state->font,
            "UP/DOWN: Step  LEFT/RIGHT: Interval  A/B: Grow/Shrink  X: Static/Dynamic  SELECT: Exit",
            (SDL_Color){180, 200, 255, 255},
            &state->controls_texture_w, &state->controls_texture_h);
    }

    return SDL_TRUE;
}

void sprite_state_destroy(SpriteBenchState *state)
{
    if (!state) {
        return;
    }

    for (int i = 0; i < SPRITE_POOL_SIZE; ++i) {
        if (state->pool[i].texture) {
            SDL_DestroyTexture(state->pool[i].texture);
            state->pool[i].texture = NULL;
        }
    }

    if (state->instances) {
        free(state->instances);
        state->instances = NULL;
    }

    if (state->fps_texture) {
        SDL_DestroyTexture(state->fps_texture);
        state->fps_texture = NULL;
    }
    if (state->hint_texture) {
        SDL_DestroyTexture(state->hint_texture);
        state->hint_texture = NULL;
    }
    if (state->controls_texture) {
        SDL_DestroyTexture(state->controls_texture);
        state->controls_texture = NULL;
    }

    if (state->font) {
        TTF_CloseFont(state->font);
        state->font = NULL;
    }
}

void sprite_state_update_pool(SpriteBenchState *state, float delta_seconds)
{
    static Uint32 pixels[SPRITE_SIZE * SPRITE_SIZE];

    if (state->static_mode) {
        return;
    }

    for (int i = 0; i < SPRITE_POOL_SIZE; ++i) {
        SpritePoolSlot *slot = &state->pool[i];
        slot->phase += delta_seconds * slot->phase_speed;

        sprite_generate_pattern(pixels, SPRITE_SIZE, slot->phase, slot->pattern);
        SDL_UpdateTexture(slot->texture, NULL, pixels, SPRITE_SIZE * (int)sizeof(Uint32));
    }
}

void sprite_state_update_ramp(SpriteBenchState *state, double delta_seconds)
{
    state->ramp_timer_seconds += delta_seconds;
    if (state->ramp_timer_seconds < state->interval_seconds) {
        return;
    }
    state->ramp_timer_seconds -= state->interval_seconds;

    int next_count = state->sprite_count + state->direction * state->step_size;
    if (next_count < 1) {
        next_count = 1;
    } else if (next_count > SPRITE_COUNT_CAP) {
        next_count = SPRITE_COUNT_CAP;
    }
    state->sprite_count = next_count;
}

void sprite_state_update_instances(SpriteBenchState *state, float delta_seconds)
{
    if (state->sprite_count > state->initialized_count) {
        for (int i = state->initialized_count; i < state->sprite_count; ++i) {
            sprite_spawn_instance(&state->instances[i], i);
        }
        state->initialized_count = state->sprite_count;
    }

    for (int i = 0; i < state->sprite_count; ++i) {
        SpriteInstance *inst = &state->instances[i];
        inst->x += inst->vx * delta_seconds;
        inst->y += inst->vy * delta_seconds;

        if (inst->x < 0.0f) {
            inst->x = 0.0f;
            inst->vx = -inst->vx;
        } else if (inst->x > (float)(bench_logical_w() - SPRITE_SIZE)) {
            inst->x = (float)(bench_logical_w() - SPRITE_SIZE);
            inst->vx = -inst->vx;
        }

        if (inst->y < 0.0f) {
            inst->y = 0.0f;
            inst->vy = -inst->vy;
        } else if (inst->y > (float)(bench_logical_h() - SPRITE_SIZE)) {
            inst->y = (float)(bench_logical_h() - SPRITE_SIZE);
            inst->vy = -inst->vy;
        }
    }
}

void sprite_state_render(SpriteBenchState *state, SDL_Renderer *renderer, BenchMetrics *metrics)
{
    for (int i = 0; i < state->sprite_count; ++i) {
        const SpriteInstance *inst = &state->instances[i];
        const SpritePoolSlot *slot = &state->pool[inst->pool_index];

        const Uint8 r = (Uint8)(128 + 127 * sinf((float)inst->pool_index * 0.7f + inst->x * 0.01f));
        const Uint8 g = (Uint8)(128 + 127 * sinf((float)inst->pool_index * 1.3f + inst->y * 0.01f));
        const Uint8 b = (Uint8)(128 + 127 * cosf((float)inst->pool_index * 0.5f + inst->x * 0.01f));
        SDL_SetTextureColorMod(slot->texture, r, g, b);

        SDL_Rect dst = {(int)inst->x, (int)inst->y, SPRITE_SIZE, SPRITE_SIZE};
        SDL_RenderCopy(renderer, slot->texture, NULL, &dst);

        if (metrics) {
            metrics->draw_calls++;
            metrics->vertices_rendered += 4;
            metrics->triangles_rendered += 2;
        }
    }
}

static int sprite_status_band_top(const SpriteBenchState *state)
{
    const int row_height = (state->line_height > 0) ? state->line_height : 20;
    const int bottom_margin = 6;
    return bench_logical_h() - bottom_margin - row_height * 3;
}

void sprite_state_render_status_bg(SpriteBenchState *state, SDL_Renderer *renderer, BenchMetrics *metrics)
{
    const int band_top = sprite_status_band_top(state);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
    SDL_Rect band = {0, band_top - 4, bench_logical_w(), bench_logical_h() - band_top + 4};
    SDL_RenderFillRect(renderer, &band);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    if (metrics) {
        metrics->draw_calls++;
    }
}

static SDL_Texture *sprite_render_text(SDL_Renderer *renderer,
                                       TTF_Font *font,
                                       const char *text,
                                       SDL_Color color,
                                       int *out_w,
                                       int *out_h)
{
    if (!font) {
        return NULL;
    }
    SDL_Surface *surface = TTF_RenderUTF8_Blended(font, text, color);
    if (!surface) {
        return NULL;
    }
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (out_w) {
        *out_w = surface->w;
    }
    if (out_h) {
        *out_h = surface->h;
    }
    SDL_FreeSurface(surface);
    return texture;
}

void sprite_state_render_text(SpriteBenchState *state,
                              SDL_Renderer *renderer,
                              BenchMetrics *metrics,
                              double delta_seconds)
{
    if (!state->font) {
        return;
    }

    state->text_refresh_timer += delta_seconds;
    if (state->text_refresh_timer >= 0.2) {
        state->text_refresh_timer = 0.0;

        char fps_line[64];
        snprintf(fps_line, sizeof(fps_line), "%.1f FPS / %.2fms",
                 metrics->current_fps, metrics->frame_time_ms);

        if (state->fps_texture) {
            SDL_DestroyTexture(state->fps_texture);
            state->fps_texture = NULL;
        }
        state->fps_texture = sprite_render_text(renderer, state->font, fps_line,
                                                (SDL_Color){0, 255, 160, 255},
                                                &state->fps_texture_w, &state->fps_texture_h);

        char hint_line[160];
        snprintf(hint_line, sizeof(hint_line),
                 "Sprites: %d | Step: %d | Interval: %.2fs | Dir: %s | Mode: %s",
                 state->sprite_count, state->step_size, state->interval_seconds,
                 state->direction > 0 ? "+" : "-",
                 state->static_mode ? "Static" : "Dynamic");

        if (state->hint_texture) {
            SDL_DestroyTexture(state->hint_texture);
            state->hint_texture = NULL;
        }
        state->hint_texture = sprite_render_text(renderer, state->font, hint_line,
                                                 (SDL_Color){255, 200, 0, 255},
                                                 &state->hint_texture_w, &state->hint_texture_h);
    }

    /* Three stacked rows across the bottom, bottom-most first, so none of
     * them can clip into each other regardless of text length. */
    const int row_height = (state->line_height > 0) ? state->line_height : 20;
    const int row0_y = sprite_status_band_top(state);
    const int row1_y = row0_y + row_height;
    const int row2_y = row1_y + row_height;

    if (state->controls_texture) {
        SDL_Rect dst = {8, row0_y, state->controls_texture_w, state->controls_texture_h};
        SDL_RenderCopy(renderer, state->controls_texture, NULL, &dst);
        if (metrics) {
            metrics->draw_calls++;
        }
    }

    if (state->fps_texture) {
        SDL_Rect dst = {8, row1_y, state->fps_texture_w, state->fps_texture_h};
        SDL_RenderCopy(renderer, state->fps_texture, NULL, &dst);
        if (metrics) {
            metrics->draw_calls++;
        }
    }

    if (state->hint_texture) {
        SDL_Rect dst = {8, row2_y, state->hint_texture_w, state->hint_texture_h};
        SDL_RenderCopy(renderer, state->hint_texture, NULL, &dst);
        if (metrics) {
            metrics->draw_calls++;
        }
    }
}
