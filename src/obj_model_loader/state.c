#include "obj_model_loader/state.h"

#include <dirent.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "obj_model_loader/placeholder_model.h"

/* Per-model authoring quirks that can't be inferred from the OBJ file
 * itself -- the format carries no up-axis metadata. */
typedef struct {
    const char *name;
    SDL_bool z_up; /* true if the source is authored Z-up (rotated to Y-up on load) */
} ObjModelQuirks;

static const ObjModelQuirks OBJ_MODEL_QUIRKS[] = {
    {"miyoo", SDL_TRUE},
};

static SDL_bool obj_model_needs_z_up_fix(const char *name)
{
    for (size_t i = 0; i < sizeof(OBJ_MODEL_QUIRKS) / sizeof(OBJ_MODEL_QUIRKS[0]); i++) {
        if (strcmp(OBJ_MODEL_QUIRKS[i].name, name) == 0) {
            return OBJ_MODEL_QUIRKS[i].z_up;
        }
    }
    return SDL_FALSE;
}

/* Z-up -> Y-up: rotate -90 degrees about X once at load time: (x,y,z) ->
 * (x,z,-y). A pure rotation (determinant +1), so normals transform
 * identically to positions -- no inverse-transpose needed. */
static void obj_state_fix_axis_z_up_to_y_up(Mesh *mesh)
{
    for (int i = 0; i < mesh->vertex_count; i++) {
        MeshVertex *v = &mesh->vertices[i];
        const float pos_y = v->position.y;
        v->position.y = v->position.z;
        v->position.z = -pos_y;

        const float normal_y = v->normal.y;
        v->normal.y = v->normal.z;
        v->normal.z = -normal_y;
    }
}

static int obj_model_name_compare(const void *a, const void *b)
{
    return strcmp((const char *)a, (const char *)b);
}

/* Dual-path probe (assets/models or ../assets/models) depends on the
 * launcher having already cd'd into the app directory. Safe to find
 * zero -- callers fall back to the placeholder cube. */
static void obj_state_scan_models(ObjModelLoaderState *state)
{
    static const char *ROOTS[] = {"assets/models", "../assets/models", NULL};
    state->available_model_count = 0;

    for (int r = 0; ROOTS[r] != NULL && state->available_model_count == 0; r++) {
        DIR *dir = opendir(ROOTS[r]);
        if (!dir) {
            continue;
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL && state->available_model_count < OBJ_MODEL_MAX_COUNT) {
            if (entry->d_name[0] == '.') {
                continue;
            }

            char obj_path[300];
            snprintf(obj_path, sizeof(obj_path), "%s/%s/%s.obj", ROOTS[r], entry->d_name, entry->d_name);
            if (access(obj_path, F_OK) != 0) {
                continue;
            }

            snprintf(state->available_models[state->available_model_count], OBJ_MODEL_NAME_LEN, "%s", entry->d_name);
            state->available_model_count++;
        }

        closedir(dir);
    }

    if (state->available_model_count > 1) {
        qsort(state->available_models, (size_t)state->available_model_count, OBJ_MODEL_NAME_LEN, obj_model_name_compare);
    }
}

static int obj_state_find_model(const ObjModelLoaderState *state, const char *name)
{
    for (int i = 0; i < state->available_model_count; i++) {
        if (strcmp(state->available_models[i], name) == 0) {
            return i;
        }
    }
    return -1;
}

/* Cache keys on triangle count only -- must reset on every model swap or
 * two same-tri-count models can render with stale colors. */
static void obj_state_load_model_by_index(SDL_Renderer *renderer, ObjModelLoaderState *state, int index)
{
    model_instance_destroy(renderer, &state->model);
    memset(&state->model, 0, sizeof(state->model));

    SDL_bool loaded = SDL_FALSE;
    if (index >= 0 && index < state->available_model_count) {
        const char *name = state->available_models[index];
        char path[300];
        snprintf(path, sizeof(path), "assets/models/%s/%s.obj", name, name);
        if (access(path, F_OK) != 0) {
            snprintf(path, sizeof(path), "../assets/models/%s/%s.obj", name, name);
        }

        loaded = model_instance_load(renderer, path, &state->model);
        if (loaded) {
            snprintf(state->model_label, sizeof(state->model_label), "%s", path);
            if (obj_model_needs_z_up_fix(name)) {
                obj_state_fix_axis_z_up_to_y_up(&state->model.mesh);
            }
            state->current_model_index = index;
        }
    }

    if (!loaded) {
        Mesh placeholder;
        placeholder_model_build(&placeholder);
        state->model.mesh = placeholder;
        state->model.material_textures = NULL;
        state->model.material_texture_count = 0;
        snprintf(state->model_label, sizeof(state->model_label), "%s", "placeholder cube");
    }

    Vec3 center;
    float radius;
    mesh_compute_bounds(&state->model.mesh, &center, &radius);
    if (radius < 0.01f) {
        radius = 1.0f;
    }
    camera3d_init(&state->camera, center, radius * 2.5f);

    if (state->scratch) {
        render3d_scratch_reset(state->scratch);
    }
}

void obj_state_init(SDL_Renderer *renderer, ObjModelLoaderState *state)
{
    memset(state, 0, sizeof(*state));

    state->scratch = render3d_scratch_create();

    obj_state_scan_models(state);

    const char *initial_name = getenv("OBJ_MODEL_NAME");
    int initial_index = (initial_name && *initial_name) ? obj_state_find_model(state, initial_name) : -1;
    if (initial_index < 0) {
        initial_index = obj_state_find_model(state, "sheep");
    }
    if (initial_index < 0) {
        initial_index = 0; /* first model found, if any; -1 (none found) falls through to the placeholder */
    }

    obj_state_load_model_by_index(renderer, state, initial_index);

    state->auto_rotate = SDL_TRUE;
    state->auto_rotate_radians_per_second = 0.6f;
    state->wireframe = SDL_FALSE;
    state->ambient_floor = 0.4f;
}

void obj_state_destroy(SDL_Renderer *renderer, ObjModelLoaderState *state)
{
    model_instance_destroy(renderer, &state->model);
    render3d_scratch_destroy(state->scratch);
    memset(state, 0, sizeof(*state));
}

void obj_state_update_layout(ObjModelLoaderState *state, BenchOverlay *overlay)
{
    state->top_margin = (float)bench_overlay_height(overlay);
}

void obj_state_cycle_model(SDL_Renderer *renderer, ObjModelLoaderState *state, int direction)
{
    if (state->available_model_count <= 1) {
        return;
    }
    int next = (state->current_model_index + direction) % state->available_model_count;
    if (next < 0) {
        next += state->available_model_count;
    }
    obj_state_load_model_by_index(renderer, state, next);
}
