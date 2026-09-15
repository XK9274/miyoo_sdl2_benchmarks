#include "title/fireflies.h"

#include <stdlib.h>

#include <SDL2/SDL_opengles2.h>

#include "common/driver_support.h"
#include "common/types.h"
#include "title/battery_icon.h"
#include "title/statusbar.h"

/* Tiny sprite rendered once at init and upscaled for a smooth glow; per-frame animation is pure SDL texture modulation, no GL. */
#define TITLE_FIREFLIES_GL_W 15
#define TITLE_FIREFLIES_GL_H 15
#define TITLE_FIREFLIES_DRAW_SIZE 24
#define TITLE_FIREFLIES_MARGIN 24.0f

#define TITLE_FIREFLIES_POP_DURATION_S 0.5f
#define TITLE_FIREFLIES_RIPPLE_SPREAD_S 0.12f
#define TITLE_FIREFLIES_FADE_DURATION_S 1.5f
#define TITLE_FIREFLIES_RING_DURATION_S 0.18f /* short burst, independent of the slower 500ms colour blend */

/* Approximates the header battery icon's screen position -- the actual
 * position depends on live text widths computed at render time. Close
 * enough for a subtle ripple stagger. */
#define TITLE_FIREFLIES_CHARGE_ORIGIN_X ((float)BENCH_NATIVE_W - 55.0f)
#define TITLE_FIREFLIES_CHARGE_ORIGIN_Y (TITLE_STATUSBAR_HEADER_HEIGHT / 2.0f)

static const char *g_firefly_sprite_fragment_src =
    "precision mediump float;\n"
    "varying vec2 v_uv;\n"
    "void main() {\n"
    "    float d = length(v_uv - vec2(0.5)) * 2.0;\n"
    "    float glow = smoothstep(1.0, 0.0, d);\n"
    "    gl_FragColor = vec4(vec3(1.0), glow * glow);\n"
    "}\n";

void title_fireflies_init(TitleFireflies *fx, SDL_Renderer *renderer)
{
    if (!fx) {
        return;
    }
    SDL_zerop(fx);

    for (int i = 0; i < TITLE_FIREFLY_COUNT; i++) {
        fx->flies[i].x = (float)(rand() % BENCH_NATIVE_W);
        fx->flies[i].y = (float)(rand() % BENCH_NATIVE_H);
        const float angle = (float)(rand() % 360) * 0.0174533f;
        const float speed = 6.0f + (float)(rand() % 10);
        fx->flies[i].vx = SDL_cosf(angle) * speed;
        fx->flies[i].vy = SDL_sinf(angle) * speed;
        fx->flies[i].phase = (float)(rand() % 628) / 100.0f;
        fx->flies[i].hue_mix = (float)(rand() % 100) / 100.0f;
        fx->flies[i].far = (i % 2) == 0;
        fx->flies[i].pop_elapsed = -1.0f;
    }

    if (!renderer || !gl_effect_context_acquire()) {
        return;
    }
    if (!gl_effect_target_create(&fx->target, renderer, TITLE_FIREFLIES_GL_W, TITLE_FIREFLIES_GL_H)) {
        gl_effect_context_release();
        return;
    }

    fx->program = gl_effect_compile_program(g_firefly_sprite_fragment_src);
    if (!fx->program) {
        gl_effect_target_destroy(&fx->target);
        gl_effect_context_release();
        return;
    }

    /* Render the sprite shape exactly once -- static, no uniforms needed. */
    gl_effect_render(&fx->target, fx->program, NULL, NULL);
    SDL_SetTextureBlendMode(fx->target.screen_texture, SDL_BLENDMODE_ADD);

    fx->ready = SDL_TRUE;
}

void title_fireflies_shutdown(TitleFireflies *fx)
{
    if (!fx || !fx->ready) {
        return;
    }
    gl_effect_destroy_program(fx->program);
    fx->program = 0;
    gl_effect_target_destroy(&fx->target);
    gl_effect_context_release();
    fx->ready = SDL_FALSE;
}

