#include "gl_fbo_effects/scenes/effects.h"

#include "common/gl_effect.h"
#include "common/gl_effect_library.h"

#include <SDL2/SDL_log.h>

#include <math.h>

typedef struct {
    GLEffectTarget target;
    Uint32 program;
} RsglEffect;

static RsglEffect rsgl_effects[RSGL_EFFECT_MAX];
static int rsgl_effect_total = 0;

static SDL_bool rsgl_create_effects(SDL_Renderer *renderer, int width, int height)
{
    rsgl_effect_total = 0;
    for (int i = 0; i < RSGL_EFFECT_MAX; ++i) {
        RsglEffect *effect = &rsgl_effects[rsgl_effect_total];
        if (!gl_effect_target_create(&effect->target, renderer, width, height)) {
            continue;
        }
        effect->program = gl_effect_compile_program(gl_effect_library_fragment_source(i));
        if (!effect->program) {
            gl_effect_target_destroy(&effect->target);
            continue;
        }
        ++rsgl_effect_total;
    }

    if (rsgl_effect_total == 0) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "rsgl_create_effects: no effects compiled");
        return SDL_FALSE;
    }
    return SDL_TRUE;
}

static void rsgl_destroy_effects(void)
{
    for (int i = 0; i < rsgl_effect_total; ++i) {
        gl_effect_destroy_program(rsgl_effects[i].program);
        rsgl_effects[i].program = 0;
        gl_effect_target_destroy(&rsgl_effects[i].target);
    }
    rsgl_effect_total = 0;
}

SDL_bool rsgl_effects_init(RsglState *state, SDL_Renderer *renderer)
{
    if (!state || !renderer) {
        return SDL_FALSE;
    }

    if (!gl_effect_context_acquire()) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "rsgl_effects_init: failed to acquire shared GL context");
        return SDL_FALSE;
    }

    if (!rsgl_create_effects(renderer, state->fbo_width, state->fbo_height)) {
        gl_effect_context_release();
        return SDL_FALSE;
    }

    state->gl_ready = SDL_TRUE;
    rsgl_state_commit_fbo_size(state);
    state->effect_count = rsgl_effect_total;
    if (state->effect_index >= state->effect_count) {
        state->effect_index = 0;
    }

    return SDL_TRUE;
}

SDL_bool rsgl_effects_apply_fbo_size(RsglState *state, SDL_Renderer *renderer)
{
    if (!state || !renderer) {
        return SDL_FALSE;
    }
    if (!state->gl_ready) {
        rsgl_state_commit_fbo_size(state);
        return SDL_TRUE;
    }

    const int width = state->fbo_width;
    const int height = state->fbo_height;
    if (width <= 0 || height <= 0) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "rsgl_effects_apply_fbo_size: invalid target size %dx%d",
                    width, height);
        rsgl_state_revert_fbo_size(state);
        return SDL_FALSE;
    }

    /* Build the new-size targets before touching the live ones, so a
     * failure partway through leaves the currently rendering effects
     * untouched instead of half torn down. */
    GLEffectTarget new_targets[RSGL_EFFECT_MAX];
    int built = 0;
    for (; built < rsgl_effect_total; ++built) {
        if (!gl_effect_target_create(&new_targets[built], renderer, width, height)) {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                        "rsgl_effects_apply_fbo_size: target creation failed at index %d",
                        built);
            for (int i = 0; i < built; ++i) {
                gl_effect_target_destroy(&new_targets[i]);
            }
            rsgl_state_revert_fbo_size(state);
            return SDL_FALSE;
        }
    }

    for (int i = 0; i < rsgl_effect_total; ++i) {
        gl_effect_target_destroy(&rsgl_effects[i].target);
        rsgl_effects[i].target = new_targets[i];
    }

    rsgl_state_commit_fbo_size(state);
    return SDL_TRUE;
}

void rsgl_effects_render(RsglState *state,
                         SDL_Renderer *renderer,
                         BenchMetrics *metrics,
                         double delta_seconds)
{
    if (!state || !renderer || !state->gl_ready) {
        return;
    }

    state->elapsed_time += (float)delta_seconds;
    if (state->auto_cycle && state->effect_count > 0) {
        const double cycle_time = 10.0;
        if (fmod(state->elapsed_time, cycle_time) < delta_seconds) {
            state->effect_index = (state->effect_index + 1) % state->effect_count;
        }
    }

    if (state->effect_count <= 0) {
        return;
    }
    const int mode = state->effect_index % state->effect_count;
    RsglEffect *effect = &rsgl_effects[mode];
    gl_effect_render(&effect->target, effect->program, gl_effect_set_time_uniform, &state->elapsed_time);

    SDL_Rect dst = {
        0,
        (int)state->top_margin,
        state->screen_width,
        SDL_max(1, state->screen_height - (int)state->top_margin)
    };

    SDL_RenderCopy(renderer, effect->target.screen_texture, NULL, &dst);

    if (metrics) {
        metrics->draw_calls += 2; // GL render + SDL copy
        metrics->texture_switches++;
    }
}

void rsgl_effects_warmup(RsglState *state)
{
    if (!state || !state->gl_ready || state->effect_count <= 0) {
        return;
    }

    float warm_time = 0.0f;
    const int mode = state->effect_index % state->effect_count;
    RsglEffect *effect = &rsgl_effects[mode];
    gl_effect_render(&effect->target, effect->program, gl_effect_set_time_uniform, &warm_time);
}

void rsgl_effects_cleanup(RsglState *state)
{
    if (!state) {
        return;
    }

    if (state->gl_ready) {
        rsgl_destroy_effects();
        gl_effect_context_release();
    }

    state->gl_ready = SDL_FALSE;
    state->effect_count = 0;
}

int rsgl_effect_count(void)
{
    return rsgl_effect_total;
}

const char *rsgl_effect_name(int index)
{
    if (index < 0 || index >= rsgl_effect_total) {
        return "Unknown";
    }
    return gl_effect_library_name(index);
}

SDL_bool rsgl_effect_index_from_name(const char *name, int *out_index)
{
    for (int i = 0; i < rsgl_effect_total; i++) {
        if (SDL_strcasecmp(name, gl_effect_library_name(i)) == 0) {
            *out_index = i;
            return SDL_TRUE;
        }
    }
    return SDL_FALSE;
}
