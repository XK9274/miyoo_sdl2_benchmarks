#include "audio_bench/input.h"

#include <SDL2/SDL.h>

#include "audio_bench/audio_device.h"
#include "audio_bench/waveform.h"
#include "controller_input.h"
#include "common/metrics.h"

SDL_bool audio_handle_input(BenchMetrics *metrics)
{
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            return SDL_FALSE;
        }
        if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case BTN_START:
                case BTN_EXIT:
                case SDLK_ESCAPE:
                    return SDL_FALSE;
                case BTN_A:
                    if (audio_device_is_playing()) {
                        audio_device_stop(SDL_FALSE);
                    } else {
                        audio_device_play();
                    }
                    break;
                case BTN_B:
                    audio_device_stop(SDL_TRUE);
                    audio_device_play();
                    break;
                case BTN_LEFT:
                case BTN_L2:
                    audio_device_seek(-5.0f);
                    break;
                case BTN_RIGHT:
                case BTN_R2:
                    audio_device_seek(5.0f);
                    break;
                case BTN_UP:
                case BTN_R1:
                    audio_device_adjust_volume(0.1f);
                    break;
                case BTN_DOWN:
                case BTN_L1:
                    audio_device_adjust_volume(-0.1f);
                    break;
                case BTN_X:
                    bench_reset_metrics(metrics);
                    break;
                case BTN_Y:
                    waveform_toggle_mode();
                    break;
                default:
                    break;
            }
        }
    }
    return SDL_TRUE;
}
