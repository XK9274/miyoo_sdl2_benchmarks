#include "common/loading_screen.h"

#include <SDL2/SDL_log.h>
#include <SDL2/SDL_opengles2.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "common/overlay.h"

#define LOADING_GL_WIDTH 160
#define LOADING_GL_HEIGHT 120
#define LOADING_MESSAGE_MAX 95

static const GLfloat g_loading_vertices[] = {
    -1.0f, -1.0f, 0.0f, 0.0f,
     1.0f, -1.0f, 1.0f, 0.0f,
     1.0f,  1.0f, 1.0f, 1.0f,
    -1.0f,  1.0f, 0.0f, 1.0f,
};

static const GLushort g_loading_indices[] = {0, 1, 2, 0, 2, 3};

static const char *g_loading_vertex_shader =
    "attribute vec2 a_position;\n"
    "attribute vec2 a_uv;\n"
    "varying vec2 v_uv;\n"
    "void main() {\n"
    "    gl_Position = vec4(a_position, 0.0, 1.0);\n"
    "    v_uv = a_uv;\n"
    "}\n";

static const char *g_loading_fragment_shader =
    "precision mediump float;\n"
    "varying vec2 v_uv;\n"
    "uniform float u_time;\n"
    "uniform float u_progress;\n"
    "void main() {\n"
    "    vec2 uv = v_uv - 0.5;\n"
    "    float angle = atan(uv.y, uv.x);\n"
    "    float radius = length(uv);\n"
    "    float swirl = sin(angle * 4.0 + u_time * 2.4) * 0.2;\n"
    "    float band = smoothstep(0.45 + swirl * 0.05, 0.0, radius);\n"
    "    float pulse = 0.5 + 0.5 * sin(u_time * 3.0 + radius * 12.0);\n"
    "    vec3 base = mix(vec3(0.04, 0.08, 0.16), vec3(0.15, 0.18, 0.32), radius + pulse * 0.05);\n"
    "    float progress_mask = smoothstep(u_progress - 0.02, u_progress + 0.02, v_uv.x);\n"
    "    vec3 progress_color = mix(vec3(0.08, 0.18, 0.36), vec3(0.08, 0.8, 0.9), progress_mask);\n"
    "    vec3 color = base + vec3(band) * 0.2 + progress_color * 0.6;\n"
    "    color += vec3(0.2, 0.3, 0.6) * smoothstep(0.48, 0.5, v_uv.y + sin(u_time + v_uv.x * 6.0) * 0.02);\n"
    "    gl_FragColor = vec4(color, 1.0);\n"
    "}\n";

static void bench_loading_reset(BenchLoadingScreen *screen)
{
    if (!screen) {
        return;
    }
    SDL_memset(screen, 0, sizeof(*screen));
}

static void bench_loading_clear_gl_stage(BenchLoadingScreen *screen)
{
    if (!screen) {
        return;
    }
    if (screen->gl_stage_texture) {
        SDL_DestroyTexture(screen->gl_stage_texture);
        screen->gl_stage_texture = NULL;
    }
    if (screen->gl_pixels) {
        SDL_free(screen->gl_pixels);
        screen->gl_pixels = NULL;
        screen->gl_capacity = 0;
    }
}

static void bench_loading_release_gl_pipeline(BenchLoadingScreen *screen)
{
    if (!screen || !screen->gl_ready) {
        return;
    }

    if (screen->gl_window && screen->gl_context) {
        if (SDL_GL_MakeCurrent(screen->gl_window, screen->gl_context) == 0) {
            if (screen->gl_fbo) {
                glDeleteFramebuffers(1, &screen->gl_fbo);
                screen->gl_fbo = 0;
            }
            if (screen->gl_color_texture) {
                glDeleteTextures(1, &screen->gl_color_texture);
                screen->gl_color_texture = 0;
            }
            if (screen->gl_vbo) {
                glDeleteBuffers(1, &screen->gl_vbo);
                screen->gl_vbo = 0;
            }
            if (screen->gl_ibo) {
                glDeleteBuffers(1, &screen->gl_ibo);
                screen->gl_ibo = 0;
            }
            if (screen->gl_program) {
                glDeleteProgram(screen->gl_program);
                screen->gl_program = 0;
            }
        }
        SDL_GL_MakeCurrent(screen->gl_window, NULL);
    }

    screen->gl_ready = SDL_FALSE;
    screen->gl_time_accum = 0.0f;
}

