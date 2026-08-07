#include "space_bench/gl_effects.h"

#include "common/gl_effect.h"

#include <SDL2/SDL_opengles2.h>

/* Shaders output luminance in RGB and shape in alpha only -- no colour
 * uniform. Per-instance colour comes from SDL_SetTextureColorMod on the
 * shared screen_texture at blit time (see the render/ directory), so one GL
 * render per effect type per frame can serve any number of
 * differently-coloured on-screen instances cheaply. Effect textures are
 * kept small; on-screen size comes entirely from the destination rect at
 * blit time, not the FBO resolution. */

#define SPACE_GL_BOLT_SIZE 20
#define SPACE_GL_PICKUP_SIZE 40
#define SPACE_GL_THUMPER_SIZE 64
#define SPACE_GL_SHIELD_SIZE 72

static const char *g_bolt_fragment_src =
    "precision mediump float;\n"
    "varying vec2 v_uv;\n"
    "uniform float u_time;\n"
    "void main() {\n"
    "    vec2 uv = v_uv - 0.5;\n"
    "    float d = length(uv) * 2.0;\n"
    "    float pulse = 0.85 + 0.15 * sin(u_time * 20.0);\n"
    "    float core = smoothstep(0.35, 0.0, d) * pulse;\n"
    "    float glow = smoothstep(1.0, 0.15, d) * 0.6;\n"
    "    float i = clamp(core + glow, 0.0, 1.0);\n"
    "    gl_FragColor = vec4(vec3(i), i);\n"
    "}\n";

static const char *g_pickup_fragment_src =
    "precision mediump float;\n"
    "varying vec2 v_uv;\n"
    "uniform float u_time;\n"
    "void main() {\n"
    "    vec2 uv = v_uv - 0.5;\n"
    "    float d = length(uv) * 2.0;\n"
    "    float pulse = 0.6 + 0.4 * sin(u_time * 4.0);\n"
    "    float core = smoothstep(0.3, 0.0, d);\n"
    "    float ring = smoothstep(0.12, 0.0, abs(d - 0.55)) * pulse;\n"
    "    float glow = smoothstep(1.0, 0.2, d) * 0.5 * pulse;\n"
    "    float i = clamp(core + ring + glow, 0.0, 1.0);\n"
    "    gl_FragColor = vec4(vec3(i), i);\n"
    "}\n";

static const char *g_thumper_fragment_src =
    "precision mediump float;\n"
    "varying vec2 v_uv;\n"
    "uniform float u_progress;\n"
    "void main() {\n"
    "    vec2 uv = v_uv - 0.5;\n"
    "    float d = length(uv) * 2.0;\n"
    "    float ring_radius = 0.15 + u_progress * 0.8;\n"
    "    float ring = smoothstep(0.1, 0.0, abs(d - ring_radius));\n"
    "    float fade = 1.0 - u_progress;\n"
    "    float i = ring * fade;\n"
    "    gl_FragColor = vec4(vec3(i), i);\n"
    "}\n";

static const char *g_shield_fragment_src =
    "precision mediump float;\n"
    "varying vec2 v_uv;\n"
    "uniform float u_time;\n"
    "void main() {\n"
    "    vec2 uv = v_uv - 0.5;\n"
    "    float d = length(uv) * 2.0;\n"
    "    float rim = smoothstep(0.55, 0.85, d) * (1.0 - smoothstep(0.95, 1.0, d));\n"
    "    float pulse = 0.75 + 0.25 * sin(u_time * 3.0);\n"
    "    float ripple = 0.85 + 0.15 * sin(d * 20.0 - u_time * 6.0);\n"
    "    float i = rim * pulse * ripple;\n"
    "    gl_FragColor = vec4(vec3(i), i);\n"
    "}\n";

typedef struct {
    GLEffectTarget target;
    Uint32 program;
} SpaceGLEffect;

static SDL_bool g_ready = SDL_FALSE;
static SpaceGLEffect g_bolt;
static SpaceGLEffect g_pickup;
static SpaceGLEffect g_thumper;
static SpaceGLEffect g_shield;
static SDL_bool g_thumper_rendered_this_frame = SDL_FALSE;

static void set_time_uniform(Uint32 program, void *userdata)
{
    const float time = *(const float *)userdata;
    const int loc = glGetUniformLocation(program, "u_time");
    if (loc >= 0) {
        glUniform1f(loc, time);
    }
}

static void set_progress_uniform(Uint32 program, void *userdata)
{
    const float progress = *(const float *)userdata;
    const int loc = glGetUniformLocation(program, "u_progress");
    if (loc >= 0) {
        glUniform1f(loc, progress);
    }
}

