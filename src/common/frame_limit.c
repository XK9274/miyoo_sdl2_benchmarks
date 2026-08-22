#include "common/frame_limit.h"

#include <stdlib.h>

static int g_target_fps = 0;

void bench_frame_limit_load(void)
{
    const char *fps_str = SDL_getenv(BENCH_ENV_FRAME_LIMIT_FPS);
    g_target_fps = fps_str ? SDL_atoi(fps_str) : 0;
    if (g_target_fps < 0) {
        g_target_fps = 0;
    }
}

void bench_frame_limit_wait(Uint64 frame_start_counter)
{
    if (g_target_fps <= 0) {
        return;
    }

    const Uint64 freq = SDL_GetPerformanceFrequency();
    const Uint64 target_ticks = freq / (Uint64)g_target_fps;
    const Uint64 now = SDL_GetPerformanceCounter();
    const Uint64 elapsed = now - frame_start_counter;

    if (elapsed >= target_ticks) {
        return;
    }

    const Uint64 remaining_ticks = target_ticks - elapsed;
    const Uint32 remaining_ms = (Uint32)((remaining_ticks * 1000ULL) / freq);
    if (remaining_ms > 0) {
        SDL_Delay(remaining_ms);
    }
}
