#include "title/backend_status.h"

#include <stdio.h>
#include <string.h>

#include "common/backend_probe.h"
#include "common/driver_support.h"
#include "common/gl_effect.h"

static void title_backend_probe_audio(TitleBackendStatus *out)
{
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        out->audio_ok = SDL_FALSE;
        return;
    }

    out->audio_ok = SDL_TRUE;
    const char *driver = SDL_GetCurrentAudioDriver();
    if (driver) {
        strncpy(out->audio_driver, driver, sizeof(out->audio_driver) - 1);
    }

    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

/* GL/audio capability and SDL version are static for the process lifetime --
   caching them avoids a GL context create/destroy and an audio subsystem
   init/quit on every return from a child benchmark. */
static SDL_bool g_static_probed = SDL_FALSE;
static SDL_bool g_static_gl_ok = SDL_FALSE;
static SDL_bool g_static_audio_ok = SDL_FALSE;
static char g_static_audio_driver[32] = {0};
static int g_static_cpu_count = 0;
static int g_static_sdl_major = 0;
static int g_static_sdl_minor = 0;
static int g_static_sdl_patch = 0;

void title_backend_status_probe(TitleBackendStatus *out, SDL_Window *window, SDL_Renderer *renderer)
{
    if (!out) {
        return;
    }
    memset(out, 0, sizeof(*out));

    if (renderer) {
        SDL_RendererInfo info;
        if (SDL_GetRendererInfo(renderer, &info) == 0) {
            out->render_ok = (info.flags & SDL_RENDERER_ACCELERATED) ? SDL_TRUE : SDL_FALSE;
            strncpy(out->renderer_name, info.name ? info.name : "", sizeof(out->renderer_name) - 1);
        }
    }

    const char *video_driver = SDL_GetCurrentVideoDriver();
    if (video_driver) {
        strncpy(out->video_driver, video_driver, sizeof(out->video_driver) - 1);
    }

    SDL_DisplayMode mode;
    if (SDL_GetCurrentDisplayMode(0, &mode) == 0) {
        out->display_w = mode.w;
        out->display_h = mode.h;
        out->display_refresh_hz = mode.refresh_rate;
    }

    BenchDriverStatus driver_status;
    bench_driver_get_status(&driver_status);
    out->joystick_ok = driver_status.joystick_attached;
    out->haptic_ok = driver_status.rumble_supported;
    out->power_ok = driver_status.power_info_valid;

    if (!g_static_probed) {
        title_backend_probe_audio(out);
        g_static_audio_ok = out->audio_ok;
        SDL_strlcpy(g_static_audio_driver, out->audio_driver, sizeof(g_static_audio_driver));

        if (gl_effect_context_acquire()) {
            g_static_gl_ok = SDL_TRUE;
            gl_effect_context_release();
        }
        /* GL context creation steals window focus on this driver -- re-raise ours. */
        if (window) {
            SDL_RaiseWindow(window);
        }

        g_static_cpu_count = SDL_GetCPUCount();

        SDL_version v;
        SDL_GetVersion(&v);
        g_static_sdl_major = v.major;
        g_static_sdl_minor = v.minor;
        g_static_sdl_patch = v.patch;

        g_static_probed = SDL_TRUE;
    } else {
        out->audio_ok = g_static_audio_ok;
        SDL_strlcpy(out->audio_driver, g_static_audio_driver, sizeof(out->audio_driver));
    }

    out->gl_ok = g_static_gl_ok;
    out->cpu_count = g_static_cpu_count;
    out->sdl_major = g_static_sdl_major;
    out->sdl_minor = g_static_sdl_minor;
    out->sdl_patch = g_static_sdl_patch;

    bench_backend_probe_mma_pool(&out->mma_pool_used_bytes, &out->mma_pool_max_bytes);
}
