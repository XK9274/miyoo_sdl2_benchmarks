#ifndef AUDIO_BENCH_WAVEFORM_H
#define AUDIO_BENCH_WAVEFORM_H

#include <SDL2/SDL.h>

#include "common/types.h"

void waveform_reset(void);
void waveform_capture_stream(const Uint8 *stream,
                             Uint32 len,
                             Uint32 bytes_per_frame,
                             SDL_AudioFormat format,
                             int channels);
void waveform_draw(SDL_Renderer *target,
                   BenchMetrics *metrics,
                   int x,
                   int y,
                   int w,
                   int h);

void waveform_toggle_mode(void);

void waveform_draw_ui_area(SDL_Renderer *target,
                          BenchMetrics *metrics,
                          int x,
                          int y,
                          int w,
                          int h);

#endif /* AUDIO_BENCH_WAVEFORM_H */
