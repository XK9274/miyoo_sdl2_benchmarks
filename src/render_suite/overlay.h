#ifndef RENDER_SUITE_OVERLAY_H
#define RENDER_SUITE_OVERLAY_H

#include "render_suite/state.h"

void rs_overlay_submit(BenchOverlay *overlay,
                       const RenderSuiteState *state,
                       const BenchMetrics *metrics);

#endif /* RENDER_SUITE_OVERLAY_H */
