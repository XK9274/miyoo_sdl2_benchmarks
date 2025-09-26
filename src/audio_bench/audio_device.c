#include "audio_bench/audio_device.h"

#include <SDL2/SDL_atomic.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "audio_bench/waveform.h"

typedef struct {
    Uint8 *buffer;
    Uint32 length_bytes;
    Uint32 position_bytes;
    SDL_bool playing;
    SDL_bool loop;
    float volume;
    SDL_mutex *lock;
    Uint64 frames_played;
    SDL_AudioSpec spec;
    SDL_AudioDeviceID device;
} AudioState;

static const char *AUDIO_PATHS[] = {
    "assets/bgm.wav",
    "../assets/bgm.wav",
    "/home/mattpc/HueTesting/union-miyoomini-toolchain/workspace/testSDL2_fps/assets/bgm.wav",
    NULL
};

static AudioState audio_state;
static SDL_atomic_t atomic_position_bytes;
static SDL_atomic_t atomic_length_bytes;
static SDL_atomic_t atomic_frames_played;
static SDL_atomic_t atomic_playing;
static SDL_atomic_t atomic_loop;
static SDL_atomic_t atomic_volume_int;
static SDL_atomic_t atomic_device_ready;

static const char *resolve_audio_path(void)
{
    for (int i = 0; AUDIO_PATHS[i] != NULL; ++i) {
        if (access(AUDIO_PATHS[i], F_OK) == 0) {
            return AUDIO_PATHS[i];
        }
    }
    return NULL;
}

static void update_atomics_locked(void)
{
    SDL_AtomicSet(&atomic_position_bytes, (int)audio_state.position_bytes);
    SDL_AtomicSet(&atomic_frames_played, (int)audio_state.frames_played);
    SDL_AtomicSet(&atomic_playing, audio_state.playing ? 1 : 0);
    SDL_AtomicSet(&atomic_volume_int, (int)(audio_state.volume * 1000.0f));
    SDL_AtomicSet(&atomic_loop, audio_state.loop ? 1 : 0);
}

static void audio_callback(void *userdata, Uint8 *stream, int len)
{
    AudioState *state = (AudioState *)userdata;
    SDL_memset(stream, 0, len);

    SDL_LockMutex(state->lock);
    if (!state->buffer || state->length_bytes == 0) {
        SDL_UnlockMutex(state->lock);
        return;
    }

    const SDL_AudioFormat format = state->spec.format;
    const int channels = state->spec.channels;
    const Uint32 bytes_per_frame = (Uint32)(SDL_AUDIO_BITSIZE(format) / 8 * SDL_max(1, channels));
    Uint32 captured_bytes = 0;

    if (state->playing) {
        Uint32 remaining = state->length_bytes - state->position_bytes;
        Uint32 to_copy = (Uint32)len;
        if (to_copy > remaining) {
            to_copy = remaining;
        }

        SDL_MixAudioFormat(stream,
                           state->buffer + state->position_bytes,
                           format,
                           to_copy,
                           (int)(SDL_MIX_MAXVOLUME * state->volume));

        state->position_bytes += to_copy;
        state->frames_played += to_copy / bytes_per_frame;
        captured_bytes = to_copy;

        if (state->position_bytes >= state->length_bytes) {
            if (state->loop) {
                state->position_bytes = 0;
            } else {
                state->playing = SDL_FALSE;
            }
        }
    }

    update_atomics_locked();
    SDL_UnlockMutex(state->lock);

    if (captured_bytes > 0 && bytes_per_frame > 0) {
        waveform_capture_stream(stream,
                                 captured_bytes,
                                 bytes_per_frame,
                                 format,
                                 channels);
    }
}

SDL_bool audio_device_init(void)
{
    SDL_memset(&audio_state, 0, sizeof(audio_state));

    const char *path = resolve_audio_path();
    if (!path) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Audio file not found");
        return SDL_FALSE;
    }

    SDL_AudioSpec wav_spec;
    Uint8 *wav_buffer = NULL;
    Uint32 wav_length = 0;

    if (!SDL_LoadWAV(path, &wav_spec, &wav_buffer, &wav_length)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_LoadWAV failed: %s", SDL_GetError());
        return SDL_FALSE;
    }

    audio_state.spec = wav_spec;
    audio_state.buffer = wav_buffer;
    audio_state.length_bytes = wav_length;
    audio_state.position_bytes = 0;
    audio_state.playing = SDL_FALSE;
    audio_state.loop = SDL_TRUE;
    audio_state.volume = 1.0f;
    audio_state.frames_played = 0;
    audio_state.lock = SDL_CreateMutex();

    if (!audio_state.lock) {
        SDL_FreeWAV(audio_state.buffer);
        audio_state.buffer = NULL;
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create audio mutex: %s", SDL_GetError());
        return SDL_FALSE;
    }

    SDL_AtomicSet(&atomic_length_bytes, (int)wav_length);
    SDL_AtomicSet(&atomic_position_bytes, 0);
    SDL_AtomicSet(&atomic_frames_played, 0);
    SDL_AtomicSet(&atomic_playing, 0);
    SDL_AtomicSet(&atomic_loop, 1);
    SDL_AtomicSet(&atomic_volume_int, 1000);
    SDL_AtomicSet(&atomic_device_ready, 0);

    waveform_reset();

    SDL_AudioSpec want = wav_spec;
    want.callback = audio_callback;
    want.userdata = &audio_state;

    audio_state.device = SDL_OpenAudioDevice(NULL, 0, &want, &audio_state.spec, 0);
    if (audio_state.device == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_OpenAudioDevice failed: %s", SDL_GetError());
        SDL_DestroyMutex(audio_state.lock);
        audio_state.lock = NULL;
        SDL_FreeWAV(audio_state.buffer);
        audio_state.buffer = NULL;
        return SDL_FALSE;
    }

    SDL_PauseAudioDevice(audio_state.device, 0);
    SDL_AtomicSet(&atomic_device_ready, 1);
    return SDL_TRUE;
}