static SDL_bool create_effect(SpaceGLEffect *effect, SDL_Renderer *renderer, int width, int height, const char *fragment_src)
{
    if (!gl_effect_target_create(&effect->target, renderer, width, height)) {
        return SDL_FALSE;
    }
    effect->program = gl_effect_compile_program(fragment_src);
    if (!effect->program) {
        gl_effect_target_destroy(&effect->target);
        return SDL_FALSE;
    }
    return SDL_TRUE;
}

static void destroy_effect(SpaceGLEffect *effect)
{
    gl_effect_destroy_program(effect->program);
    effect->program = 0;
    gl_effect_target_destroy(&effect->target);
}

SDL_bool space_gl_effects_init(SDL_Renderer *renderer)
{
    if (g_ready) {
        return SDL_TRUE;
    }
    if (!renderer || !gl_effect_context_acquire()) {
        return SDL_FALSE;
    }

    const SDL_bool ok =
        create_effect(&g_bolt, renderer, SPACE_GL_BOLT_SIZE, SPACE_GL_BOLT_SIZE, g_bolt_fragment_src) &&
        create_effect(&g_pickup, renderer, SPACE_GL_PICKUP_SIZE, SPACE_GL_PICKUP_SIZE, g_pickup_fragment_src) &&
        create_effect(&g_thumper, renderer, SPACE_GL_THUMPER_SIZE, SPACE_GL_THUMPER_SIZE, g_thumper_fragment_src) &&
        create_effect(&g_shield, renderer, SPACE_GL_SHIELD_SIZE, SPACE_GL_SHIELD_SIZE, g_shield_fragment_src);

    if (!ok) {
        destroy_effect(&g_bolt);
        destroy_effect(&g_pickup);
        destroy_effect(&g_thumper);
        destroy_effect(&g_shield);
        gl_effect_context_release();
        return SDL_FALSE;
    }

    g_ready = SDL_TRUE;
    return SDL_TRUE;
}

void space_gl_effects_warmup(void)
{
    if (!g_ready) {
        return;
    }

    /* Compiling shader source up front (space_gl_effects_init) is not
     * enough on its own -- many GL drivers still defer real work (shader
     * finalisation, FBO binding, first glReadPixels/texture upload) to the
     * first actual draw with a given program/target. Since normal gameplay
     * only renders each effect when something on screen needs it
     * (space_gl_effects_update's *_active gating), that cold first-draw
     * would otherwise land on whichever frame first fires a bolt/grabs a
     * pickup/raises a shield, as a visible stutter. Force it here instead,
     * once, while the loading screen is still up. */
    float time = 0.0f;
    float progress = 0.5f;
    gl_effect_render(&g_bolt.target, g_bolt.program, set_time_uniform, &time);
    gl_effect_render(&g_pickup.target, g_pickup.program, set_time_uniform, &time);
    gl_effect_render(&g_shield.target, g_shield.program, set_time_uniform, &time);
    gl_effect_render(&g_thumper.target, g_thumper.program, set_progress_uniform, &progress);
}

void space_gl_effects_shutdown(void)
{
    if (!g_ready) {
        return;
    }
    destroy_effect(&g_bolt);
    destroy_effect(&g_pickup);
    destroy_effect(&g_thumper);
    destroy_effect(&g_shield);
    gl_effect_context_release();
    g_ready = SDL_FALSE;
}

SDL_bool space_gl_effects_ready(void)
{
    return g_ready;
}

void space_gl_effects_update(const SpaceGLEffectsFrameInput *input)
{
    if (!g_ready || !input) {
        return;
    }

    float time = input->time_accumulator;

    if (input->bolts_active) {
        gl_effect_render(&g_bolt.target, g_bolt.program, set_time_uniform, &time);
    }
    if (input->pickups_active) {
        gl_effect_render(&g_pickup.target, g_pickup.program, set_time_uniform, &time);
    }
    if (input->shield_active) {
        gl_effect_render(&g_shield.target, g_shield.program, set_time_uniform, &time);
    }

    g_thumper_rendered_this_frame = SDL_FALSE;
    if (input->thumper_progress >= 0.0f) {
        float progress = input->thumper_progress;
        gl_effect_render(&g_thumper.target, g_thumper.program, set_progress_uniform, &progress);
        g_thumper_rendered_this_frame = SDL_TRUE;
    }
}

SDL_Texture *space_gl_effect_bolt_texture(void)
{
    return g_ready ? g_bolt.target.screen_texture : NULL;
}

SDL_Texture *space_gl_effect_pickup_texture(void)
{
    return g_ready ? g_pickup.target.screen_texture : NULL;
}

SDL_Texture *space_gl_effect_thumper_texture(void)
{
    return (g_ready && g_thumper_rendered_this_frame) ? g_thumper.target.screen_texture : NULL;
}

SDL_Texture *space_gl_effect_shield_texture(void)
{
    return g_ready ? g_shield.target.screen_texture : NULL;
}
