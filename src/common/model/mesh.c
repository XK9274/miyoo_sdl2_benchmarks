#include "common/model/mesh.h"

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

void mesh_init(Mesh *mesh)
{
    memset(mesh, 0, sizeof(*mesh));
}

void mesh_free(Mesh *mesh)
{
    free(mesh->vertices);
    free(mesh->triangle_material_ids);
    free(mesh->materials);
    mesh_init(mesh);
}

void mesh_compute_bounds(const Mesh *mesh, Vec3 *out_center, float *out_radius)
{
    Vec3 min = {FLT_MAX, FLT_MAX, FLT_MAX};
    Vec3 max = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

    if (mesh->vertex_count == 0) {
        Vec3 zero = {0.0f, 0.0f, 0.0f};
        *out_center = zero;
        *out_radius = 0.0f;
        return;
    }

    for (int i = 0; i < mesh->vertex_count; i++) {
        const Vec3 p = mesh->vertices[i].position;
        if (p.x < min.x) min.x = p.x;
        if (p.y < min.y) min.y = p.y;
        if (p.z < min.z) min.z = p.z;
        if (p.x > max.x) max.x = p.x;
        if (p.y > max.y) max.y = p.y;
        if (p.z > max.z) max.z = p.z;
    }

    const Vec3 center = {
        (min.x + max.x) * 0.5f,
        (min.y + max.y) * 0.5f,
        (min.z + max.z) * 0.5f
    };

    float radius = 0.0f;
    for (int i = 0; i < mesh->vertex_count; i++) {
        const Vec3 p = mesh->vertices[i].position;
        const Vec3 d = bench_vec3_sub(p, center);
        const float dist = bench_vec3_length(d);
        if (dist > radius) {
            radius = dist;
        }
    }

    *out_center = center;
    *out_radius = radius;
}
