#ifndef RENDER_SUITE_STATE_H
#define RENDER_SUITE_STATE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "bench_common.h"

#define RS_SIN_TABLE_SIZE 512

typedef enum {
    SCENE_FILL = 0,
    SCENE_TEXTURE,
    SCENE_LINES,
    SCENE_GEOMETRY,
    SCENE_SCALING,
    SCENE_MEMORY,
    SCENE_PIXELS,
    SCENE_MAX
} SceneKind;

typedef enum {
    RS_GEOMETRY_RENDER_FILLED = 0,
    RS_GEOMETRY_RENDER_WIREFRAME,
    RS_GEOMETRY_RENDER_POINTS,
    RS_GEOMETRY_RENDER_MODE_MAX
} RSGeometryRenderMode;

typedef struct {
    SceneKind active_scene;
    SDL_bool auto_cycle;
    int stress_level;
    float texture_angle;
    float top_margin;
    TTF_Font *font;
    SDL_Texture *checker_texture;
    SDL_Texture *pixel_texture;

    float sin_table[RS_SIN_TABLE_SIZE];
    int sin_table_size;
    int sin_table_mask;

    float fill_phase_units;
    float texture_phase_units;
    float lines_cursor_progress;
    int lines_cursor_index;

    float geometry_rotation;
    int geometry_triangle_count;
    float geometry_phase;
    int geometry_render_mode;

    int scaling_current_width;
    int scaling_current_height;
    float scaling_phase;
    SDL_Texture **scaling_targets;
    int scaling_target_count;

    float resources_phase;
    SDL_Texture **resource_textures;
    int resource_texture_count;
    int resource_allocation_index;

    SDL_Surface *pixel_surface;
    void *pixel_buffer;
    float pixel_phase;
    int pixel_plasma_offset;

    SDL_bool has_neon;
} RenderSuiteState;

void rs_state_init(RenderSuiteState *state);
void rs_state_update_layout(RenderSuiteState *state, BenchOverlay *overlay);
void rs_state_destroy(RenderSuiteState *state, SDL_Renderer *renderer);

float rs_state_stress_factor(const RenderSuiteState *state);
float rs_state_sin(const RenderSuiteState *state, float units);
float rs_state_cos(const RenderSuiteState *state, float units);
float rs_state_sin_rad(const RenderSuiteState *state, float radians);
float rs_state_cos_rad(const RenderSuiteState *state, float radians);

#endif /* RENDER_SUITE_STATE_H */
