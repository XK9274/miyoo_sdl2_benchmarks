#ifndef COMMON_MODEL_OBJ_LOADER_H
#define COMMON_MODEL_OBJ_LOADER_H

#include "common/model/mesh.h"

/* Wraps tinyobjloader-c; callers never see tinyobjloader types. Diffuse
 * texture paths (map_Kd) are resolved relative to obj_path's directory but
 * not opened -- image loading happens elsewhere. */

typedef enum {
    OBJ_LOADER_OK = 0,
    OBJ_LOADER_ERROR_FILE_NOT_FOUND,
    OBJ_LOADER_ERROR_PARSE_FAILED,
    OBJ_LOADER_ERROR_OUT_OF_MEMORY
} ObjLoaderResult;

/* mesh is (re-)initialized internally -- caller does not pre-init it. On
 * any return, including an error, mesh is left valid and safe to free
 * (empty on error, never partially filled). */
ObjLoaderResult obj_loader_load(const char *obj_path, Mesh *mesh);

#endif /* COMMON_MODEL_OBJ_LOADER_H */