void audio_device_shutdown(void)
{
    if (audio_state.device) {
        SDL_CloseAudioDevice(audio_state.device);
        audio_state.device = 0;
    }
    if (audio_state.lock) {
        SDL_DestroyMutex(audio_state.lock);
        audio_state.lock = NULL;
    }
    if (audio_state.buffer) {
        SDL_FreeWAV(audio_state.buffer);
        audio_state.buffer = NULL;
    }
}

void audio_device_play(void)
{
    SDL_LockMutex(audio_state.lock);
    audio_state.playing = SDL_TRUE;
    update_atomics_locked();
    SDL_UnlockMutex(audio_state.lock);
}

void audio_device_stop(SDL_bool reset_position)
{
    SDL_LockMutex(audio_state.lock);
    audio_state.playing = SDL_FALSE;
    if (reset_position) {
        audio_state.position_bytes = 0;
        audio_state.frames_played = 0;
    }
    update_atomics_locked();
    SDL_UnlockMutex(audio_state.lock);

    if (reset_position) {
        waveform_reset();
    }
}

void audio_device_seek(float delta_seconds)
{
    SDL_LockMutex(audio_state.lock);
    const Uint32 bytes_per_frame = (Uint32)(SDL_AUDIO_BITSIZE(audio_state.spec.format) / 8 * audio_state.spec.channels);
    if (delta_seconds < 0.0f) {
        const Uint32 delta_bytes = (Uint32)(-delta_seconds * audio_state.spec.freq) * bytes_per_frame;
        if (delta_bytes >= audio_state.position_bytes) {
            audio_state.position_bytes = 0;
        } else {
            audio_state.position_bytes -= delta_bytes;
        }
        audio_state.frames_played = audio_state.position_bytes / bytes_per_frame;
    } else if (delta_seconds > 0.0f) {
        Uint32 delta_bytes = (Uint32)(delta_seconds * audio_state.spec.freq) * bytes_per_frame;
        if (audio_state.position_bytes + delta_bytes >= audio_state.length_bytes) {
            if (audio_state.loop) {
                audio_state.position_bytes = (audio_state.position_bytes + delta_bytes) % audio_state.length_bytes;
            } else {
                audio_state.position_bytes = audio_state.length_bytes;
                audio_state.playing = SDL_FALSE;
            }
        } else {
            audio_state.position_bytes += delta_bytes;
        }
        audio_state.frames_played = audio_state.position_bytes / bytes_per_frame;
    }
    update_atomics_locked();
    SDL_UnlockMutex(audio_state.lock);
}

void audio_device_adjust_volume(float delta)
{
    SDL_LockMutex(audio_state.lock);
    audio_state.volume += delta;
    if (audio_state.volume < 0.0f) {
        audio_state.volume = 0.0f;
    }
    if (audio_state.volume > 1.0f) {
        audio_state.volume = 1.0f;
    }
    update_atomics_locked();
    SDL_UnlockMutex(audio_state.lock);
}

float audio_device_get_volume(void)
{
    return (float)SDL_AtomicGet(&atomic_volume_int) / 1000.0f;
}

SDL_bool audio_device_is_playing(void)
{
    return SDL_AtomicGet(&atomic_playing) ? SDL_TRUE : SDL_FALSE;
}

void audio_device_get_snapshot(AudioSnapshot *snapshot)
{
    if (!snapshot) {
        return;
    }

    SDL_memset(snapshot, 0, sizeof(*snapshot));

    snapshot->position_bytes = (Uint32)SDL_AtomicGet(&atomic_position_bytes);
    snapshot->length_bytes = (Uint32)SDL_AtomicGet(&atomic_length_bytes);
    snapshot->frames_played = (Uint64)SDL_AtomicGet(&atomic_frames_played);
    snapshot->playing = SDL_AtomicGet(&atomic_playing) ? SDL_TRUE : SDL_FALSE;
    snapshot->loop = SDL_AtomicGet(&atomic_loop) ? SDL_TRUE : SDL_FALSE;
    snapshot->volume = (float)SDL_AtomicGet(&atomic_volume_int) / 1000.0f;

    if (SDL_AtomicGet(&atomic_device_ready)) {
        snapshot->spec = audio_state.spec;
    }
}