void title_fireflies_update(TitleFireflies *fx, float dt)
{
    if (!fx) {
        return;
    }

    const float min_x = -TITLE_FIREFLIES_MARGIN;
    const float max_x = (float)BENCH_NATIVE_W + TITLE_FIREFLIES_MARGIN;
    const float min_y = -TITLE_FIREFLIES_MARGIN;
    const float max_y = (float)BENCH_NATIVE_H + TITLE_FIREFLIES_MARGIN;
    const float span_x = max_x - min_x;
    const float span_y = max_y - min_y;

    BenchDriverStatus status;
    bench_driver_get_status(&status);

    const SDL_bool rising_edge = status.charging && !fx->was_charging;
    const SDL_bool falling_edge = !status.charging && fx->was_charging;
    const float diagonal = SDL_sqrtf((float)(BENCH_NATIVE_W * BENCH_NATIVE_W + BENCH_NATIVE_H * BENCH_NATIVE_H));

    for (int i = 0; i < TITLE_FIREFLY_COUNT; i++) {
        TitleFirefly *fly = &fx->flies[i];

        const float dx = fly->x - TITLE_FIREFLIES_CHARGE_ORIGIN_X;
        const float dy = fly->y - TITLE_FIREFLIES_CHARGE_ORIGIN_Y;
        const float dist = SDL_sqrtf(dx * dx + dy * dy);

        if (rising_edge) {
            fly->pop_delay = (dist / diagonal) * TITLE_FIREFLIES_RIPPLE_SPREAD_S;
            fly->pop_elapsed = 0.0f;
            fly->fading = SDL_FALSE;
        } else if (falling_edge) {
            if (fly->pop_elapsed >= 0.0f) {
                fly->pop_elapsed = -1.0f;
            }
            fly->fading = SDL_TRUE;
        }

        if (fly->pop_elapsed >= 0.0f) {
            fly->pop_elapsed += dt;
            if (fly->pop_elapsed >= fly->pop_delay) {
                const float t = SDL_clamp((fly->pop_elapsed - fly->pop_delay) / TITLE_FIREFLIES_POP_DURATION_S, 0.0f, 1.0f);
                fly->charge_mix = t;
                if (t >= 1.0f) {
                    fly->pop_elapsed = -1.0f;
                }
            }
        }

        if (fly->fading) {
            fly->charge_mix = SDL_max(0.0f, fly->charge_mix - dt / TITLE_FIREFLIES_FADE_DURATION_S);
            if (fly->charge_mix <= 0.0f) {
                fly->fading = SDL_FALSE;
            }
        }

        fly->x += fly->vx * dt;
        fly->y += fly->vy * dt;

        if (fly->x < min_x) {
            fly->x += span_x;
        } else if (fly->x > max_x) {
            fly->x -= span_x;
        }
        if (fly->y < min_y) {
            fly->y += span_y;
        } else if (fly->y > max_y) {
            fly->y -= span_y;
        }
    }

    fx->was_charging = status.charging;
}

void title_fireflies_render(SDL_Renderer *renderer, TitleFireflies *fx)
{
    if (!fx || !fx->ready || !renderer) {
        return;
    }

    const float time = SDL_GetTicks() / 1000.0f;

    for (int i = 0; i < TITLE_FIREFLY_COUNT; i++) {
        const TitleFirefly *fly = &fx->flies[i];
        const float pulse = 0.7f + 0.3f * SDL_sinf(time * 1.6f + fly->phase);
        const float brightness = fly->far ? 0.5f : 1.0f;

        const float base_r = SDL_clamp(0.45f + 0.45f * fly->hue_mix, 0.0f, 1.0f) * 255;
        const float base_g = 255.0f;
        const float base_b = SDL_clamp(0.25f + 0.05f * fly->hue_mix, 0.0f, 1.0f) * 255;

        const SDL_Color charge_color = TITLE_BATTERY_CHARGE_COLOR;
        const Uint8 r = (Uint8)(base_r + (charge_color.r - base_r) * fly->charge_mix);
        const Uint8 g = (Uint8)(base_g + (charge_color.g - base_g) * fly->charge_mix);
        const Uint8 b = (Uint8)(base_b + (charge_color.b - base_b) * fly->charge_mix);

        const SDL_bool popping = fly->pop_elapsed >= 0.0f && fly->pop_elapsed >= fly->pop_delay;
        const float t = popping
            ? SDL_clamp((fly->pop_elapsed - fly->pop_delay) / TITLE_FIREFLIES_POP_DURATION_S, 0.0f, 1.0f)
            : 0.0f;
        const float flare = popping ? (1.0f + 0.6f * (1.0f - t)) : 1.0f;

        const int size = fly->far ? TITLE_FIREFLIES_DRAW_SIZE / 2 : TITLE_FIREFLIES_DRAW_SIZE;
        const int half = size / 2;

        const float elapsed_since_pop = fly->pop_elapsed - fly->pop_delay;
        if (popping && elapsed_since_pop < TITLE_FIREFLIES_RING_DURATION_S) {
            const float rt = SDL_clamp(elapsed_since_pop / TITLE_FIREFLIES_RING_DURATION_S, 0.0f, 1.0f);
            const float ease_out = 1.0f - (1.0f - rt) * (1.0f - rt); /* fast expansion, decelerating */
            const float fade = (1.0f - rt) * (1.0f - rt) * (1.0f - rt); /* brightness collapses faster than the radius grows */

            const int ring_size = (int)(size * 3.0f * ease_out); /* 0 -> 3x size */
            const Uint8 ring_alpha = (Uint8)SDL_clamp(fade * 220.0f, 0.0f, 255.0f);
            const int ring_half = ring_size / 2;
            const SDL_Rect ring_dst = {(int)fly->x - ring_half, (int)fly->y - ring_half, ring_size, ring_size};
            SDL_SetTextureColorMod(fx->target.screen_texture, charge_color.r, charge_color.g, charge_color.b);
            SDL_SetTextureAlphaMod(fx->target.screen_texture, ring_alpha);
            SDL_RenderCopy(renderer, fx->target.screen_texture, NULL, &ring_dst);
        }

        SDL_SetTextureColorMod(fx->target.screen_texture, r, g, b);
        SDL_SetTextureAlphaMod(fx->target.screen_texture, (Uint8)SDL_clamp(pulse * brightness * flare * 255.0f, 0.0f, 255.0f));

        const SDL_Rect dst = {(int)fly->x - half, (int)fly->y - half, size, size};
        SDL_RenderCopy(renderer, fx->target.screen_texture, NULL, &dst);
    }
}
