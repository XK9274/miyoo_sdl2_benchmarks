#include "common/render3d/material_texture.h"

#include <SDL2/SDL_image.h>

SDL_Texture *material_texture_load(SDL_Renderer *renderer, const MeshMaterial *material)
{
    if (material->diffuse_texture_path[0] == '\0') {
        return NULL;
    }

    SDL_Surface *surface = IMG_Load(material->diffuse_texture_path);
    if (!surface) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "material_texture_load: %s: %s",
                    material->diffuse_texture_path, IMG_GetError());
        return NULL;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    if (!texture) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "material_texture_load: %s: %s",
                    material->diffuse_texture_path, SDL_GetError());
        return NULL;
    }

    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    return texture;
}
