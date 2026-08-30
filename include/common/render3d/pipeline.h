#ifndef COMMON_RENDER3D_PIPELINE_H
#define COMMON_RENDER3D_PIPELINE_H

#include <SDL2/SDL.h>

#include "common/math3d/mat4.h"
#include "common/model/mesh.h"
#include "common/types.h" /* BenchMetrics */

/* SDL2 rasterizer: MVP transform, near-plane clip, backface cull, flat
 * lighting, depth sort, texture-batched SDL_RenderGeometry draws. Renders
 * any mesh; knows nothing about tinyobjloader or the .obj format. */

typedef struct Render3DScratch Render3DScratch;

Render3DScratch *render3d_scratch_create(void);
void render3d_scratch_destroy(Render3DScratch *scratch);

typedef struct {
    Mat4 view;
    Mat4 projection;
    float near_plane; /* must match the near value baked into `projection` */

    Vec3 light_direction; /* world-space direction light travels FROM the
                           * light; need not be unit length */
    float ambient_floor;  /* 0..1, minimum lit brightness for a face facing away from the light */

    int viewport_width;
    int viewport_height;
    SDL_bool wireframe;
} Render3DFrameParams;

/* material_textures may be NULL (flat diffuse_color fill for every
 * material) or sized mesh->material_count with any entries NULL. scratch
 * grows as needed and can be reused across frames. */
void render3d_draw_mesh(SDL_Renderer *renderer,
                        const Mesh *mesh,
                        SDL_Texture *const *material_textures,
                        const Render3DFrameParams *params,
                        Render3DScratch *scratch,
                        BenchMetrics *metrics);

#endif /* COMMON_RENDER3D_PIPELINE_H */
