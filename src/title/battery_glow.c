#include "title/battery_glow.h"

#include <SDL2/SDL_opengles2.h>

/* Tiny sprite rendered once at init; per-frame animation is pure SDL texture modulation, no GL. */
#define TITLE_BATTERY_GLOW_GL_W 21
#define TITLE_BATTERY_GLOW_GL_H 21

static const char *g_battery_glow_fragment_src =
    "precision mediump float;\n"
    "varying vec2 v_uv;\n"
    "void main() {\n"
    "    float d = length(v_uv - vec2(0.5)) * 2.0;\n"
    "    float glow = smoothstep(1.0, 0.0, d);\n"
    "    gl_FragColor = vec4(vec3(1.0), glow * glow);\n"
    "}\n";

void title_battery_glow_init(TitleBatteryGlow *glow, SDL_Renderer *renderer)
{
    if (!glow) {
        return;
    }
    SDL_zerop(glow);

    if (!renderer || !gl_effect_context_acquire()) {
        return;
    }
    if (!gl_effect_target_create(&glow->target, renderer, TITLE_BATTERY_GLOW_GL_W, TITLE_BATTERY_GLOW_GL_H)) {
        gl_effect_context_release();
        return;
    }

    glow->program = gl_effect_compile_program(g_battery_glow_fragment_src);
    if (!glow->program) {
        gl_effect_target_destroy(&glow->target);
        gl_effect_context_release();
        return;
    }

    /* Render the glow shape exactly once -- static, no uniforms needed. */
    gl_effect_render(&glow->target, glow->program, NULL, NULL);
    SDL_SetTextureBlendMode(glow->target.screen_texture, SDL_BLENDMODE_ADD);

    glow->ready = SDL_TRUE;
}

void title_battery_glow_shutdown(TitleBatteryGlow *glow)
{
    if (!glow || !glow->ready) {
        return;
    }
    gl_effect_destroy_program(glow->program);
    glow->program = 0;
    gl_effect_target_destroy(&glow->target);
    gl_effect_context_release();
    glow->ready = SDL_FALSE;
}

void title_battery_glow_render(TitleBatteryGlow *glow, SDL_Renderer *renderer,
                               int center_x, int center_y, int diameter, SDL_Color color)
{
    if (!glow || !glow->ready || !renderer) {
        return;
    }

    const float time = SDL_GetTicks() / 1000.0f;
    const float pulse = 0.5f + 0.5f * SDL_sinf(time * 1.2f);

    SDL_SetTextureColorMod(glow->target.screen_texture, color.r, color.g, color.b);
    SDL_SetTextureAlphaMod(glow->target.screen_texture, (Uint8)(pulse * 255.0f));

    const SDL_Rect dst = {center_x - diameter / 2, center_y - diameter / 2, diameter, diameter};
    SDL_RenderCopy(renderer, glow->target.screen_texture, NULL, &dst);
}
