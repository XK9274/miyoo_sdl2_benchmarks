#ifndef SPACE_BENCH_GL_EFFECTS_H
#define SPACE_BENCH_GL_EFFECTS_H

#include <SDL2/SDL.h>

/* Small, cheap GLES2 effects for the space game: plasma bolt projectiles,
 * upgrade pickups, the thumper pulse, and the shield. Each effect is a
 * single small texture rendered once per frame (see space_gl_effects_update)
 * and blitted many times via SDL_RenderCopy/SDL_RenderCopyEx with
 * SDL_SetTextureColorMod for per-instance colour, instead of one GL render
 * per on-screen instance -- that's what keeps this cheap regardless of how
 * many bolts/pickups are alive. Effects with nothing active this frame skip
 * their GL render entirely (see the *_active fields below) rather than
 * re-rendering an unused texture. Uses the shared context in
 * common/gl_effect.h. */

typedef struct {
    float time_accumulator;
    SDL_bool bolts_active;
    SDL_bool pickups_active;
    float thumper_progress;   /* < 0 = no pulse active this frame, skip */
    SDL_bool shield_active;   /* player shield OR anomaly shield */
} SpaceGLEffectsFrameInput;

SDL_bool space_gl_effects_init(SDL_Renderer *renderer);

/* Forces one real draw through every effect's GL program/FBO/texture path,
 * once, so the driver's first-use cold path (not just shader compilation)
 * happens here instead of on whichever frame first needs an effect during
 * gameplay. Call once after space_gl_effects_init succeeds, while a loading
 * screen is still showing. */
void space_gl_effects_warmup(void);

void space_gl_effects_shutdown(void);
SDL_bool space_gl_effects_ready(void);

/* Call once per frame, before any per-instance blits. */
void space_gl_effects_update(const SpaceGLEffectsFrameInput *input);

SDL_Texture *space_gl_effect_bolt_texture(void);
SDL_Texture *space_gl_effect_pickup_texture(void);
SDL_Texture *space_gl_effect_thumper_texture(void); /* NULL if not active this frame */
SDL_Texture *space_gl_effect_shield_texture(void);

#endif /* SPACE_BENCH_GL_EFFECTS_H */