static void bench_loading_destroy_gl(BenchLoadingScreen *screen)
{
    if (!screen) {
        return;
    }

    bench_loading_release_gl_pipeline(screen);
    bench_loading_clear_gl_stage(screen);

    if (!screen->gl_transferred) {
        if (screen->gl_context) {
            SDL_GL_DeleteContext(screen->gl_context);
        }
        if (screen->gl_window) {
            SDL_DestroyWindow(screen->gl_window);
        }
    }

    screen->gl_context = NULL;
    screen->gl_window = NULL;

    if (screen->gl_library_loaded && screen->gl_library_owned) {
        SDL_GL_UnloadLibrary();
        screen->gl_library_loaded = SDL_FALSE;
        screen->gl_library_owned = SDL_FALSE;
    }
}

static Uint32 bench_loading_compile(GLenum type, const char *source)
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
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "bench_loading_compile: shader error %s",
                    log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static SDL_bool bench_loading_setup_gl(BenchLoadingScreen *screen)
{
    if (!screen || !screen->renderer || !screen->window) {
        return SDL_FALSE;
    }

    if (!screen->gl_library_loaded) {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 0);
        if (SDL_GL_LoadLibrary(NULL) != 0) {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                        "bench_loading_setup_gl: failed to load GL library (%s)",
                        SDL_GetError());
            return SDL_FALSE;
        }
        screen->gl_library_loaded = SDL_TRUE;
        screen->gl_library_owned = SDL_TRUE;
    }

    screen->gl_window = SDL_CreateWindow("bench-loading-gl",
                                         SDL_WINDOWPOS_UNDEFINED,
                                         SDL_WINDOWPOS_UNDEFINED,
                                         LOADING_GL_WIDTH,
                                         LOADING_GL_HEIGHT,
                                         SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if (!screen->gl_window) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "bench_loading_setup_gl: failed to create GL window (%s)",
                    SDL_GetError());
        return SDL_FALSE;
    }

    screen->gl_context = SDL_GL_CreateContext(screen->gl_window);
    if (!screen->gl_context) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "bench_loading_setup_gl: failed to create GL context (%s)",
                    SDL_GetError());
        return SDL_FALSE;
    }

    if (SDL_GL_MakeCurrent(screen->gl_window, screen->gl_context) != 0) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "bench_loading_setup_gl: make current failed (%s)",
                    SDL_GetError());
        return SDL_FALSE;
    }

    Uint32 vs = bench_loading_compile(GL_VERTEX_SHADER, g_loading_vertex_shader);
    Uint32 fs = bench_loading_compile(GL_FRAGMENT_SHADER, g_loading_fragment_shader);
    if (!vs || !fs) {
        if (vs) {
            glDeleteShader(vs);
        }
        if (fs) {
            glDeleteShader(fs);
        }
        SDL_GL_MakeCurrent(screen->gl_window, NULL);
        return SDL_FALSE;
    }

    screen->gl_program = glCreateProgram();
    if (!screen->gl_program) {
        glDeleteShader(vs);
        glDeleteShader(fs);
        SDL_GL_MakeCurrent(screen->gl_window, NULL);
        return SDL_FALSE;
    }

    glAttachShader(screen->gl_program, vs);
    glAttachShader(screen->gl_program, fs);
    glBindAttribLocation(screen->gl_program, 0, "a_position");
    glBindAttribLocation(screen->gl_program, 1, "a_uv");
    glLinkProgram(screen->gl_program);

    GLint link_status = GL_FALSE;
    glGetProgramiv(screen->gl_program, GL_LINK_STATUS, &link_status);
    if (link_status != GL_TRUE) {
        char log[256];
        GLsizei len = 0;
        glGetProgramInfoLog(screen->gl_program, sizeof(log) - 1, &len, log);
        log[len] = '\0';
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "bench_loading_setup_gl: link error %s",
                    log);
        glDeleteProgram(screen->gl_program);
        screen->gl_program = 0;
        glDeleteShader(vs);
        glDeleteShader(fs);
        SDL_GL_MakeCurrent(screen->gl_window, NULL);
        return SDL_FALSE;
    }

    glDetachShader(screen->gl_program, vs);
    glDetachShader(screen->gl_program, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);

    glGenBuffers(1, &screen->gl_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, screen->gl_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_loading_vertices), g_loading_vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &screen->gl_ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, screen->gl_ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(g_loading_indices), g_loading_indices, GL_STATIC_DRAW);

    glGenTextures(1, &screen->gl_color_texture);
    glBindTexture(GL_TEXTURE_2D, screen->gl_color_texture);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 LOADING_GL_WIDTH,
                 LOADING_GL_HEIGHT,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenFramebuffers(1, &screen->gl_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, screen->gl_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D,
                           screen->gl_color_texture,
                           0);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    if (status != GL_FRAMEBUFFER_COMPLETE) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "bench_loading_setup_gl: framebuffer incomplete (0x%04x)",
                    status);
        SDL_GL_MakeCurrent(screen->gl_window, NULL);
        return SDL_FALSE;
    }

    screen->gl_uniform_time = glGetUniformLocation(screen->gl_program, "u_time");
    screen->gl_uniform_progress = glGetUniformLocation(screen->gl_program, "u_progress");

    screen->gl_width = LOADING_GL_WIDTH;
    screen->gl_height = LOADING_GL_HEIGHT;

    const size_t required = (size_t)screen->gl_width * (size_t)screen->gl_height * 4u;
    screen->gl_pixels = (Uint8 *)SDL_malloc(required);
    if (!screen->gl_pixels) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "bench_loading_setup_gl: pixel allocation failed (%zu bytes)",
                    required);
        SDL_GL_MakeCurrent(screen->gl_window, NULL);
        return SDL_FALSE;
    }
    screen->gl_capacity = required;

    screen->gl_stage_texture = SDL_CreateTexture(screen->renderer,
                                                 SDL_PIXELFORMAT_ABGR8888,
                                                 SDL_TEXTUREACCESS_STREAMING,
                                                 screen->gl_width,
                                                 screen->gl_height);
    if (!screen->gl_stage_texture) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "bench_loading_setup_gl: SDL texture creation failed (%s)",
                    SDL_GetError());
        SDL_GL_MakeCurrent(screen->gl_window, NULL);
        return SDL_FALSE;
    }

    screen->gl_ready = SDL_TRUE;
    SDL_GL_MakeCurrent(screen->gl_window, NULL);
    return SDL_TRUE;
}

