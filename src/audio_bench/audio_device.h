#ifndef AUDIO_BENCH_AUDIO_DEVICE_H
#define AUDIO_BENCH_AUDIO_DEVICE_H

#include <SDL2/SDL.h>

#include "common/types.h"

typedef struct {
    SDL_AudioSpec spec;
    Uint32 length_bytes;
    Uint32 position_bytes;
    SDL_bool playing;
    SDL_bool loop;
    float volume;
    Uint64 frames_played;
} AudioSnapshot;

SDL_bool audio_device_init(void);
void audio_device_shutdown(void);

void audio_device_play(void);
void audio_device_stop(SDL_bool reset_position);
void audio_device_seek(float delta_seconds);
void audio_device_adjust_volume(float delta);
float audio_device_get_volume(void);
SDL_bool audio_device_is_playing(void);

void audio_device_get_snapshot(AudioSnapshot *snapshot);

#endif /* AUDIO_BENCH_AUDIO_DEVICE_H */
