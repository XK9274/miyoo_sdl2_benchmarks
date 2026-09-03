#include "gl_fbo_effects/scenes/effects.h"

#include "common/gl_effect.h"

#include <SDL2/SDL_log.h>
#include <SDL2/SDL_opengles2.h>

#include <math.h>

static const char *rsgl_effect_names[RSGL_EFFECT_MAX] = {
    "Sunrise Gradient",
    "Soft Waves",
    "Scanline Glow",
    "Floating Orbs",
    "Aurora Borealis",
    "Nebula Clouds",
    "Fire Effect",
    "Lightning Storm",
    "Crystal Cavern",
    "Plasma Flow",
    "Electric Grid",
    "Ocean Depths",
    "Retro Sun",
    "Digital Rain",
    "Chromatic Shift"
};

static const char *rsgl_fragment_shaders[RSGL_EFFECT_MAX] = {
    "precision mediump float;\n"
    "varying vec2 v_uv;\n"
    "uniform float u_time;\n"
    "void main() {\n"
    "    vec2 uv = v_uv;\n"
    "    float t = u_time * 0.2;\n"
    "    vec3 top = vec3(0.95, 0.65, 0.35);\n"
    "    vec3 bottom = vec3(0.05, 0.09, 0.18);\n"
    "    float mixv = smoothstep(0.0, 1.0, uv.y);\n"
    "    float sun = exp(-10.0 * length(uv - vec2(0.5, 0.2)));\n"
    "    vec3 color = mix(bottom, top, mixv) + vec3(1.0, 0.8, 0.4) * sun * (0.6 + 0.4 * sin(u_time));\n"
    "    gl_FragColor = vec4(color, 1.0);\n"
    "}\n",

    "precision mediump float;\n"
    "varying vec2 v_uv;\n"
    "uniform float u_time;\n"
    "void main() {\n"
    "    vec2 uv = v_uv * vec2(1.0, 1.2);\n"
    "    float wave = sin((uv.x + u_time * 0.8) * 6.0) * 0.25 + 0.5;\n"
    "    float wave2 = sin((uv.y - u_time * 0.6) * 5.0);\n"
    "    float mask = smoothstep(0.0, 1.0, wave + wave2 * 0.2);\n"
    "    vec3 base = vec3(0.12, 0.18, 0.28);\n"
    "    vec3 highlight = vec3(0.35, 0.70, 0.85);\n"
    "    gl_FragColor = vec4(mix(base, highlight, mask), 1.0);\n"
    "}\n",

    "precision mediump float;\n"
    "varying vec2 v_uv;\n"
    "uniform float u_time;\n"
    "void main() {\n"
    "    vec2 uv = v_uv;\n"
    "    float scan = step(0.5, fract(uv.y * 60.0 - u_time * 1.2));\n"
    "    float glow = smoothstep(0.48, 0.52, abs(fract(uv.x * 20.0 + u_time * 0.8) - 0.5));\n"
    "    vec3 base = vec3(0.05, 0.07, 0.12);\n"
    "    vec3 glow_color = vec3(0.2, 0.9, 0.7);\n"
    "    vec3 color = base + glow_color * glow * 0.4 + glow_color * scan * 0.4;\n"
    "    gl_FragColor = vec4(color, 1.0);\n"
    "}\n",

    "precision mediump float;\n"
    "varying vec2 v_uv;\n"
    "uniform float u_time;\n"
    "void main() {\n"
    "    vec2 uv = v_uv;\n"
    "    vec3 base = vec3(0.02, 0.04, 0.15);\n"
    "    vec3 color = base;\n"
    "    vec2 star1 = vec2(0.3 + sin(u_time * 0.2) * 0.3, 0.7 + cos(u_time * 0.3) * 0.2);\n"
    "    vec2 star2 = vec2(0.8 - sin(u_time * 0.25) * 0.2, 0.4 + cos(u_time * 0.2) * 0.3);\n"
    "    vec2 star3 = vec2(0.6 + sin(u_time * 0.15) * 0.4, 0.2 + cos(u_time * 0.35) * 0.1);\n"
    "    vec2 star4 = vec2(0.2 + cos(u_time * 0.22) * 0.25, 0.45 + sin(u_time * 0.27) * 0.25);\n"
    "    float d1 = distance(uv, star1);\n"
    "    float d2 = distance(uv, star2);\n"
    "    float d3 = distance(uv, star3);\n"
    "    float d4 = distance(uv, star4);\n"
    "    float glow1 = 1.0 / (1.0 + d1 * 40.0);\n"
    "    float glow2 = 1.0 / (1.0 + d2 * 35.0);\n"
    "    float glow3 = 1.0 / (1.0 + d3 * 45.0);\n"
    "    float glow4 = 1.0 / (1.0 + d4 * 38.0);\n"
    "    color += vec3(0.9, 0.8, 0.6) * glow1;\n"
    "    color += vec3(0.8, 0.9, 0.7) * glow2;\n"
    "    color += vec3(0.9, 0.7, 0.8) * glow3;\n"
    "    color += vec3(0.7, 0.9, 1.0) * glow4;\n"
    "    gl_FragColor = vec4(color, 1.0);\n"
    "}\n",

    "precision mediump float;\n"
    "varying vec2 v_uv;\n"
    "uniform float u_time;\n"
    "void main() {\n"
    "    vec2 uv = v_uv;\n"
    "    vec3 base = vec3(0.05, 0.15, 0.25);\n"
    "    vec3 color = base;\n"
    "    float y = uv.y + sin(uv.x * 8.0 + u_time * 1.5) * 0.1;\n"
    "    float aurora1 = smoothstep(0.3, 0.7, y) * smoothstep(0.8, 0.4, y);\n"
    "    float aurora2 = smoothstep(0.2, 0.6, y + 0.2) * smoothstep(0.9, 0.5, y + 0.2);\n"
    "    float flow = sin(uv.x * 12.0 + u_time * 2.0) * 0.5 + 0.5;\n"
    "    vec3 green = vec3(0.2, 1.0, 0.5) * aurora1 * flow;\n"
    "    vec3 blue = vec3(0.3, 0.6, 1.0) * aurora2 * (1.0 - flow);\n"
    "    color += green + blue;\n"
    "    gl_FragColor = vec4(color, 1.0);\n"
    "}\n",

    "precision mediump float;\n"
    "varying vec2 v_uv;\n"
    "uniform float u_time;\n"
    "void main() {\n"
    "    vec2 uv = v_uv;\n"
    "    vec3 base = vec3(0.1, 0.02, 0.2);\n"
    "    float noise = sin(uv.x * 15.0 + u_time * 0.8) * sin(uv.y * 12.0 + u_time * 0.6);\n"
    "    noise += sin(uv.x * 25.0 - u_time * 1.2) * sin(uv.y * 20.0 - u_time * 0.9) * 0.5;\n"
    "    noise = (noise + 2.0) * 0.25;\n"
    "    float dist = length(uv - vec2(0.5, 0.5));\n"
    "    float nebula = smoothstep(0.8, 0.2, dist) * noise;\n"
    "    vec3 purple = vec3(0.6, 0.2, 0.8) * nebula;\n"
    "    vec3 pink = vec3(1.0, 0.4, 0.6) * nebula * 0.7;\n"
    "    vec3 blue = vec3(0.2, 0.5, 1.0) * nebula * 0.5;\n"
    "    gl_FragColor = vec4(base + purple + pink + blue, 1.0);\n"
    "}\n",

    "precision mediump float;\n"
    "varying vec2 v_uv;\n"
    "uniform float u_time;\n"
    "void main() {\n"
    "    vec2 uv = v_uv;\n"
    "    float y = uv.y;\n"
    "    float x = uv.x - 0.5;\n"
    "    float flicker = sin(u_time * 6.0) * 0.05 + 0.95;\n"
    "    float wave1 = sin(x * 8.0 + u_time * 3.0) * 0.1;\n"
    "    float wave2 = sin(x * 12.0 - u_time * 4.0) * 0.05;\n"
    "    float flame_width = (0.3 - y * 0.2) * flicker;\n"
    "    float flame_mask = smoothstep(flame_width, flame_width - 0.1, abs(x + wave1 + wave2));\n"
    "    float flame_height = smoothstep(0.0, 0.2, y) * smoothstep(0.9, 0.6, y);\n"
    "    float flame = flame_mask * flame_height;\n"
    "    vec3 white = vec3(1.0, 1.0, 0.9);\n"
    "    vec3 yellow = vec3(1.0, 0.8, 0.2);\n"
    "    vec3 orange = vec3(1.0, 0.4, 0.1);\n"
    "    vec3 red = vec3(0.8, 0.2, 0.0);\n"
    "    vec3 color = mix(vec3(0.05, 0.02, 0.0), red, flame * 0.3);\n"
    "    color = mix(color, orange, flame * smoothstep(0.7, 0.3, y));\n"
    "    color = mix(color, yellow, flame * smoothstep(0.5, 0.1, y));\n"
    "    color = mix(color, white, flame * smoothstep(0.3, 0.0, y) * 0.8);\n"
    "    gl_FragColor = vec4(color, 1.0);\n"
    "}\n",

    "precision mediump float;\n"
    "varying vec2 v_uv;\n"
    "uniform float u_time;\n"
    "void main() {\n"
    "    vec2 uv = v_uv;\n"
    "    vec3 base = vec3(0.05, 0.05, 0.15);\n"
    "    vec3 color = base;\n"
    "    float lightning = 0.0;\n"
    "    float flash = step(0.98, sin(u_time * 6.0 + sin(u_time * 3.0)));\n"
    "    if (flash > 0.5) {\n"
    "        float bolt = abs(uv.x - 0.5 - sin(uv.y * 10.0 + u_time * 20.0) * 0.1);\n"
    "        lightning = smoothstep(0.05, 0.01, bolt);\n"
    "        color += vec3(0.9, 0.9, 1.0) * lightning;\n"
    "        color += vec3(0.3, 0.3, 0.6) * flash * 0.3;\n"
    "    }\n"
    "    float rain = fract(uv.y * 50.0 - u_time * 10.0);\n"
    "    rain = smoothstep(0.9, 1.0, rain);\n"
    "    color += vec3(0.2, 0.2, 0.3) * rain * 0.3;\n"
    "    gl_FragColor = vec4(color, 1.0);\n"
    "}\n",

    "precision mediump float;\n"
    "varying vec2 v_uv;\n"
    "uniform float u_time;\n"
    "void main() {\n"
    "    vec2 uv = v_uv - 0.5;\n"
    "    float angle = atan(uv.y, uv.x);\n"
    "    float radius = length(uv);\n"
    "    float facets = abs(fract(angle * 2.0 / 3.14159 + radius * 3.0 - u_time * 0.2) - 0.5);\n"
    "    float shimmer = 1.0 - smoothstep(0.0, 0.45, facets);\n"
    "    vec3 base = vec3(0.04, 0.08, 0.16);\n"
    "    vec3 edge = vec3(0.2, 0.8, 1.0);\n"
    "    vec3 color = mix(base, edge, shimmer);\n"
    "    color += edge * (1.0 - smoothstep(0.0, 0.6, radius)) * 0.4;\n"
    "    gl_FragColor = vec4(color, 1.0);\n"
    "}\n",

    "precision mediump float;\n"
    "varying vec2 v_uv;\n"
    "uniform float u_time;\n"
    "void main() {\n"
    "    vec2 uv = v_uv * 2.0 - 1.0;\n"
    "    float t = u_time * 0.7;\n"
    "    float r = length(uv);\n"
    "    float a = atan(uv.y, uv.x);\n"
    "    float wave = sin(r * 6.0 - t * 2.0) + sin(a * 3.0 + t);\n"
    "    float swirl = cos((uv.x + uv.y) * 3.5 - t) * 0.5 + 0.5;\n"
    "    float energy = 0.5 + 0.5 * sin(wave + swirl * 3.14159);\n"
    "    vec3 base = vec3(0.1, 0.0, 0.2);\n"
    "    vec3 glow = vec3(0.8, 0.3, 1.0);\n"
    "    vec3 color = mix(base, glow, energy);\n"
    "    color += vec3(0.2, 0.6, 1.0) * smoothstep(0.0, 0.6, 1.0 - abs(sin(a * 2.0 - t)));\n"
    "    gl_FragColor = vec4(color, 1.0);\n"
    "}\n",

    "precision mediump float;\n"
    "varying vec2 v_uv;\n"
    "uniform float u_time;\n"
    "void main() {\n"
    "    vec2 uv = v_uv * 1.2;\n"
    "    vec2 grid = abs(fract(uv * 12.0 - u_time * 0.4) - 0.5);\n"
    "    float line = smoothstep(0.0, 0.08, 0.5 - min(grid.x, grid.y));\n"
    "    float pulse = 0.5 + 0.5 * sin(u_time * 5.0 + uv.x * 6.0);\n"
    "    vec3 base = vec3(0.02, 0.03, 0.08);\n"
    "    vec3 neon = vec3(0.0, 0.8, 1.0);\n"
    "    vec3 color = base + neon * line * (0.4 + 0.6 * pulse);\n"
    "    gl_FragColor = vec4(color, 1.0);\n"
    "}\n",

    "precision mediump float;\n"
    "varying vec2 v_uv;\n"
    "uniform float u_time;\n"
    "void main() {\n"
    "    vec2 uv = v_uv * 2.0 - 1.0;\n"
    "    float t = u_time * 0.5;\n"
    "    float wave = sin(uv.x * 4.0 + t) + cos(uv.y * 5.0 - t * 1.4);\n"
    "    float caustics = sin((uv.x + uv.y) * 8.0 + t * 3.0) * 0.5 + 0.5;\n"
    "    float depth = clamp(1.0 - length(uv), 0.0, 1.0);\n"
    "    vec3 deep = vec3(0.0, 0.1, 0.2);\n"
    "    vec3 light = vec3(0.0, 0.7, 0.8);\n"
    "    vec3 color = mix(deep, light, caustics * depth);\n"
    "    color += vec3(0.0, 0.3, 0.5) * (wave * 0.1);\n"
    "    gl_FragColor = vec4(color, 1.0);\n"
    "}\n",

    "precision mediump float;\n"
    "varying vec2 v_uv;\n"
    "uniform float u_time;\n"
    "void main() {\n"
    "    vec2 uv = v_uv;\n"
    "    vec2 sun_center = vec2(0.5, 0.35);\n"
    "    float dist = length(uv - sun_center);\n"
    "    float sun = 1.0 - smoothstep(0.0, 0.4, dist);\n"
    "    float band = smoothstep(0.0, 0.1, abs(fract((uv.y - 0.35) * 18.0 - u_time * 0.3) - 0.5));\n"
    "    vec3 sky = mix(vec3(0.02, 0.0, 0.15), vec3(0.8, 0.2, 0.4), uv.y);\n"
    "    vec3 sun_color = vec3(1.0, 0.7, 0.2);\n"
    "    vec3 color = sky;\n"
    "    color = mix(color, sun_color, sun);\n"
    "    color = mix(color, color * 0.5, band * sun);\n"
    "    gl_FragColor = vec4(color, 1.0);\n"
    "}\n",

    "precision mediump float;\n"
    "varying vec2 v_uv;\n"
    "uniform float u_time;\n"
    "float hash(float n) {\n"
    "    return fract(sin(n) * 43758.5453123);\n"
    "}\n"
    "void main() {\n"
    "    vec2 uv = vec2(v_uv.x * 1.2, v_uv.y);\n"
    "    float t = u_time * 0.8;\n"
    "    float column = floor(uv.x * 32.0);\n"
    "    float offset = hash(column * 12.9898);\n"
    "    float speed = 0.3 + offset * 0.7;\n"
    "    float trail = fract(uv.y + t * speed);\n"
    "    float lane = 1.0 - smoothstep(0.0, 0.48, abs(fract(uv.x * 32.0) - 0.5));\n"
    "    float head = smoothstep(0.0, 0.1, 1.0 - trail) * lane;\n"
    "    float tail = smoothstep(0.0, 0.4, trail) * (1.0 - smoothstep(0.7, 1.0, trail)) * lane;\n"
    "    vec3 base = vec3(0.0, 0.05, 0.08);\n"
    "    vec3 color = base + vec3(0.0, 1.0, 0.4) * (head * 0.9 + tail * 0.5);\n"
    "    gl_FragColor = vec4(color, 1.0);\n"
    "}\n",

    "precision mediump float;\n"
    "varying vec2 v_uv;\n"
    "uniform float u_time;\n"
    "void main() {\n"
    "    vec2 uv = v_uv - 0.5;\n"
    "    float t = u_time * 0.6;\n"
    "    float radius = length(uv);\n"
    "    float angle = atan(uv.y, uv.x);\n"
    "    float ripple = sin(radius * 12.0 - t * 4.0);\n"
    "    float offset = 0.02 * sin(angle * 3.0 + t * 2.0);\n"
    "    float r = clamp(0.5 + 0.5 * sin((radius + offset) * 10.0 - t), 0.0, 1.0);\n"
    "    float g = clamp(0.5 + 0.5 * sin((radius - offset) * 12.0 + t * 1.2), 0.0, 1.0);\n"
    "    float b = clamp(0.5 + 0.5 * sin((radius + ripple * 0.1) * 14.0 + t * 1.5), 0.0, 1.0);\n"
    "    float vignette = 1.0 - smoothstep(0.2, 0.7, radius);\n"
    "    vec3 color = vec3(r, g, b) * vignette;\n"
    "    gl_FragColor = vec4(color, 1.0);\n"
    "}\n"
};

