#ifndef OBJ_MODEL_LOADER_STATE_H
#define OBJ_MODEL_LOADER_STATE_H

#include <SDL2/SDL.h>

#include "bench_common.h"
#include "common/render3d/camera.h"
#include "common/render3d/model_instance.h"
#include "common/render3d/pipeline.h"

#define OBJ_MODEL_MAX_COUNT 16
#define OBJ_MODEL_NAME_LEN 64

typedef struct {
    ModelInstance model;
    char model_label[128]; /* shown on the HUD: bundled model path, or "placeholder cube" */

    Camera3D camera;
    SDL_bool auto_rotate;
    float auto_rotate_radians_per_second;

    SDL_bool wireframe;
    float ambient_floor;

    Render3DScratch *scratch;

    float top_margin;

    /* Every assets/models/<name>/<name>.obj found at startup, sorted
     * alphabetically. R2/L2 cycle current_model_index through this list. */
    char available_models[OBJ_MODEL_MAX_COUNT][OBJ_MODEL_NAME_LEN];
    int available_model_count;
    int current_model_index;
} ObjModelLoaderState;

/* Scans for bundled models, then loads OBJ_MODEL_NAME (or "sheep", or
 * whatever was found). Falls back to the built-in placeholder cube on any
 * load failure or if nothing was found. Always leaves state renderable. */
void obj_state_init(SDL_Renderer *renderer, ObjModelLoaderState *state);
void obj_state_destroy(SDL_Renderer *renderer, ObjModelLoaderState *state);
void obj_state_update_layout(ObjModelLoaderState *state, BenchOverlay *overlay);

/* Cycles to the next/previous discovered model. No-op if fewer than 2
 * models were found. */
void obj_state_cycle_model(SDL_Renderer *renderer, ObjModelLoaderState *state, int direction);

#endif /* OBJ_MODEL_LOADER_STATE_H */
