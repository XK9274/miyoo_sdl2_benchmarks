#ifndef AUDIO_BENCH_INPUT_H
#define AUDIO_BENCH_INPUT_H

#include <SDL2/SDL.h>

#include "common/overlay.h"
#include "common/types.h"

SDL_bool audio_handle_input(BenchMetrics *metrics, BenchOverlay *overlay);

#endif /* AUDIO_BENCH_INPUT_H */
