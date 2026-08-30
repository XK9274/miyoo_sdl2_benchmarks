#ifndef COMMON_RENDER3D_MODEL_INSTANCE_H
#define COMMON_RENDER3D_MODEL_INSTANCE_H

#include <SDL2/SDL.h>

#include "common/model/mesh.h"

/* Ties the SDL-free OBJ/MTL loader to SDL2_image texture loading. This
 * header stays generic (Mesh + SDL_Texture only); loader internals stay in
 * the .c file. */

typedef struct {
    Mesh mesh;
    SDL_Texture **material_textures; /* one entry per mesh.materials[i]; entries may be NULL */
    int material_texture_count;      /* == mesh.material_count */
} ModelInstance;

/* On success, populates out_instance (mesh + loaded/resolved textures) --
 * caller must release it. On failure, out_instance is left zeroed and safe
 * to release the same way. */
SDL_bool model_instance_load(SDL_Renderer *renderer, const char *obj_path, ModelInstance *out_instance);

void model_instance_destroy(SDL_Renderer *renderer, ModelInstance *instance);

#endif /* COMMON_RENDER3D_MODEL_INSTANCE_H */
