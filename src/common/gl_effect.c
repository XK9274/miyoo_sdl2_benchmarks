#include "common/gl_effect.h"

#include <SDL2/SDL_log.h>
#include <SDL2/SDL_opengles2.h>

#define GL_EFFECT_POSITION_LOC 0
#define GL_EFFECT_TEXCOORD_LOC 1

static const GLfloat g_quad[] = {
    -1.0f, -1.0f, 0.0f, 0.0f,
     1.0f, -1.0f, 1.0f, 0.0f,
     1.0f,  1.0f, 1.0f, 1.0f,
    -1.0f,  1.0f, 0.0f, 1.0f,
};

static const GLushort g_indices[] = {0, 1, 2, 0, 2, 3};

static const char *g_vertex_shader_src =
    "attribute vec2 a_position;\n"
    "attribute vec2 a_uv;\n"
    "varying vec2 v_uv;\n"
    "void main() {\n"
    "    gl_Position = vec4(a_position, 0.0, 1.0);\n"
    "    v_uv = a_uv;\n"
    "}\n";

static int g_refcount = 0;
static SDL_Window *g_window = NULL;
static SDL_GLContext g_context = NULL;
static SDL_bool g_library_loaded = SDL_FALSE;
static Uint32 g_vbo = 0;
static Uint32 g_ibo = 0;

static Uint32 gl_effect_compile(GLenum type, const char *source)
{
    Uint32 shader = glCreateShader(type);
    if (!shader) {
        return 0;
    }
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint status = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        char log[256];
        GLsizei len = 0;
        glGetShaderInfoLog(shader, sizeof(log) - 1, &len, log);
        log[len] = '\0';
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "gl_effect_compile: shader error %s", log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

SDL_bool gl_effect_context_acquire(void)
{
    if (g_refcount > 0) {
        g_refcount++;
        return SDL_TRUE;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 0);

    if (SDL_GL_LoadLibrary(NULL) != 0) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "gl_effect_context_acquire: failed to load GL library (%s)", SDL_GetError());
        return SDL_FALSE;
    }
    g_library_loaded = SDL_TRUE;

    g_window = SDL_CreateWindow("gl_effect",
                                SDL_WINDOWPOS_UNDEFINED,
                                SDL_WINDOWPOS_UNDEFINED,
                                64, 64,
                                SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if (!g_window) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "gl_effect_context_acquire: failed to create GL window (%s)", SDL_GetError());
        SDL_GL_UnloadLibrary();
        g_library_loaded = SDL_FALSE;
        return SDL_FALSE;
    }

    g_context = SDL_GL_CreateContext(g_window);
    if (!g_context) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "gl_effect_context_acquire: failed to create GL context (%s)", SDL_GetError());
        SDL_DestroyWindow(g_window);
        g_window = NULL;
        SDL_GL_UnloadLibrary();
        g_library_loaded = SDL_FALSE;
        return SDL_FALSE;
    }

    if (SDL_GL_MakeCurrent(g_window, g_context) != 0) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "gl_effect_context_acquire: make current failed (%s)", SDL_GetError());
        SDL_GL_DeleteContext(g_context);
        g_context = NULL;
        SDL_DestroyWindow(g_window);
        g_window = NULL;
        SDL_GL_UnloadLibrary();
        g_library_loaded = SDL_FALSE;
        return SDL_FALSE;
    }

    glGenBuffers(1, &g_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad), g_quad, GL_STATIC_DRAW);

    glGenBuffers(1, &g_ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(g_indices), g_indices, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    SDL_GL_MakeCurrent(g_window, NULL);

    g_refcount = 1;
    return SDL_TRUE;
}

void gl_effect_context_release(void)
{
    if (g_refcount <= 0) {
        return;
    }
    g_refcount--;
    if (g_refcount > 0) {
        return;
    }

    if (g_context && g_window && SDL_GL_MakeCurrent(g_window, g_context) == 0) {
        if (g_vbo) {
            glDeleteBuffers(1, &g_vbo);
            g_vbo = 0;
        }
        if (g_ibo) {
            glDeleteBuffers(1, &g_ibo);
            g_ibo = 0;
        }
        SDL_GL_MakeCurrent(g_window, NULL);
    }

    if (g_context) {
        SDL_GL_DeleteContext(g_context);
        g_context = NULL;
    }
    if (g_window) {
        SDL_DestroyWindow(g_window);
        g_window = NULL;
    }
    if (g_library_loaded) {
        SDL_GL_UnloadLibrary();
        g_library_loaded = SDL_FALSE;
    }
}

SDL_bool gl_effect_target_create(GLEffectTarget *target, SDL_Renderer *renderer, int width, int height)
{
    if (!target || !renderer || width <= 0 || height <= 0 || !g_context) {
        return SDL_FALSE;
    }
    SDL_zerop(target);
    target->width = width;
    target->height = height;

    const size_t required = (size_t)width * (size_t)height * 4u;
    target->pixel_buffer = (Uint8 *)SDL_malloc(required);
    if (!target->pixel_buffer) {
        return SDL_FALSE;
    }
    target->pixel_capacity = required;

    if (SDL_GL_MakeCurrent(g_window, g_context) != 0) {
        SDL_free(target->pixel_buffer);
        target->pixel_buffer = NULL;
        return SDL_FALSE;
    }

    glGenTextures(1, &target->color_texture);
    glBindTexture(GL_TEXTURE_2D, target->color_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenFramebuffers(1, &target->fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, target->fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target->color_texture, 0);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    SDL_GL_MakeCurrent(g_window, NULL);

    if (status != GL_FRAMEBUFFER_COMPLETE) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "gl_effect_target_create: framebuffer incomplete (0x%04x)", status);
        gl_effect_target_destroy(target);
        return SDL_FALSE;
    }

    target->screen_texture = SDL_CreateTexture(renderer,
                                               SDL_PIXELFORMAT_ABGR8888,
                                               SDL_TEXTUREACCESS_STREAMING,
                                               width, height);
    if (!target->screen_texture) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "gl_effect_target_create: failed to create SDL texture (%s)", SDL_GetError());
        gl_effect_target_destroy(target);
        return SDL_FALSE;
    }
    SDL_SetTextureBlendMode(target->screen_texture, SDL_BLENDMODE_BLEND);

    return SDL_TRUE;
}

