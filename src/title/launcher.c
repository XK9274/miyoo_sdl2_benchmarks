#define _POSIX_C_SOURCE 200809L

#include "title/launcher.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <SDL2/SDL_image.h>

#include "controller_input.h"
#include "common/display_config.h"
#include "common/frame_limit.h"
#include "title/background.h"
#include "title/config_panel.h"

static TTF_Font *title_load_menu_font(int size)
{
    char bin_dir[PATH_MAX];
    if (!title_get_bin_dir(bin_dir, sizeof(bin_dir))) {
        return NULL;
    }
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/../assets/ThaleahFat.ttf", bin_dir);
    TTF_Font *font = TTF_OpenFont(path, size);
    if (font) {
        /* Keep glyph edges smooth at the native 640x480 title resolution. */
        TTF_SetFontHinting(font, TTF_HINTING_LIGHT);
    }
    return font;
}

SDL_bool title_context_init(TitleContext *ctx)
{
    if (!ctx) {
        return SDL_FALSE;
    }
    memset(ctx, 0, sizeof(*ctx));

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "title: SDL_Init failed: %s\n", SDL_GetError());
        return SDL_FALSE;
    }
    if (TTF_Init() < 0) {
        fprintf(stderr, "title: TTF_Init failed: %s\n", TTF_GetError());
        SDL_Quit();
        return SDL_FALSE;
    }
    if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) == 0) {
        fprintf(stderr, "title: IMG_Init failed: %s\n", IMG_GetError());
        TTF_Quit();
        SDL_Quit();
        return SDL_FALSE;
    }

    ctx->window = SDL_CreateWindow("SDL2 Demo Suites",
                                   SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                   BENCH_NATIVE_W, BENCH_NATIVE_H,
                                   SDL_WINDOW_SHOWN);
    if (!ctx->window) {
        fprintf(stderr, "title: window creation failed: %s\n", SDL_GetError());
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return SDL_FALSE;
    }

    /* The hint is the only way to disable vsync -- SDL_CreateRenderer force-ORs it in. */
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "0");
    ctx->renderer = SDL_CreateRenderer(ctx->window, -1, SDL_RENDERER_ACCELERATED);
    if (!ctx->renderer) {
        fprintf(stderr, "title: renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(ctx->window);
        ctx->window = NULL;
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return SDL_FALSE;
    }

    /* Title's own UI stays native -- resolution config applies to the child suite. */
    bench_display_config_apply(ctx->renderer, BENCH_NATIVE_W, BENCH_NATIVE_H);

    bench_driver_init(ctx->window, ctx->renderer);
    title_battery_fill_init(&ctx->battery_fill, ctx->renderer);
    title_battery_glow_init(&ctx->battery_glow, ctx->renderer);
    /* Run before the backend probe so its SDL_RaiseWindow fires last. */
    title_fireflies_init(&ctx->fireflies, ctx->renderer);
    title_backend_status_probe(&ctx->backend, ctx->window, ctx->renderer);
    ctx->background = title_background_load(ctx->renderer);

    ctx->title_font = title_load_menu_font(32);
    ctx->ui_font = title_load_menu_font(18);
    ctx->small_font = title_load_menu_font(16);
    ctx->accent_font = title_load_menu_font(24);

    return SDL_TRUE;
}

void title_context_shutdown(TitleContext *ctx)
{
    if (!ctx) {
        return;
    }

    title_fireflies_shutdown(&ctx->fireflies);
    title_battery_fill_shutdown(&ctx->battery_fill);
    title_battery_glow_shutdown(&ctx->battery_glow);
    bench_driver_shutdown();

    if (ctx->title_font) {
        TTF_CloseFont(ctx->title_font);
        ctx->title_font = NULL;
    }
    if (ctx->ui_font) {
        TTF_CloseFont(ctx->ui_font);
        ctx->ui_font = NULL;
    }
    if (ctx->small_font) {
        TTF_CloseFont(ctx->small_font);
        ctx->small_font = NULL;
    }
    if (ctx->accent_font) {
        TTF_CloseFont(ctx->accent_font);
        ctx->accent_font = NULL;
    }
    if (ctx->background) {
        SDL_DestroyTexture(ctx->background);
        ctx->background = NULL;
    }
    if (ctx->renderer) {
        SDL_DestroyRenderer(ctx->renderer);
        ctx->renderer = NULL;
    }
    if (ctx->window) {
        SDL_DestroyWindow(ctx->window);
        ctx->window = NULL;
    }

    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
}

