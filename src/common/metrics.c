#include "common/metrics.h"

#include <float.h>

#include <SDL2/SDL.h>

void bench_reset_metrics(BenchMetrics *metrics)
{
    if (!metrics) {
        return;
    }
    SDL_memset(metrics, 0, sizeof(*metrics));
    metrics->min_fps = DBL_MAX;
    metrics->min_frame_time_ms = DBL_MAX;
    metrics->accumulated_frame_time_ms = 0.0;
}

double bench_get_delta_seconds(Uint64 *last_counter, Uint64 perf_freq)
{
    Uint64 current = SDL_GetPerformanceCounter();
    double delta = 0.0;
    if (last_counter && *last_counter != 0 && perf_freq != 0) {
        delta = (double)(current - *last_counter) / (double)perf_freq;
    }
    if (last_counter) {
        *last_counter = current;
    }
    if (delta <= 0.0) {
        delta = 1.0 / 600.0;
    }
    return delta;
}

void bench_update_metrics(BenchMetrics *metrics, double frame_time_ms)
{
    if (!metrics) {
        return;
    }

    metrics->frame_count++;
    metrics->frame_time_ms = frame_time_ms;
    metrics->accumulated_frame_time_ms += frame_time_ms;

    if (frame_time_ms < metrics->min_frame_time_ms) {
        metrics->min_frame_time_ms = frame_time_ms;
    }
    if (frame_time_ms > metrics->max_frame_time_ms) {
        metrics->max_frame_time_ms = frame_time_ms;
    }

    const double fps = (frame_time_ms > 0.0) ? 1000.0 / frame_time_ms : 0.0;
    metrics->current_fps = fps;
    if (fps < metrics->min_fps) {
        metrics->min_fps = fps;
    }
    if (fps > metrics->max_fps) {
        metrics->max_fps = fps;
    }

    if (metrics->accumulated_frame_time_ms > 0.0) {
        const double total_seconds = metrics->accumulated_frame_time_ms / 1000.0;
        if (total_seconds > 0.0) {
            metrics->avg_fps = metrics->frame_count / total_seconds;
        }
    }
}
