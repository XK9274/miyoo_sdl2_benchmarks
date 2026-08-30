#include "common/render3d/model_instance.h"

#include <stdlib.h>
#include <string.h>

#include "common/model/obj_loader.h"
#include "common/render3d/material_texture.h"

SDL_bool model_instance_load(SDL_Renderer *renderer, const char *obj_path, ModelInstance *out_instance)
{
    memset(out_instance, 0, sizeof(*out_instance));

    Mesh mesh;
    if (obj_loader_load(obj_path, &mesh) != OBJ_LOADER_OK) {
        return SDL_FALSE;
    }

    SDL_Texture **textures = NULL;
    if (mesh.material_count > 0) {
        textures = (SDL_Texture **)calloc((size_t)mesh.material_count, sizeof(SDL_Texture *));
        if (!textures) {
            mesh_free(&mesh);
            return SDL_FALSE;
        }
        for (int i = 0; i < mesh.material_count; i++) {
            textures[i] = material_texture_load(renderer, &mesh.materials[i]);
        }
    }

    out_instance->mesh = mesh;
    out_instance->material_textures = textures;
    out_instance->material_texture_count = mesh.material_count;
    return SDL_TRUE;
}

void model_instance_destroy(SDL_Renderer *renderer, ModelInstance *instance)
{
    (void)renderer;

    if (instance->material_textures) {
        for (int i = 0; i < instance->material_texture_count; i++) {
            if (instance->material_textures[i]) {
                SDL_DestroyTexture(instance->material_textures[i]);
            }
        }
        free(instance->material_textures);
    }
    mesh_free(&instance->mesh);
    memset(instance, 0, sizeof(*instance));
}