typedef struct {
    GLEffectTarget target;
    Uint32 program;
} RsglEffect;

static RsglEffect rsgl_effects[RSGL_EFFECT_MAX];
static int rsgl_effect_total = 0;

static void rsgl_set_time_uniform(Uint32 program, void *userdata)
{
    const float time = *(const float *)userdata;
    const int loc = glGetUniformLocation(program, "u_time");
    if (loc >= 0) {
        glUniform1f(loc, time);
    }
}

static SDL_bool rsgl_create_effects(SDL_Renderer *renderer, int width, int height)
{
    rsgl_effect_total = 0;
    for (int i = 0; i < RSGL_EFFECT_MAX; ++i) {
        RsglEffect *effect = &rsgl_effects[rsgl_effect_total];
        if (!gl_effect_target_create(&effect->target, renderer, width, height)) {
            continue;
        }
        effect->program = gl_effect_compile_program(rsgl_fragment_shaders[i]);
        if (!effect->program) {
            gl_effect_target_destroy(&effect->target);
            continue;
        }
        ++rsgl_effect_total;
    }

    if (rsgl_effect_total == 0) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "rsgl_create_effects: no effects compiled");
        return SDL_FALSE;
    }
    return SDL_TRUE;
}

static void rsgl_destroy_effects(void)
{
    for (int i = 0; i < rsgl_effect_total; ++i) {
        gl_effect_destroy_program(rsgl_effects[i].program);
        rsgl_effects[i].program = 0;
        gl_effect_target_destroy(&rsgl_effects[i].target);
    }
    rsgl_effect_total = 0;
}

