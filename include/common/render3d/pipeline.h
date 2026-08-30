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

/* Forces recomputing the cached per-triangle lighting on the next draw --
 * call whenever the mesh changes at runtime, since the cache can't tell
 * "different mesh" from "same triangle count." */
void render3d_scratch_reset(Render3DScratch *scratch);

#define RENDER3D_MAX_LIGHTS 4

typedef struct {
    Mat4 view;
    Mat4 projection;
    float near_plane; /* must match the near value baked into `projection` */

    /* Directional lights summed per triangle (Lambertian: weight *
     * max(dot(N,to_light),0), no shadowing). Fixed in world space so every
     * face gets light across the full orbit. */
    Vec3 light_directions[RENDER3D_MAX_LIGHTS]; /* direction each light travels FROM the light; need not be unit length */
    float light_weights[RENDER3D_MAX_LIGHTS];   /* per-light contribution weight */
    int light_count;                             /* 0..RENDER3D_MAX_LIGHTS */

    float ambient_floor;  /* 0..1, minimum lit brightness for a face facing away from every light */

    /* NDC maps into this sub-rect, not the full screen, so the model
     * centers below an overlay. No frustum side-plane clipping (only
     * near-plane), so content can still draw outside this rect without an
     * explicit clip rect. */
    int viewport_x;
    int viewport_y;
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
