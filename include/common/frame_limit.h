#ifndef COMMON_FRAME_LIMIT_H
#define COMMON_FRAME_LIMIT_H

#include <SDL2/SDL.h>

/* Set by sdl2_title before exec'ing a suite. 0 or unset = no cap. */
#define BENCH_ENV_FRAME_LIMIT_FPS "BENCH_FRAME_LIMIT_FPS"

/* Reads BENCH_FRAME_LIMIT_FPS once; call at suite startup. */
void bench_frame_limit_load(void);

/* Sleeps out the remainder of the target frame interval, if a limit is active.
 * Call after SDL_RenderPresent and after metrics capture so the cap only
 * affects presentation pacing, not the logged unthrottled frame time. */
void bench_frame_limit_wait(Uint64 frame_start_counter);

#endif /* COMMON_FRAME_LIMIT_H */
