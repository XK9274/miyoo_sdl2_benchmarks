#ifndef GFX_BENCH_OVERLAY_H
#define GFX_BENCH_OVERLAY_H

#include "gfx_bench/state.h"

void gb_overlay_submit(BenchOverlay *overlay,
                       const GfxBenchState *state,
                       const BenchMetrics *metrics);

#endif /* GFX_BENCH_OVERLAY_H */