SDL_bool title_get_bin_dir(char *out_dir, size_t out_size)
{
    char exe_path[PATH_MAX];
    const ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len <= 0) {
        return SDL_FALSE;
    }
    exe_path[len] = '\0';

    char *slash = strrchr(exe_path, '/');
    if (!slash) {
        return SDL_FALSE;
    }
    *slash = '\0';

    if (strlen(exe_path) >= out_size) {
        return SDL_FALSE;
    }
    strcpy(out_dir, exe_path);
    return SDL_TRUE;
}

static const char *title_vsync_mode_string(BenchVSyncStatus mode)
{
    switch (mode) {
        case BENCH_VSYNC_STATUS_ADAPTIVE: return BENCH_VSYNC_MODE_ADAPTIVE;
        case BENCH_VSYNC_STATUS_STRICT:   return BENCH_VSYNC_MODE_STRICT;
        case BENCH_VSYNC_STATUS_OFF:
        default:                          return BENCH_VSYNC_MODE_OFF;
    }
}

static const char *title_input_mode_string(BenchInputSource mode)
{
    return (mode == BENCH_INPUT_SOURCE_JOYSTICK) ? BENCH_INPUT_MODE_JOYSTICK : BENCH_INPUT_MODE_KEYBOARD;
}

SDL_bool title_launch_suite(const TitleState *state, const char *bin_name, TitleContext *ctx, TitleLaunchResult *out_result)
{
    if (!state || !bin_name || !ctx || !out_result) {
        return SDL_FALSE;
    }
    memset(out_result, 0, sizeof(*out_result));

    char bin_dir[PATH_MAX];
    if (!title_get_bin_dir(bin_dir, sizeof(bin_dir))) {
        fprintf(stderr, "title: could not resolve own executable directory\n");
        return SDL_FALSE;
    }

    char full_path[PATH_MAX];
    snprintf(full_path, sizeof(full_path), "%s/%s", bin_dir, bin_name);

    int logical_w, logical_h;
    title_resolution_dims(state->logical_res, &logical_w, &logical_h);

    char logical_w_str[16], logical_h_str[16], frame_limit_str[16];
    snprintf(logical_w_str, sizeof(logical_w_str), "%d", logical_w);
    snprintf(logical_h_str, sizeof(logical_h_str), "%d", logical_h);
    snprintf(frame_limit_str, sizeof(frame_limit_str), "%d", state->frame_limit_fps);

    /* Singleton driver resources -- tear ours down before the child gets its own. */
    title_context_shutdown(ctx);

    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "title: fork failed\n");
        title_context_init(ctx);
        return SDL_FALSE;
    }

    if (pid == 0) {
        setenv(BENCH_ENV_LOGICAL_W, logical_w_str, 1);
        setenv(BENCH_ENV_LOGICAL_H, logical_h_str, 1);
        setenv(BENCH_ENV_FRAME_LIMIT_FPS, frame_limit_str, 1);
        setenv(BENCH_HINT_MMIYOO_VSYNC_MODE, title_vsync_mode_string(state->vsync_mode), 1);
        setenv(BENCH_HINT_MMIYOO_INPUT_MODE, title_input_mode_string(state->input_mode), 1);

        char *argv[] = {full_path, NULL};
        execv(full_path, argv);
        fprintf(stderr, "title: execv of %s failed: %s\n", full_path, strerror(errno));
        _exit(127); /* execv only returns on failure */
    }

    int status = 0;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status)) {
        const int code = WEXITSTATUS(status);
        if (code == 127) {
            out_result->exec_failed = SDL_TRUE;
        }
        out_result->exit_code = code;
    } else if (WIFSIGNALED(status)) {
        out_result->crashed = SDL_TRUE;
        out_result->signal_number = WTERMSIG(status);
    }

    const SDL_bool reinit_ok = title_context_init(ctx);

    /* Flush any stale input events from before/during reinit. */
    SDL_Event flush_event;
    while (SDL_PollEvent(&flush_event)) {
    }

    return reinit_ok;
}
