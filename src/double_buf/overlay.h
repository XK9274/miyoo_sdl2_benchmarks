#ifndef DOUBLE_BUF_OVERLAY_H
#define DOUBLE_BUF_OVERLAY_H

#include "double_buf/state.h"

void db_overlay_submit(BenchOverlay *overlay,
                       const DoubleBenchState *state,
                       const BenchMetrics *metrics);

#endif /* DOUBLE_BUF_OVERLAY_H */
