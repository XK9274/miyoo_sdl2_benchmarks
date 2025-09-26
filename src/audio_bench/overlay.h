#ifndef AUDIO_BENCH_OVERLAY_H
#define AUDIO_BENCH_OVERLAY_H

#include <SDL2/SDL.h>

#include "audio_bench/audio_device.h"
#include "common/overlay.h"
#include "common/types.h"

void audio_overlay_start(BenchOverlay *overlay, BenchMetrics *metrics);
void audio_overlay_stop(void);

#endif /* AUDIO_BENCH_OVERLAY_H */