void gl_effect_target_destroy(GLEffectTarget *target)
{
    if (!target) {
        return;
    }

    if (g_context && g_window && (target->fbo || target->color_texture)) {
        if (SDL_GL_MakeCurrent(g_window, g_context) == 0) {
            if (target->fbo) {
                glDeleteFramebuffers(1, &target->fbo);
                target->fbo = 0;
            }
            if (target->color_texture) {
                glDeleteTextures(1, &target->color_texture);
                target->color_texture = 0;
            }
            SDL_GL_MakeCurrent(g_window, NULL);
        }
    }

    if (target->screen_texture) {
        SDL_DestroyTexture(target->screen_texture);
        target->screen_texture = NULL;
    }
    if (target->pixel_buffer) {
        SDL_free(target->pixel_buffer);
        target->pixel_buffer = NULL;
        target->pixel_capacity = 0;
    }
}

Uint32 gl_effect_compile_program(const char *fragment_src)
{
    if (!g_context || !fragment_src) {
        return 0;
    }
    if (SDL_GL_MakeCurrent(g_window, g_context) != 0) {
        return 0;
    }

    Uint32 vs = gl_effect_compile(GL_VERTEX_SHADER, g_vertex_shader_src);
    if (!vs) {
        SDL_GL_MakeCurrent(g_window, NULL);
        return 0;
    }
    Uint32 fs = gl_effect_compile(GL_FRAGMENT_SHADER, fragment_src);
    if (!fs) {
        glDeleteShader(vs);
        SDL_GL_MakeCurrent(g_window, NULL);
        return 0;
    }

    Uint32 result = 0;
    Uint32 program = glCreateProgram();
    if (program) {
        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glBindAttribLocation(program, GL_EFFECT_POSITION_LOC, "a_position");
        glBindAttribLocation(program, GL_EFFECT_TEXCOORD_LOC, "a_uv");
        glLinkProgram(program);

        GLint status = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &status);
        if (status == GL_TRUE) {
            glDetachShader(program, vs);
            glDetachShader(program, fs);
            result = program;
        } else {
            char log[256];
            GLsizei len = 0;
            glGetProgramInfoLog(program, sizeof(log) - 1, &len, log);
            log[len] = '\0';
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "gl_effect_compile_program: link error %s", log);
            glDeleteProgram(program);
        }
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    SDL_GL_MakeCurrent(g_window, NULL);
    return result;
}

void gl_effect_destroy_program(Uint32 program)
{
    if (!program || !g_context || !g_window) {
        return;
    }
    if (SDL_GL_MakeCurrent(g_window, g_context) == 0) {
        glDeleteProgram(program);
        SDL_GL_MakeCurrent(g_window, NULL);
    }
}

void gl_effect_render(GLEffectTarget *target, Uint32 program, GLEffectSetUniforms set_uniforms, void *userdata)
{
    if (!target || !program || !g_context || !target->fbo || !target->pixel_buffer) {
        return;
    }
    if (SDL_GL_MakeCurrent(g_window, g_context) != 0) {
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, target->fbo);
    glViewport(0, 0, target->width, target->height);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(program);
    if (set_uniforms) {
        set_uniforms(program, userdata);
    }

    glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ibo);

    glEnableVertexAttribArray(GL_EFFECT_POSITION_LOC);
    glVertexAttribPointer(GL_EFFECT_POSITION_LOC, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4, (const void *)0);

    glEnableVertexAttribArray(GL_EFFECT_TEXCOORD_LOC);
    glVertexAttribPointer(GL_EFFECT_TEXCOORD_LOC, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4,
                          (const void *)(sizeof(GLfloat) * 2));

    glDrawElements(GL_TRIANGLES, (GLsizei)(sizeof(g_indices) / sizeof(g_indices[0])), GL_UNSIGNED_SHORT, (const void *)0);

    glDisableVertexAttribArray(GL_EFFECT_TEXCOORD_LOC);
    glDisableVertexAttribArray(GL_EFFECT_POSITION_LOC);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glUseProgram(0);
    glDisable(GL_BLEND);

    glReadPixels(0, 0, target->width, target->height, GL_RGBA, GL_UNSIGNED_BYTE, target->pixel_buffer);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    SDL_GL_MakeCurrent(g_window, NULL);

    if (!target->screen_texture) {
        return;
    }

    void *pixels = NULL;
    int pitch = 0;
    if (SDL_LockTexture(target->screen_texture, NULL, &pixels, &pitch) != 0) {
        return;
    }

    Uint8 *dst = (Uint8 *)pixels;
    const Uint8 *src = target->pixel_buffer;
    const int src_stride = target->width * 4;
    for (int y = 0; y < target->height; ++y) {
        Uint8 *row = dst + y * pitch;
        const Uint8 *src_row = src + (target->height - 1 - y) * src_stride;
        SDL_memcpy(row, src_row, src_stride);
    }

    SDL_UnlockTexture(target->screen_texture);
}
