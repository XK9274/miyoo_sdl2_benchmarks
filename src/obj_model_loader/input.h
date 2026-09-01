#ifndef OBJ_MODEL_LOADER_INPUT_H
#define OBJ_MODEL_LOADER_INPUT_H

#include <SDL2/SDL.h>

#include "bench_common.h"
#include "obj_model_loader/state.h"

SDL_bool obj_handle_input(SDL_Renderer *renderer, ObjModelLoaderState *state, BenchMetrics *metrics, BenchOverlay *overlay);

#endif /* OBJ_MODEL_LOADER_INPUT_H */
