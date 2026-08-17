#ifndef SPACE_BENCH_GL_EFFECTS_H
#define SPACE_BENCH_GL_EFFECTS_H

#include <SDL2/SDL.h>

/* Small GLES2 effects for the space game: bolts, pickups, thumper, shield.
 * One texture per effect, tinted per-instance via SDL_SetTextureColorMod. */

typedef struct {
    float time_accumulator;
    SDL_bool bolts_active;
    SDL_bool pickups_active;
    float thumper_progress;   /* < 0 = no pulse active this frame, skip */
    SDL_bool shield_active;   /* player shield OR anomaly shield */
    SDL_bool lasers_active;   /* any beam (player or drone) firing this frame */
} SpaceGLEffectsFrameInput;

SDL_bool space_gl_effects_init(SDL_Renderer *renderer);

/* Pre-warms the GL driver's first-draw cost. Call after init. */
void space_gl_effects_warmup(void);

void space_gl_effects_shutdown(void);
SDL_bool space_gl_effects_ready(void);

/* Call once per frame, before any per-instance blits. */
void space_gl_effects_update(const SpaceGLEffectsFrameInput *input);

SDL_Texture *space_gl_effect_bolt_texture(void);
SDL_Texture *space_gl_effect_pickup_texture(void);
SDL_Texture *space_gl_effect_thumper_texture(void); /* NULL if not active this frame */
SDL_Texture *space_gl_effect_shield_texture(void);

/* Beam laser cross-section strips: gradient in the V axis, tiled horizontally by the caller (space_render_laser_beam) at SPACE_GL_LASER_TILE_W px/tile. Draw order glow -> edge -> core. NULL if no beam fired this frame. */
#define SPACE_GL_LASER_TILE_W 64
SDL_Texture *space_gl_effect_laser_glow_texture(void);
SDL_Texture *space_gl_effect_laser_edge_texture(void);
SDL_Texture *space_gl_effect_laser_core_texture(void);

#endif /* SPACE_BENCH_GL_EFFECTS_H */
