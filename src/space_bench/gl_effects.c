#include "space_bench/gl_effects.h"

#include "common/gl_effect.h"

#include <SDL2/SDL_opengles2.h>

/* Shaders output grayscale luminance; colour comes from SDL_SetTextureColorMod at blit time. */

#define SPACE_GL_BOLT_SIZE 20
#define SPACE_GL_PICKUP_SIZE 40
#define SPACE_GL_THUMPER_SIZE 64
#define SPACE_GL_SHIELD_SIZE 72
/* Laser beam strips are tiled horizontally (not stretched) to cover the
 * beam's length -- see space_render_laser_beam in render/projectiles.c.
 * Native pixel size, so no scaling distortion regardless of beam length.
 * SPACE_GL_LASER_TILE_W itself lives in gl_effects.h (shared with the tiling
 * code in render/projectiles.c). */
#define SPACE_GL_LASER_GLOW_H 22
#define SPACE_GL_LASER_EDGE_H 10
#define SPACE_GL_LASER_CORE_H 4

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
    "    float core = smoothstep(0.55, 0.35, d);\n"
    "    float ring = smoothstep(0.1, 0.0, abs(d - 0.7)) * pulse * 0.6;\n"
    "    float glow = smoothstep(1.0, 0.55, d) * 0.25;\n"
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

/* Laser beam cross-section strips (core/edge/glow), sampled along v_uv.y
 * only -- v_uv.x just feeds a subtle animated shimmer, since these tiles
 * are drawn repeated (not stretched) along the beam's length.
 *
 * These three are additive-blended (SDL_BLENDMODE_ADD, set right after
 * creation below), unlike bolt/pickup/thumper/shield which use normal
 * alpha blending. SDL's ADD formula is dstRGB += srcRGB * srcA: with the
 * usual vec4(vec3(i), i) convention (color AND alpha both equal the
 * falloff i), that multiplies the falloff by itself -- an already-soft
 * 0.1-0.3 glow gets squared down to 0.01-0.09, effectively invisible.
 * So these output alpha = 1.0 always and put the actual falloff in RGB,
 * making srcRGB * srcA = i * 1 = i, the correct linear contribution. */
static const char *g_laser_glow_fragment_src =
    "precision mediump float;\n"
    "varying vec2 v_uv;\n"
    "uniform float u_time;\n"
    "void main() {\n"
    "    float d = clamp(abs(v_uv.y - 0.5) * 2.0, 0.0, 1.0);\n"
    "    float glow = pow(1.0 - d, 2.0) * 0.9;\n"
    "    float shimmer = 0.9 + 0.1 * sin(u_time * 6.0 + v_uv.x * 20.0);\n"
    "    float i = glow * shimmer;\n"
    "    gl_FragColor = vec4(vec3(i), 1.0);\n"
    "}\n";

/* Distinct bright ring between the core and the outer glow, like the
 * crisp edge of a neon tube -- narrow band centered around d=0.45. */
static const char *g_laser_edge_fragment_src =
    "precision mediump float;\n"
    "varying vec2 v_uv;\n"
    "uniform float u_time;\n"
    "void main() {\n"
    "    float d = clamp(abs(v_uv.y - 0.5) * 2.0, 0.0, 1.0);\n"
    "    float edge = smoothstep(0.0, 0.4, 1.0 - abs(d - 0.45) / 0.4);\n"
    "    float shimmer = 0.9 + 0.1 * sin(u_time * 10.0 + v_uv.x * 25.0);\n"
    "    float i = edge * shimmer;\n"
    "    gl_FragColor = vec4(vec3(i), 1.0);\n"
    "}\n";

