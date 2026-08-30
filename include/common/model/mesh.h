#ifndef COMMON_MODEL_MESH_H
#define COMMON_MODEL_MESH_H

#include <stddef.h>

#include "common/math3d/vec3.h"

/* No SDL2 dependency -- generic triangle-mesh representation shared by the
 * OBJ/MTL loader and the SDL2 rasterizer. Neither side owns this file. */

#define MESH_MATERIAL_NAME_LEN 64
#define MESH_TEXTURE_PATH_LEN 512

typedef struct {
    Vec3 position;
    Vec3 normal;   /* only meaningful when has_normal */
    Vec2 texcoord; /* only meaningful when has_texcoord; v already flipped to SDL's top-left-origin convention */
    unsigned char has_normal;
    unsigned char has_texcoord;
} MeshVertex;

typedef struct {
    char name[MESH_MATERIAL_NAME_LEN];
    float diffuse_color[3];  /* Kd */
    float specular_color[3]; /* Ks */
    float opacity;           /* d, or 1 - Tr; 1 = opaque */
    float shininess;         /* Ns */
    char diffuse_texture_path[MESH_TEXTURE_PATH_LEN]; /* resolved filesystem
                              path; empty string = no texture, use diffuse_color */
} MeshMaterial;

/* Fully flattened, unindexed: triangle i owns vertices[3*i], [3*i+1], [3*i+2].
 * No vertex welding -- fine at the triangle counts this suite targets. */
typedef struct {
    MeshVertex *vertices;
    int vertex_count; /* always 3 * triangle_count */

    int *triangle_material_ids; /* one per triangle, index into materials[];
                                  * never -1 -- unassigned triangles are
                                  * remapped to a synthetic default material */
    int triangle_count;

    MeshMaterial *materials;
    int material_count;

    size_t bytes_allocated; /* tracked for memory-usage metrics */
} Mesh;

/* Leaves mesh valid/empty (NULL pointers, zero counts) and safe to release. */
void mesh_init(Mesh *mesh);

void mesh_free(Mesh *mesh);

/* Bounding-sphere center/radius over all vertex positions. Zero-vertex mesh
 * yields center (0,0,0), radius 0. */
void mesh_compute_bounds(const Mesh *mesh, Vec3 *out_center, float *out_radius);

#endif /* COMMON_MODEL_MESH_H */
