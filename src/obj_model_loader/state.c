#include "obj_model_loader/state.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "obj_model_loader/placeholder_model.h"

/* Same access()-probing pattern audio_bench uses for assets/bgm.wav --
 * relies on launch.sh's `cd "$bench_dir"` before exec'ing suite binaries. */
static const char *OBJ_MODEL_PATHS[] = {
    "assets/models/sheep/sheep.obj",
    "../assets/models/sheep/sheep.obj",
    NULL
};

static const char *obj_state_resolve_model_path(void)
{
    for (int i = 0; OBJ_MODEL_PATHS[i] != NULL; i++) {
        if (access(OBJ_MODEL_PATHS[i], F_OK) == 0) {
            return OBJ_MODEL_PATHS[i];
        }
    }
    return NULL;
}

void obj_state_init(SDL_Renderer *renderer, ObjModelLoaderState *state)
{
    memset(state, 0, sizeof(*state));

    const char *path = obj_state_resolve_model_path();
    SDL_bool loaded = SDL_FALSE;
    if (path) {
        loaded = model_instance_load(renderer, path, &state->model);
        if (loaded) {
            snprintf(state->model_label, sizeof(state->model_label), "%s", path);
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

    state->auto_rotate = SDL_TRUE;
    state->auto_rotate_radians_per_second = 0.6f;
    state->wireframe = SDL_FALSE;
    state->ambient_floor = 0.15f;

    state->scratch = render3d_scratch_create();
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
