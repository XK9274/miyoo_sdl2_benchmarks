#ifndef GL_FBO_EFFECTS_OVERLAY_H
#define GL_FBO_EFFECTS_OVERLAY_H

#include "bench_common.h"
#include "gl_fbo_effects/state.h"

void rsgl_overlay_submit(BenchOverlay *overlay,
                         const RsglState *state,
                         const BenchMetrics *metrics);

#endif /* GL_FBO_EFFECTS_OVERLAY_H */
