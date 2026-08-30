#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include "tinyobjloader-c/tinyobj_loader_c.h"

#include "common/model/obj_loader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Reads filename verbatim into a malloc'd buffer; path resolution has
 * already happened before this is called. */
static void obj_file_reader(void *ctx, const char *filename, int is_mtl,
                            const char *obj_filename, char **buf, size_t *len)
{
    (void)ctx;
    (void)is_mtl;
    (void)obj_filename;

    *buf = NULL;
    *len = 0;

    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        return;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return;
    }
    const long size = ftell(fp);
    if (size < 0 || fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return;
    }

    char *data = (char *)malloc((size_t)size);
    if (!data) {
        fclose(fp);
        return;
    }

    const size_t read_bytes = fread(data, 1, (size_t)size, fp);
    fclose(fp);

    if (read_bytes != (size_t)size) {
        free(data);
        return;
    }

    *buf = data;
    *len = read_bytes;
}

static void obj_dirname(const char *path, char *out_dir, size_t out_size)
{
    const char *slash = strrchr(path, '/');
    if (!slash) {
        out_dir[0] = '\0';
        return;
    }
    size_t len = (size_t)(slash - path);
    if (len >= out_size) {
        len = out_size - 1;
    }
    memcpy(out_dir, path, len);
    out_dir[len] = '\0';
}

static void obj_convert_material(const tinyobj_material_t *src, MeshMaterial *dst, const char *obj_dir)
{
    memset(dst, 0, sizeof(*dst));

    if (src->name) {
        snprintf(dst->name, sizeof(dst->name), "%s", src->name);
    }
    dst->diffuse_color[0] = src->diffuse[0];
    dst->diffuse_color[1] = src->diffuse[1];
    dst->diffuse_color[2] = src->diffuse[2];
    dst->specular_color[0] = src->specular[0];
    dst->specular_color[1] = src->specular[1];
    dst->specular_color[2] = src->specular[2];
    dst->opacity = src->dissolve;
    dst->shininess = src->shininess;

    if (src->diffuse_texname && src->diffuse_texname[0] != '\0') {
        if (obj_dir[0] != '\0') {
            snprintf(dst->diffuse_texture_path, sizeof(dst->diffuse_texture_path),
                     "%s/%s", obj_dir, src->diffuse_texname);
        } else {
            snprintf(dst->diffuse_texture_path, sizeof(dst->diffuse_texture_path),
                     "%s", src->diffuse_texname);
        }
    }
}

static void obj_set_default_material(MeshMaterial *dst)
{
    memset(dst, 0, sizeof(*dst));
    snprintf(dst->name, sizeof(dst->name), "%s", "default");
    dst->diffuse_color[0] = 0.8f;
    dst->diffuse_color[1] = 0.8f;
    dst->diffuse_color[2] = 0.8f;
    dst->opacity = 1.0f;
}

static void obj_copy_vertex(const tinyobj_attrib_t *attrib, tinyobj_vertex_index_t idx, MeshVertex *out)
{
    memset(out, 0, sizeof(*out));

    out->position.x = attrib->vertices[3 * (size_t)idx.v_idx + 0];
    out->position.y = attrib->vertices[3 * (size_t)idx.v_idx + 1];
    out->position.z = attrib->vertices[3 * (size_t)idx.v_idx + 2];

    /* tinyobjloader-c returns a large negative sentinel (not -1) for absent
     * vt/vn indices -- treat any negative value as absent. */
    if (idx.vn_idx >= 0) {
        out->normal.x = attrib->normals[3 * (size_t)idx.vn_idx + 0];
        out->normal.y = attrib->normals[3 * (size_t)idx.vn_idx + 1];
        out->normal.z = attrib->normals[3 * (size_t)idx.vn_idx + 2];
        out->has_normal = 1;
    }

    if (idx.vt_idx >= 0) {
        out->texcoord.u = attrib->texcoords[2 * (size_t)idx.vt_idx + 0];
        /* OBJ's V=0-at-bottom vs SDL's texture V=0-at-top -- flipped
         * exactly once, here, so every downstream consumer already sees
         * SDL-convention UVs. */
        out->texcoord.v = 1.0f - attrib->texcoords[2 * (size_t)idx.vt_idx + 1];
        out->has_texcoord = 1;
    }
}

