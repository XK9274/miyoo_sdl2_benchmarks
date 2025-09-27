#ifndef SPACE_BENCH_OVERLAY_H
#define SPACE_BENCH_OVERLAY_H

#include "bench_common.h"
#include "space_bench/state.h"

void space_overlay_submit(BenchOverlay *overlay,
                          const SpaceBenchState *state,
                          const BenchMetrics *metrics);

#endif /* SPACE_BENCH_OVERLAY_H */