static const char *g_laser_core_fragment_src =
    "precision mediump float;\n"
    "varying vec2 v_uv;\n"
    "uniform float u_time;\n"
    "void main() {\n"
    "    float d = clamp(abs(v_uv.y - 0.5) * 2.0, 0.0, 1.0);\n"
    "    float core = smoothstep(0.9, 0.0, d);\n"
    "    float flicker = 0.94 + 0.06 * sin(u_time * 40.0 + v_uv.x * 60.0);\n"
    "    float i = core * flicker;\n"
    "    gl_FragColor = vec4(vec3(i), 1.0);\n"
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
static SpaceGLEffect g_laser_glow;
static SpaceGLEffect g_laser_edge;
static SpaceGLEffect g_laser_core;
static SDL_bool g_thumper_rendered_this_frame = SDL_FALSE;
static SDL_bool g_lasers_rendered_this_frame = SDL_FALSE;

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
        create_effect(&g_shield, renderer, SPACE_GL_SHIELD_SIZE, SPACE_GL_SHIELD_SIZE, g_shield_fragment_src) &&
        create_effect(&g_laser_glow, renderer, SPACE_GL_LASER_TILE_W, SPACE_GL_LASER_GLOW_H, g_laser_glow_fragment_src) &&
        create_effect(&g_laser_edge, renderer, SPACE_GL_LASER_TILE_W, SPACE_GL_LASER_EDGE_H, g_laser_edge_fragment_src) &&
        create_effect(&g_laser_core, renderer, SPACE_GL_LASER_TILE_W, SPACE_GL_LASER_CORE_H, g_laser_core_fragment_src);

    if (!ok) {
        destroy_effect(&g_bolt);
        destroy_effect(&g_pickup);
        destroy_effect(&g_thumper);
        destroy_effect(&g_shield);
        destroy_effect(&g_laser_glow);
        destroy_effect(&g_laser_edge);
        destroy_effect(&g_laser_core);
        gl_effect_context_release();
        return SDL_FALSE;
    }

    /* Additive rather than the default alpha-blend: a beam should brighten
     * the space background under it, not paint a translucent rect over it.
     * (Shaders above already output alpha=1.0 to play correctly with ADD's
     * dstRGB += srcRGB * srcA -- see the comment above g_laser_glow_fragment_src.) */
    SDL_SetTextureBlendMode(g_laser_glow.target.screen_texture, SDL_BLENDMODE_ADD);
    SDL_SetTextureBlendMode(g_laser_edge.target.screen_texture, SDL_BLENDMODE_ADD);
    SDL_SetTextureBlendMode(g_laser_core.target.screen_texture, SDL_BLENDMODE_ADD);

    g_ready = SDL_TRUE;
    return SDL_TRUE;
}

void space_gl_effects_warmup(void)
{
    if (!g_ready) {
        return;
    }

    float time = 0.0f;
    float progress = 0.5f;
    gl_effect_render(&g_bolt.target, g_bolt.program, set_time_uniform, &time);
    gl_effect_render(&g_pickup.target, g_pickup.program, set_time_uniform, &time);
    gl_effect_render(&g_shield.target, g_shield.program, set_time_uniform, &time);
    gl_effect_render(&g_thumper.target, g_thumper.program, set_progress_uniform, &progress);
    gl_effect_render(&g_laser_glow.target, g_laser_glow.program, set_time_uniform, &time);
    gl_effect_render(&g_laser_edge.target, g_laser_edge.program, set_time_uniform, &time);
    gl_effect_render(&g_laser_core.target, g_laser_core.program, set_time_uniform, &time);
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
    destroy_effect(&g_laser_glow);
    destroy_effect(&g_laser_edge);
    destroy_effect(&g_laser_core);
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

    g_lasers_rendered_this_frame = SDL_FALSE;
    if (input->lasers_active) {
        gl_effect_render(&g_laser_glow.target, g_laser_glow.program, set_time_uniform, &time);
        gl_effect_render(&g_laser_edge.target, g_laser_edge.program, set_time_uniform, &time);
        gl_effect_render(&g_laser_core.target, g_laser_core.program, set_time_uniform, &time);
        g_lasers_rendered_this_frame = SDL_TRUE;
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

SDL_Texture *space_gl_effect_laser_glow_texture(void)
{
    return (g_ready && g_lasers_rendered_this_frame) ? g_laser_glow.target.screen_texture : NULL;
}

SDL_Texture *space_gl_effect_laser_edge_texture(void)
{
    return (g_ready && g_lasers_rendered_this_frame) ? g_laser_edge.target.screen_texture : NULL;
}

SDL_Texture *space_gl_effect_laser_core_texture(void)
{
    return (g_ready && g_lasers_rendered_this_frame) ? g_laser_core.target.screen_texture : NULL;
}
