#ifndef COMMON_GL_EFFECT_H
#define COMMON_GL_EFFECT_H

#include <SDL2/SDL.h>

/* Shared, refcounted GLES2 context for small offscreen effects. A single
 * hidden window + context is created on first acquire and torn down on the
 * last release, so callers never each stand up their own GL state -- see
 * render_suite_gl/scenes/effects.c for the (duplicated, per-effect) pattern
 * this was extracted from. */
SDL_bool gl_effect_context_acquire(void);
void gl_effect_context_release(void);

/* A small FBO + backing SDL texture pair. Effects render into `fbo` at
 * `width`x`height`, and the result is read back and uploaded into
 * `screen_texture` for the caller to blit through their normal 2D
 * SDL_Renderer (SDL_RenderCopy/SDL_RenderCopyEx). */
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

/* Compiles `fragment_src` against the shared fullscreen-quad vertex shader
 * and returns a linked GL program, or 0 on failure (logged). */
Uint32 gl_effect_compile_program(const char *fragment_src);
void gl_effect_destroy_program(Uint32 program);

typedef void (*GLEffectSetUniforms)(Uint32 program, void *userdata);

/* Binds target's FBO, clears to transparent, draws the shared fullscreen
 * quad with `program` (calling set_uniforms first so the caller can set its
 * own uniforms), reads the result back, and uploads it into
 * target->screen_texture. Call this once per frame per effect *type*, not
 * once per on-screen instance -- colour/alpha variation between instances
 * should come from SDL_SetTextureColorMod/SDL_SetTextureAlphaMod on the
 * shared screen_texture at blit time instead, to keep this cheap. */
void gl_effect_render(GLEffectTarget *target,
                      Uint32 program,
                      GLEffectSetUniforms set_uniforms,
                      void *userdata);

#endif /* COMMON_GL_EFFECT_H */