ObjLoaderResult obj_loader_load(const char *obj_path, Mesh *mesh)
{
    mesh_init(mesh);

    FILE *probe = fopen(obj_path, "rb");
    if (!probe) {
        return OBJ_LOADER_ERROR_FILE_NOT_FOUND;
    }
    fclose(probe);

    tinyobj_attrib_t attrib;
    tinyobj_shape_t *shapes = NULL;
    size_t num_shapes = 0;
    tinyobj_material_t *materials = NULL;
    size_t num_materials = 0;

    /* Untriangulated parse + fan-triangulation by hand below --
     * tinyobjloader-c's own TRIANGULATE flag aborts on any face with 6+
     * vertices, which real low-poly exports routinely have. */
    tinyobj_attrib_init(&attrib);
    const int ret = tinyobj_parse_obj(&attrib, &shapes, &num_shapes, &materials, &num_materials,
                                      obj_path, obj_file_reader, NULL, 0);
    if (ret != TINYOBJ_SUCCESS) {
        tinyobj_attrib_free(&attrib);
        tinyobj_shapes_free(shapes, num_shapes);
        tinyobj_materials_free(materials, num_materials);
        return OBJ_LOADER_ERROR_PARSE_FAILED;
    }

    char obj_dir[512];
    obj_dirname(obj_path, obj_dir, sizeof(obj_dir));

    /* One extra slot for the synthetic default material (unassigned triangles). */
    const int material_count = (int)num_materials + 1;
    const int default_material_index = (int)num_materials;

    MeshMaterial *out_materials = (MeshMaterial *)malloc(sizeof(MeshMaterial) * (size_t)material_count);
    if (!out_materials) {
        tinyobj_attrib_free(&attrib);
        tinyobj_shapes_free(shapes, num_shapes);
        tinyobj_materials_free(materials, num_materials);
        return OBJ_LOADER_ERROR_OUT_OF_MEMORY;
    }
    for (size_t i = 0; i < num_materials; i++) {
        obj_convert_material(&materials[i], &out_materials[i], obj_dir);
    }
    obj_set_default_material(&out_materials[default_material_index]);

    /* Fan-triangulate every polygon ourselves (tinyobjloader-c's own
     * triangulation is not used). */
    int triangle_count = 0;
    for (unsigned int p = 0; p < attrib.num_face_num_verts; p++) {
        const int n = attrib.face_num_verts[p];
        if (n >= 3) {
            triangle_count += n - 2;
        }
    }
    const int vertex_count = triangle_count * 3;

    MeshVertex *out_vertices = NULL;
    int *out_material_ids = NULL;
    if (triangle_count > 0) {
        out_vertices = (MeshVertex *)malloc(sizeof(MeshVertex) * (size_t)vertex_count);
        out_material_ids = (int *)malloc(sizeof(int) * (size_t)triangle_count);
    }
    if (triangle_count > 0 && (!out_vertices || !out_material_ids)) {
        free(out_vertices);
        free(out_material_ids);
        free(out_materials);
        tinyobj_attrib_free(&attrib);
        tinyobj_shapes_free(shapes, num_shapes);
        tinyobj_materials_free(materials, num_materials);
        return OBJ_LOADER_ERROR_OUT_OF_MEMORY;
    }

    int out_tri = 0;
    unsigned int face_vertex_offset = 0;
    for (unsigned int p = 0; p < attrib.num_face_num_verts; p++) {
        const int n = attrib.face_num_verts[p];
        if (n < 3) {
            face_vertex_offset += (unsigned int)(n > 0 ? n : 0);
            continue;
        }

        const int raw_material_id = attrib.material_ids[p];
        const int material_id = (raw_material_id >= 0) ? raw_material_id : default_material_index;
        const tinyobj_vertex_index_t idx0 = attrib.faces[face_vertex_offset + 0];

        for (int t = 0; t < n - 2; t++) {
            const tinyobj_vertex_index_t idx1 = attrib.faces[face_vertex_offset + (unsigned int)t + 1];
            const tinyobj_vertex_index_t idx2 = attrib.faces[face_vertex_offset + (unsigned int)t + 2];

            out_material_ids[out_tri] = material_id;
            obj_copy_vertex(&attrib, idx0, &out_vertices[out_tri * 3 + 0]);
            obj_copy_vertex(&attrib, idx1, &out_vertices[out_tri * 3 + 1]);
            obj_copy_vertex(&attrib, idx2, &out_vertices[out_tri * 3 + 2]);
            out_tri++;
        }

        face_vertex_offset += (unsigned int)n;
    }

    mesh->vertices = out_vertices;
    mesh->vertex_count = vertex_count;
    mesh->triangle_material_ids = out_material_ids;
    mesh->triangle_count = triangle_count;
    mesh->materials = out_materials;
    mesh->material_count = material_count;
    mesh->bytes_allocated = sizeof(MeshVertex) * (size_t)vertex_count +
                            sizeof(int) * (size_t)triangle_count +
                            sizeof(MeshMaterial) * (size_t)material_count;

    tinyobj_attrib_free(&attrib);
    tinyobj_shapes_free(shapes, num_shapes);
    tinyobj_materials_free(materials, num_materials);

    return OBJ_LOADER_OK;
}
