#ifndef COMMON_METRICS_H
#define COMMON_METRICS_H

#include <SDL2/SDL.h>
#include "common/types.h"

double bench_get_delta_seconds(Uint64 *last_counter, Uint64 perf_freq);
void bench_reset_metrics(BenchMetrics *metrics);
void bench_update_metrics(BenchMetrics *metrics, double frame_time_ms);

#endif /* COMMON_METRICS_H */
