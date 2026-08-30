#include "obj_model_loader/placeholder_model.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    Vec3 normal;
    Vec3 tangent; /* any unit vector perpendicular to normal */
    float color[3];
    const char *name;
} PlaceholderFace;

static void placeholder_set_material(MeshMaterial *m, const char *name, float r, float g, float b)
{
    memset(m, 0, sizeof(*m));
    snprintf(m->name, sizeof(m->name), "%s", name);
    m->diffuse_color[0] = r;
    m->diffuse_color[1] = g;
    m->diffuse_color[2] = b;
    m->opacity = 1.0f;
}

void placeholder_model_build(Mesh *mesh)
{
    mesh_init(mesh);

    static const PlaceholderFace faces[6] = {
        {{ 1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.85f, 0.25f, 0.25f}, "px"},
        {{-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.25f, 0.65f, 0.85f}, "nx"},
        {{0.0f,  1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.35f, 0.85f, 0.35f}, "py"},
        {{0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.85f, 0.85f, 0.30f}, "ny"},
        {{0.0f, 0.0f,  1.0f}, {1.0f, 0.0f, 0.0f}, {0.80f, 0.45f, 0.85f}, "pz"},
        {{0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.85f, 0.55f, 0.20f}, "nz"}
    };

    mesh->material_count = 6;
    mesh->materials = (MeshMaterial *)calloc(6, sizeof(MeshMaterial));
    for (int i = 0; i < 6; i++) {
        placeholder_set_material(&mesh->materials[i], faces[i].name,
                                 faces[i].color[0], faces[i].color[1], faces[i].color[2]);
    }

    mesh->triangle_count = 12; /* 2 triangles per face * 6 faces */
    mesh->vertex_count = 36;
    mesh->vertices = (MeshVertex *)calloc(36, sizeof(MeshVertex));
    mesh->triangle_material_ids = (int *)calloc(12, sizeof(int));

    int vtx = 0;
    int tri = 0;
    for (int f = 0; f < 6; f++) {
        const Vec3 n = faces[f].normal;
        const Vec3 u = faces[f].tangent;
        /* v = cross(n, u) guarantees cross(u, v) == n exactly (triple-product
         * identity, n unit and u perpendicular to n) -- so the corner loop always
         * winds outward/CCW regardless of face. */
        const Vec3 v = bench_vec3_cross(n, u);

        const Vec3 corner_mm = bench_vec3_add(n, bench_vec3_add(bench_vec3_scale(u, -1.0f), bench_vec3_scale(v, -1.0f)));
        const Vec3 corner_pm = bench_vec3_add(n, bench_vec3_add(u, bench_vec3_scale(v, -1.0f)));
        const Vec3 corner_pp = bench_vec3_add(n, bench_vec3_add(u, v));
        const Vec3 corner_mp = bench_vec3_add(n, bench_vec3_add(bench_vec3_scale(u, -1.0f), v));

        const Vec3 quad[4] = {corner_mm, corner_pm, corner_pp, corner_mp};
        static const int tri_indices[2][3] = {{0, 1, 2}, {0, 2, 3}};

        for (int t = 0; t < 2; t++) {
            mesh->triangle_material_ids[tri++] = f;
            for (int c = 0; c < 3; c++) {
                MeshVertex *mv = &mesh->vertices[vtx++];
                memset(mv, 0, sizeof(*mv));
                mv->position = quad[tri_indices[t][c]];
                mv->normal = n;
                mv->has_normal = 1;
                mv->has_texcoord = 0;
            }
        }
    }

    mesh->bytes_allocated = sizeof(MeshVertex) * (size_t)mesh->vertex_count +
                            sizeof(int) * (size_t)mesh->triangle_count +
                            sizeof(MeshMaterial) * (size_t)mesh->material_count;
}
