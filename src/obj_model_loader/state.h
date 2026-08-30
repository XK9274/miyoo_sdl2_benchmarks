#ifndef OBJ_MODEL_LOADER_STATE_H
#define OBJ_MODEL_LOADER_STATE_H

#include <SDL2/SDL.h>

#include "bench_common.h"
#include "common/render3d/camera.h"
#include "common/render3d/model_instance.h"
#include "common/render3d/pipeline.h"

typedef struct {
    ModelInstance model;
    char model_label[64]; /* shown on the HUD: bundled model path, or "placeholder cube" */

    Camera3D camera;
    SDL_bool auto_rotate;
    float auto_rotate_radians_per_second;

    SDL_bool wireframe;
    float ambient_floor;

    Render3DScratch *scratch;

    float top_margin;
} ObjModelLoaderState;

/* Tries the bundled/user-supplied .obj first; falls back to the built-in
 * placeholder cube on any load failure. Always leaves state renderable. */
void obj_state_init(SDL_Renderer *renderer, ObjModelLoaderState *state);
void obj_state_destroy(SDL_Renderer *renderer, ObjModelLoaderState *state);
void obj_state_update_layout(ObjModelLoaderState *state, BenchOverlay *overlay);

#endif /* OBJ_MODEL_LOADER_STATE_H */
