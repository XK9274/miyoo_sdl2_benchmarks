#include "title/fireflies.h"

#include <stdlib.h>

#include <SDL2/SDL_opengles2.h>

#include "common/types.h"

/* Tiny sprite rendered once at init; per-frame animation is pure SDL texture modulation, no GL.
 * FBO is sized up from the sprite's own glow-falloff shader so the 12px-radius draw size stays
 * smooth instead of visibly upscaled/blocky. */
#define TITLE_FIREFLIES_GL_W 15
#define TITLE_FIREFLIES_GL_H 15
#define TITLE_FIREFLIES_DRAW_SIZE 24
#define TITLE_FIREFLIES_MARGIN 24.0f

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

    for (int i = 0; i < TITLE_FIREFLY_COUNT; i++) {
        TitleFirefly *fly = &fx->flies[i];
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

        const Uint8 r = (Uint8)(SDL_clamp(0.45f + 0.45f * fly->hue_mix, 0.0f, 1.0f) * 255);
        const Uint8 g = 255;
        const Uint8 b = (Uint8)(SDL_clamp(0.25f + 0.05f * fly->hue_mix, 0.0f, 1.0f) * 255);

        SDL_SetTextureColorMod(fx->target.screen_texture, r, g, b);
        SDL_SetTextureAlphaMod(fx->target.screen_texture, (Uint8)(pulse * brightness * 255));

        const int size = fly->far ? TITLE_FIREFLIES_DRAW_SIZE / 2 : TITLE_FIREFLIES_DRAW_SIZE;
        const int half = size / 2;
        const SDL_Rect dst = {(int)fly->x - half, (int)fly->y - half, size, size};
        SDL_RenderCopy(renderer, fx->target.screen_texture, NULL, &dst);
    }
}
