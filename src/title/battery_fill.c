#include "title/battery_fill.h"

#include <SDL2/SDL_opengles2.h>

/* Internal render resolution, stretched to the icon's actual on-screen inner rect. */
#define TITLE_BATTERY_FILL_GL_W 48
#define TITLE_BATTERY_FILL_GL_H 20

static const char *g_battery_fill_fragment_src =
    "precision mediump float;\n"
    "varying vec2 v_uv;\n"
    "uniform float u_time;\n"
    "uniform float u_fill;\n"
    "uniform float u_speed;\n"
    "uniform vec3 u_color;\n"
    "void main() {\n"
    "    float wave = sin(v_uv.y * 10.0 + u_time * 2.2) * 0.015;\n"
    "    float edge = u_fill + wave;\n"
    "    if (v_uv.x > edge) { discard; }\n"
    "    float grad = mix(1.15, 0.75, v_uv.y);\n"
    "    vec3 col = u_color * grad;\n"
    "    float diag = v_uv.x * 0.5 + v_uv.y * 0.5;\n"
    "    float shimmer_pos = fract(u_time * u_speed);\n"
    "    float shimmer = smoothstep(0.08, 0.0, abs(diag - shimmer_pos));\n"
    "    col += vec3(1.0) * shimmer * 0.5;\n"
    "    gl_FragColor = vec4(col, 1.0);\n"
    "}\n";

typedef struct {
    float fill;
    float time;
    float speed;
    float color[3];
} TitleBatteryFillUniforms;

static void title_battery_fill_set_uniforms(Uint32 program, void *userdata)
{
    const TitleBatteryFillUniforms *u = (const TitleBatteryFillUniforms *)userdata;
    const GLint loc_fill = glGetUniformLocation(program, "u_fill");
    const GLint loc_time = glGetUniformLocation(program, "u_time");
    const GLint loc_speed = glGetUniformLocation(program, "u_speed");
    const GLint loc_color = glGetUniformLocation(program, "u_color");
    if (loc_fill >= 0) {
        glUniform1f(loc_fill, u->fill);
    }
    if (loc_time >= 0) {
        glUniform1f(loc_time, u->time);
    }
    if (loc_speed >= 0) {
        glUniform1f(loc_speed, u->speed);
    }
    if (loc_color >= 0) {
        glUniform3fv(loc_color, 1, u->color);
    }
}

void title_battery_fill_init(TitleBatteryFill *fill, SDL_Renderer *renderer)
{
    if (!fill) {
        return;
    }
    SDL_zerop(fill);

    if (!renderer || !gl_effect_context_acquire()) {
        return;
    }
    if (!gl_effect_target_create(&fill->target, renderer, TITLE_BATTERY_FILL_GL_W, TITLE_BATTERY_FILL_GL_H)) {
        gl_effect_context_release();
        return;
    }

    fill->program = gl_effect_compile_program(g_battery_fill_fragment_src);
    if (!fill->program) {
        gl_effect_target_destroy(&fill->target);
        gl_effect_context_release();
        return;
    }

    fill->ready = SDL_TRUE;
}

void title_battery_fill_shutdown(TitleBatteryFill *fill)
{
    if (!fill || !fill->ready) {
        return;
    }
    gl_effect_destroy_program(fill->program);
    fill->program = 0;
    gl_effect_target_destroy(&fill->target);
    gl_effect_context_release();
    fill->ready = SDL_FALSE;
}

void title_battery_fill_render(TitleBatteryFill *fill, SDL_Renderer *renderer,
                               SDL_Rect dst, int percent, SDL_Color color, SDL_bool charging)
{
    if (!fill || !fill->ready || !renderer) {
        return;
    }
    if (percent < 0) {
        percent = 0;
    } else if (percent > 100) {
        percent = 100;
    }

    TitleBatteryFillUniforms u;
    u.fill = percent / 100.0f;
    u.time = SDL_GetTicks() / 1000.0f;
    u.speed = charging ? 0.55f : 0.2f; /* faster scroll while charging */
    u.color[0] = color.r / 255.0f;
    u.color[1] = color.g / 255.0f;
    u.color[2] = color.b / 255.0f;

    gl_effect_render(&fill->target, fill->program, title_battery_fill_set_uniforms, &u);
    SDL_RenderCopy(renderer, fill->target.screen_texture, NULL, &dst);
}