static void bench_loading_update_gl(BenchLoadingScreen *screen)
{
    if (!screen->gl_ready) {
        return;
    }

    if (SDL_GL_MakeCurrent(screen->gl_window, screen->gl_context) != 0) {
        return;
    }

    const Uint64 now = SDL_GetPerformanceCounter();
    double delta = 0.0;
    if (screen->last_counter != 0) {
        delta = (double)(now - screen->last_counter) / (double)screen->perf_freq;
    }
    screen->last_counter = now;
    screen->gl_time_accum += (float)delta;

    glBindFramebuffer(GL_FRAMEBUFFER, screen->gl_fbo);
    glViewport(0, 0, screen->gl_width, screen->gl_height);
    glClearColor(0.04f, 0.05f, 0.09f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(screen->gl_program);
    if (screen->gl_uniform_time >= 0) {
        glUniform1f(screen->gl_uniform_time, screen->gl_time_accum);
    }
    if (screen->gl_uniform_progress >= 0) {
        glUniform1f(screen->gl_uniform_progress, screen->progress);
    }

    glBindBuffer(GL_ARRAY_BUFFER, screen->gl_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, screen->gl_ibo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4, (const void *)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,
                          2,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(GLfloat) * 4,
                          (const void *)(sizeof(GLfloat) * 2));

    glDrawElements(GL_TRIANGLES,
                   (GLsizei)(sizeof(g_loading_indices) / sizeof(g_loading_indices[0])),
                   GL_UNSIGNED_SHORT,
                   (const void *)0);

    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glUseProgram(0);

    if (screen->gl_pixels && screen->gl_stage_texture) {
        glReadPixels(0,
                     0,
                     screen->gl_width,
                     screen->gl_height,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     screen->gl_pixels);

        void *pixels = NULL;
        int pitch = 0;
        if (SDL_LockTexture(screen->gl_stage_texture, NULL, &pixels, &pitch) == 0) {
            Uint8 *dst = (Uint8 *)pixels;
            const Uint8 *src = screen->gl_pixels;
            const int src_stride = screen->gl_width * 4;
            for (int y = 0; y < screen->gl_height; ++y) {
                Uint8 *row = dst + y * pitch;
                const Uint8 *src_row = src + (screen->gl_height - 1 - y) * src_stride;
                SDL_memcpy(row, src_row, (size_t)src_stride);
            }
            SDL_UnlockTexture(screen->gl_stage_texture);
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    SDL_GL_MakeCurrent(screen->gl_window, NULL);
}

static void bench_loading_render_message(BenchLoadingScreen *screen,
                                         int renderer_w,
                                         int renderer_h)
{
    if (!screen->font || screen->message[0] == '\0') {
        return;
    }

    SDL_Surface *surface = TTF_RenderUTF8_Blended(screen->font,
                                                  screen->message,
                                                  screen->text_color);
    if (!surface) {
        return;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(screen->renderer, surface);
    if (!texture) {
        SDL_FreeSurface(surface);
        return;
    }

    SDL_Rect dst;
    dst.w = surface->w;
    dst.h = surface->h;
    dst.x = (renderer_w - dst.w) / 2;
    dst.y = renderer_h / 2 - dst.h - 36;

    SDL_FreeSurface(surface);

    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_RenderCopy(screen->renderer, texture, NULL, &dst);
    SDL_DestroyTexture(texture);
}

static void bench_loading_render_bar(BenchLoadingScreen *screen,
                                     int renderer_w,
                                     int renderer_h)
{
    const int bar_width = renderer_w / 2;
    const int bar_height = 20;
    SDL_Rect outline = {
        (renderer_w - bar_width) / 2,
        renderer_h - bar_height - 48,
        bar_width,
        bar_height
    };

    SDL_SetRenderDrawBlendMode(screen->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(screen->renderer,
                           screen->bar_outline.r,
                           screen->bar_outline.g,
                           screen->bar_outline.b,
                           screen->bar_outline.a);
    SDL_RenderFillRect(screen->renderer, &outline);

    SDL_Rect inner = {
        outline.x + 2,
        outline.y + 2,
        outline.w - 4,
        outline.h - 4
    };
    SDL_SetRenderDrawColor(screen->renderer, 12, 18, 32, 200);
    SDL_RenderFillRect(screen->renderer, &inner);

    SDL_Rect fill = inner;
    fill.w = (int)((float)fill.w * SDL_clamp(screen->progress, 0.0f, 1.0f));
    SDL_SetRenderDrawColor(screen->renderer,
                           screen->bar_fill.r,
                           screen->bar_fill.g,
                           screen->bar_fill.b,
                           screen->bar_fill.a);
    SDL_RenderFillRect(screen->renderer, &fill);
}

static void bench_loading_render_percent(BenchLoadingScreen *screen,
                                         int renderer_w,
                                         int renderer_h)
{
    if (!screen->font) {
        return;
    }

    char buffer[32];
    SDL_snprintf(buffer, sizeof(buffer), "%d%%", (int)(screen->progress * 100.0f));

    SDL_Surface *surface = TTF_RenderUTF8_Blended(screen->font, buffer, screen->text_color);
    if (!surface) {
        return;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(screen->renderer, surface);
    if (!texture) {
        SDL_FreeSurface(surface);
        return;
    }

    SDL_Rect dst;
    dst.w = surface->w;
    dst.h = surface->h;
    dst.x = (renderer_w - dst.w) / 2;
    dst.y = renderer_h - dst.h - 80;

    SDL_FreeSurface(surface);

    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_RenderCopy(screen->renderer, texture, NULL, &dst);
    SDL_DestroyTexture(texture);
}

static void bench_loading_present(BenchLoadingScreen *screen)
{
    if (!screen || !screen->renderer) {
        return;
    }

    int w = 0;
    int h = 0;
    SDL_GetRendererOutputSize(screen->renderer, &w, &h);

    SDL_SetRenderDrawBlendMode(screen->renderer, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(screen->renderer,
                           screen->background.r,
                           screen->background.g,
                           screen->background.b,
                           screen->background.a);
    SDL_RenderClear(screen->renderer);

    if (screen->style == BENCH_LOADING_STYLE_GL && screen->gl_ready && screen->gl_stage_texture) {
        bench_loading_update_gl(screen);
        SDL_Rect dst = {0, 0, w, h};
        SDL_RenderCopy(screen->renderer, screen->gl_stage_texture, NULL, &dst);
    }

    bench_loading_render_bar(screen, w, h);
    bench_loading_render_percent(screen, w, h);
    bench_loading_render_message(screen, w, h);

    SDL_RenderPresent(screen->renderer);
}

SDL_bool bench_loading_begin(BenchLoadingScreen *screen,
                             SDL_Window *window,
                             SDL_Renderer *renderer,
                             BenchLoadingStyle style)
{
    if (!screen || !window || !renderer) {
        return SDL_FALSE;
    }

    bench_loading_reset(screen);

    screen->window = window;
    screen->renderer = renderer;
    screen->style = style;
    screen->active = SDL_TRUE;
    screen->progress = 0.0f;
    screen->background = (SDL_Color){8, 10, 18, 255};
    screen->bar_outline = (SDL_Color){28, 42, 62, 255};
    screen->bar_fill = (SDL_Color){0, 210, 180, 255};
    screen->text_color = (SDL_Color){220, 230, 255, 255};
    screen->start_ticks = SDL_GetTicks64();
    screen->last_ticks = screen->start_ticks;
    screen->perf_freq = SDL_GetPerformanceFrequency();
    screen->last_counter = SDL_GetPerformanceCounter();

    screen->font = bench_load_font(14);
    screen->owns_font = (screen->font != NULL);

    if (style == BENCH_LOADING_STYLE_GL) {
        if (!bench_loading_setup_gl(screen)) {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                        "bench_loading_begin: falling back to rect style");
            bench_loading_destroy_gl(screen);
            screen->style = BENCH_LOADING_STYLE_RECT;
        }
    }

    bench_loading_present(screen);
    return SDL_TRUE;
}

void bench_loading_step(BenchLoadingScreen *screen,
                        float progress,
                        const char *label)
{
    if (!screen || !screen->active) {
        return;
    }

    screen->progress = SDL_clamp(progress, 0.0f, 1.0f);
    if (label && label[0] != '\0') {
        SDL_strlcpy(screen->message, label, sizeof(screen->message));
    }

    bench_loading_present(screen);
    SDL_PumpEvents();
}

void bench_loading_mark_idle(BenchLoadingScreen *screen,
                             const char *label)
{
    if (!screen || !screen->active) {
        return;
    }
    screen->progress = 1.0f;
    if (label && label[0] != '\0') {
        SDL_strlcpy(screen->message, label, sizeof(screen->message));
    } else {
        SDL_strlcpy(screen->message,
                    "GL modules idle - awaiting additional workloads",
                    sizeof(screen->message));
    }
    bench_loading_present(screen);
    SDL_Delay(120);
}

void bench_loading_finish(BenchLoadingScreen *screen)
{
    if (!screen || !screen->active) {
        return;
    }

    screen->progress = 1.0f;
    if (screen->message[0] == '\0') {
        SDL_strlcpy(screen->message, "Loading complete", sizeof(screen->message));
    }
    bench_loading_present(screen);
    SDL_Delay(90);

    if (screen->owns_font && screen->font) {
        TTF_CloseFont(screen->font);
    }
    screen->font = NULL;
    screen->owns_font = SDL_FALSE;

    bench_loading_destroy_gl(screen);
    screen->active = SDL_FALSE;
}

void bench_loading_abort(BenchLoadingScreen *screen)
{
    if (!screen || !screen->active) {
        return;
    }

    if (screen->owns_font && screen->font) {
        TTF_CloseFont(screen->font);
    }
    screen->font = NULL;
    screen->owns_font = SDL_FALSE;

    bench_loading_destroy_gl(screen);
    screen->active = SDL_FALSE;
}

SDL_bool bench_loading_obtain_gl(BenchLoadingScreen *screen,
                                 SDL_Window **out_window,
                                 SDL_GLContext *out_context)
{
    if (!screen || screen->style != BENCH_LOADING_STYLE_GL || !screen->gl_ready || screen->gl_transferred) {
        return SDL_FALSE;
    }

    bench_loading_release_gl_pipeline(screen);
    bench_loading_clear_gl_stage(screen);

    if (!screen->gl_window || !screen->gl_context) {
        return SDL_FALSE;
    }

    if (out_window) {
        *out_window = screen->gl_window;
    }
    if (out_context) {
        *out_context = screen->gl_context;
    }

    screen->gl_window = NULL;
    screen->gl_context = NULL;
    screen->gl_transferred = SDL_TRUE;
    screen->style = BENCH_LOADING_STYLE_RECT;
    screen->gl_library_owned = SDL_FALSE;

    return SDL_TRUE;
}