SDL_bool rsgl_effects_init(RsglState *state, SDL_Renderer *renderer)
{
    if (!state || !renderer) {
        return SDL_FALSE;
    }

    if (!gl_effect_context_acquire()) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "rsgl_effects_init: failed to acquire shared GL context");
        return SDL_FALSE;
    }

    if (!rsgl_create_effects(renderer, state->fbo_width, state->fbo_height)) {
        gl_effect_context_release();
        return SDL_FALSE;
    }

    state->gl_ready = SDL_TRUE;
    rsgl_state_commit_fbo_size(state);
    state->effect_count = rsgl_effect_total;
    if (state->effect_index >= state->effect_count) {
        state->effect_index = 0;
    }

    return SDL_TRUE;
}

SDL_bool rsgl_effects_apply_fbo_size(RsglState *state, SDL_Renderer *renderer)
{
    if (!state || !renderer) {
        return SDL_FALSE;
    }
    if (!state->gl_ready) {
        rsgl_state_commit_fbo_size(state);
        return SDL_TRUE;
    }

    const int width = state->fbo_width;
    const int height = state->fbo_height;
    if (width <= 0 || height <= 0) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "rsgl_effects_apply_fbo_size: invalid target size %dx%d",
                    width, height);
        rsgl_state_revert_fbo_size(state);
        return SDL_FALSE;
    }

    /* Build the new-size targets before touching the live ones, so a
     * failure partway through leaves the currently rendering effects
     * untouched instead of half torn down. */
    GLEffectTarget new_targets[RSGL_EFFECT_MAX];
    int built = 0;
    for (; built < rsgl_effect_total; ++built) {
        if (!gl_effect_target_create(&new_targets[built], renderer, width, height)) {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                        "rsgl_effects_apply_fbo_size: target creation failed at index %d",
                        built);
            for (int i = 0; i < built; ++i) {
                gl_effect_target_destroy(&new_targets[i]);
            }
            rsgl_state_revert_fbo_size(state);
            return SDL_FALSE;
        }
    }

    for (int i = 0; i < rsgl_effect_total; ++i) {
        gl_effect_target_destroy(&rsgl_effects[i].target);
        rsgl_effects[i].target = new_targets[i];
    }

    rsgl_state_commit_fbo_size(state);
    return SDL_TRUE;
}

