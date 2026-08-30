#ifndef COMMON_RENDER3D_MATERIAL_TEXTURE_H
#define COMMON_RENDER3D_MATERIAL_TEXTURE_H

#include <SDL2/SDL.h>

#include "common/model/mesh.h"

/* The only SDL2_image-dependent file in the OBJ-viewer pipeline -- knows
 * about MeshMaterial, but nothing about tinyobjloader or the .obj format. */

/* Returns NULL when the path is empty or the load fails for any reason
 * (missing file, bad format, decode error) -- always a silent, expected
 * fallback to diffuse_color, never an error to report. */
SDL_Texture *material_texture_load(SDL_Renderer *renderer, const MeshMaterial *material);

#endif /* COMMON_RENDER3D_MATERIAL_TEXTURE_H */
