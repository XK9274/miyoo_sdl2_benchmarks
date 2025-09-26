#ifndef SOFTWARE_BUF_OVERLAY_H
#define SOFTWARE_BUF_OVERLAY_H

#include "software_buf/state.h"

void sb_overlay_submit(BenchOverlay *overlay,
                       const SoftwareBenchState *state,
                       const BenchMetrics *metrics);

#endif /* SOFTWARE_BUF_OVERLAY_H */
