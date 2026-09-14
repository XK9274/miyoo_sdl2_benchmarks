#ifndef COMMON_PROFILE_CAPTURE_H
#define COMMON_PROFILE_CAPTURE_H

#include <stdio.h>

#include <SDL2/SDL.h>

#include "common/types.h"

/* Set by the profiler orchestrator before exec'ing a suite; read once at
 * suite startup. */
#define BENCH_ENV_PROFILE_TAG "BENCH_PROFILE_TAG"
#define BENCH_ENV_PROFILE_DURATION_S "BENCH_PROFILE_DURATION_S"
#define BENCH_ENV_PROFILE_OUTPUT_PATH "BENCH_PROFILE_OUTPUT_PATH"

#define BENCH_PROFILE_DEFAULT_DURATION_S 5.0
#define BENCH_PROFILE_SAMPLE_INTERVAL_MS 500.0

typedef struct {
    SDL_bool active;
    char tag[64];
    double duration_s;
    double next_sample_ms;
    FILE *file;
} BenchProfileCapture;

/* Reads the BENCH_PROFILE_* env vars once. cap->active stays SDL_FALSE when
 * BENCH_PROFILE_OUTPUT_PATH is unset, so a suite launched outside the
 * profiler runs untimed at zero cost. */
void bench_profile_load(BenchProfileCapture *cap);

/* Call once per frame, after that frame's metrics are updated. Appends one
 * CSV sample row at most every BENCH_PROFILE_SAMPLE_INTERVAL_MS. Returns
 * SDL_TRUE once the configured duration has elapsed, telling the caller's
 * main loop to exit. */
SDL_bool bench_profile_update(BenchProfileCapture *cap, const BenchMetrics *metrics);

/* Flushes/closes the output file. Safe to call even if never activated. */
void bench_profile_shutdown(BenchProfileCapture *cap);

#endif /* COMMON_PROFILE_CAPTURE_H */