void rsgl_effects_render(RsglState *state,
                         SDL_Renderer *renderer,
                         BenchMetrics *metrics,
                         double delta_seconds)
{
    if (!state || !renderer || !state->gl_ready) {
        return;
    }

    state->elapsed_time += (float)delta_seconds;
    if (state->auto_cycle && state->effect_count > 0) {
        const double cycle_time = 10.0;
        if (fmod(state->elapsed_time, cycle_time) < delta_seconds) {
            state->effect_index = (state->effect_index + 1) % state->effect_count;
        }
    }

    if (state->effect_count <= 0) {
        return;
    }
    const int mode = state->effect_index % state->effect_count;
    RsglEffect *effect = &rsgl_effects[mode];
    gl_effect_render(&effect->target, effect->program, rsgl_set_time_uniform, &state->elapsed_time);

    SDL_Rect dst = {
        0,
        (int)state->top_margin,
        state->screen_width,
        SDL_max(1, state->screen_height - (int)state->top_margin)
    };

    SDL_RenderCopy(renderer, effect->target.screen_texture, NULL, &dst);

    if (metrics) {
        metrics->draw_calls += 2; // GL render + SDL copy
        metrics->texture_switches++;
    }
}

void rsgl_effects_warmup(RsglState *state)
{
    if (!state || !state->gl_ready || state->effect_count <= 0) {
        return;
    }

    float warm_time = 0.0f;
    const int mode = state->effect_index % state->effect_count;
    RsglEffect *effect = &rsgl_effects[mode];
    gl_effect_render(&effect->target, effect->program, rsgl_set_time_uniform, &warm_time);
}

void rsgl_effects_cleanup(RsglState *state)
{
    if (!state) {
        return;
    }

    if (state->gl_ready) {
        rsgl_destroy_effects();
        gl_effect_context_release();
    }

    state->gl_ready = SDL_FALSE;
    state->effect_count = 0;
}

int rsgl_effect_count(void)
{
    return rsgl_effect_total;
}

const char *rsgl_effect_name(int index)
{
    if (index < 0 || index >= rsgl_effect_total) {
        return "Unknown";
    }
    return rsgl_effect_names[index];
}

SDL_bool rsgl_effect_index_from_name(const char *name, int *out_index)
{
    for (int i = 0; i < rsgl_effect_total; i++) {
        if (SDL_strcasecmp(name, rsgl_effect_names[i]) == 0) {
            *out_index = i;
            return SDL_TRUE;
        }
    }
    return SDL_FALSE;
}
