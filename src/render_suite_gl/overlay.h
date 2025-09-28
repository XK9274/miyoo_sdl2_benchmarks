#ifndef RENDER_SUITE_GL_OVERLAY_H
#define RENDER_SUITE_GL_OVERLAY_H

#include "bench_common.h"
#include "render_suite_gl/state.h"

void rsgl_overlay_submit(BenchOverlay *overlay,
                         const RsglState *state,
                         const BenchMetrics *metrics);

#endif /* RENDER_SUITE_GL_OVERLAY_H */
