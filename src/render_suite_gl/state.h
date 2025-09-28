#ifndef RENDER_SUITE_GL_STATE_H
#define RENDER_SUITE_GL_STATE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "bench_common.h"

#define RSGL_EFFECT_MAX 15

typedef struct {
    SDL_bool running;
    SDL_bool auto_cycle;
    int effect_index;
    int effect_count;
    float elapsed_time;
    float top_margin;

    TTF_Font *font;
    SDL_Texture *screen_texture;
    Uint8 *pixel_buffer;
    size_t pixel_capacity;

    int screen_width;
    int screen_height;

    SDL_Window *gl_window;
    SDL_GLContext gl_context;
    Uint32 gl_vbo;
    Uint32 gl_ibo;
    Uint32 gl_fbo;
    Uint32 gl_color_texture;
    SDL_bool gl_ready;
    SDL_bool gl_library_loaded;
    SDL_bool gl_external;
} RsglState;

void rsgl_state_init(RsglState *state);
void rsgl_state_destroy(RsglState *state);
void rsgl_state_update_layout(RsglState *state, BenchOverlay *overlay);

#endif /* RENDER_SUITE_GL_STATE_H */
