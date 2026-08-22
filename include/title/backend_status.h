#ifndef TITLE_BACKEND_STATUS_H
#define TITLE_BACKEND_STATUS_H

#include <SDL2/SDL.h>

/* Snapshot of platform/backend capabilities, probed once per context init. */
typedef struct {
    SDL_bool render_ok;
    char renderer_name[32];

    SDL_bool joystick_ok;
    SDL_bool haptic_ok;
    SDL_bool power_ok;

    SDL_bool audio_ok;
    char audio_driver[32];

    SDL_bool gl_ok;

    char video_driver[32];
    int display_w;
    int display_h;
    int display_refresh_hz;

    int cpu_count;
    int ram_mb;

    /* System-wide MI_SYS MMA heap usage, from /proc/mi_modules/mi_sys_mma/. */
    unsigned int mma_pool_used_bytes;
    unsigned int mma_pool_max_bytes;

    int sdl_major;
    int sdl_minor;
    int sdl_patch;
} TitleBackendStatus;

void title_backend_status_probe(TitleBackendStatus *out, SDL_Window *window, SDL_Renderer *renderer);

#endif /* TITLE_BACKEND_STATUS_H */
