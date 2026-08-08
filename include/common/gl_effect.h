#ifndef COMMON_GL_EFFECT_H
#define COMMON_GL_EFFECT_H

#include <SDL2/SDL.h>

/* Shared, refcounted GLES2 context. */
SDL_bool gl_effect_context_acquire(void);
void gl_effect_context_release(void);

/* Small FBO + backing SDL texture. */
typedef struct {
    int width;
    int height;
    Uint32 fbo;
    Uint32 color_texture;
    SDL_Texture *screen_texture;
    Uint8 *pixel_buffer;
    size_t pixel_capacity;
} GLEffectTarget;

SDL_bool gl_effect_target_create(GLEffectTarget *target, SDL_Renderer *renderer, int width, int height);
void gl_effect_target_destroy(GLEffectTarget *target);

Uint32 gl_effect_compile_program(const char *fragment_src);
void gl_effect_destroy_program(Uint32 program);

typedef void (*GLEffectSetUniforms)(Uint32 program, void *userdata);

/* Call once per effect type per frame, not per instance. */
void gl_effect_render(GLEffectTarget *target,
                      Uint32 program,
                      GLEffectSetUniforms set_uniforms,
                      void *userdata);

#endif /* COMMON_GL_EFFECT_H */
