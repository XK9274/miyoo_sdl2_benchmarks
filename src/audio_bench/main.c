#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <math.h>
#include <stdio.h>

#include "audio_bench/audio_device.h"
#include "audio_bench/input.h"
#include "audio_bench/waveform.h"
#include "bench_common.h"
#include "common/format.h"
#include "common/hotkeys.h"
#include "common/loading_screen.h"
#include "common/overlay_rows.h"
#include "controller_input.h"

/* Configured logical size (not the fixed native window size -- see BENCH_NATIVE_W/H). */
#define SCREEN_W bench_logical_w()
#define SCREEN_H bench_logical_h()

static const OverlayRowSpec g_audio_rows[] = {
    {OVERLAY_ROW_CUSTOM, {240, 194, 94, 255}, 0, "%s"},
    {OVERLAY_ROW_FPS, {255, 255, 255, 255}, 0, NULL},
    {OVERLAY_ROW_CUSTOM, {0, 255, 160, 255}, 0, "%s"},
    {OVERLAY_ROW_CUSTOM, {0, 200, 255, 255}, 0, "%s"},
    {OVERLAY_ROW_CUSTOM, {255, 255, 255, 255}, 0, "%s"},
    {OVERLAY_ROW_CUSTOM, {0, 255, 160, 255}, 0, "%s"},
    {OVERLAY_ROW_CUSTOM, {255, 180, 120, 255}, 0, "%s"},
    {OVERLAY_ROW_CUSTOM, {0, 200, 255, 255}, 0, "%s"},
};

static const OverlayKeybind g_audio_keybinds[] = {
    {"SELECT", "Toggle overlay"},
    {"MENU/X", "Reset metrics"},
    {"A", "Play/pause"},
    {"B", "Restart track"},
    {"LEFT/L2", "Seek -5s"},
    {"RIGHT/R2", "Seek +5s"},
    {"UP/R1", "Volume +"},
    {"DOWN/L1", "Volume -"},
    {"Y", "Toggle draw mode"},
};

static const char *audio_format_label(SDL_AudioFormat format)
{
    switch (format) {
        case AUDIO_U8: return "U8";
        case AUDIO_S8: return "S8";
        case AUDIO_U16LSB: return "U16";
        case AUDIO_S16LSB: return "S16";
        case AUDIO_S16MSB: return "S16BE";
        case AUDIO_S32LSB: return "S32";
        case AUDIO_S32MSB: return "S32BE";
        case AUDIO_F32LSB: return "F32";
        case AUDIO_F32MSB: return "F32BE";
        default: return "Unknown";
    }
}

static void audio_format_time_string(double seconds, char *buffer, size_t buffer_size)
{
    if (!buffer || buffer_size == 0 || seconds < 0.0) {
        if (buffer && buffer_size > 0) {
            buffer[0] = '\0';
        }
        return;
    }
    const int total_seconds = (int)floor(seconds + 0.5);
    const int mins = total_seconds / 60;
    const int secs = total_seconds % 60;
    const double fractional = seconds - floor(seconds);
    const int millis = (int)floor(fractional * 1000.0 + 0.5);
    if (millis > 0) {
        snprintf(buffer, buffer_size, "%02d:%02d.%03d", mins, secs, millis);
    } else {
        snprintf(buffer, buffer_size, "%02d:%02d", mins, secs);
    }
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    if (SDL_getenv("SDL_AUDIODRIVER") == NULL) {
        SDL_setenv("SDL_AUDIODRIVER", "mmiyoo", 0);
    }

    Uint64 perf_freq = SDL_GetPerformanceFrequency();
    Uint64 last_counter = SDL_GetPerformanceCounter();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() < 0) {
        printf("TTF_Init failed: %s\n", TTF_GetError());
    }

    SDL_Window *window = SDL_CreateWindow("SDL2 Audio Bench",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          BENCH_NATIVE_W, BENCH_NATIVE_H,
                                          SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    /* SDL_CreateRenderer force-ORs in SDL_RENDERER_PRESENTVSYNC in this SDL2
     * fork regardless of flags -- the hint is the only way to turn it off. */
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "0");
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    int logical_w, logical_h;
    bench_display_config_load(&logical_w, &logical_h);
    bench_display_config_apply(renderer, logical_w, logical_h);
    bench_frame_limit_load();

    bench_driver_init(window, renderer);

    BenchLoadingScreen loading;
    SDL_bool loading_active = bench_loading_begin(&loading,
                                                  window,
                                                  renderer,
                                                  BENCH_LOADING_STYLE_RECT);
    if (loading_active) {
        bench_loading_step(&loading, 0.1f, "Setting up overlay");
    }

    BenchOverlay *overlay = bench_overlay_create(renderer, SCREEN_W, 16, 12);
    if (!overlay) {
        printf("Overlay creation failed\n");
        if (loading_active) {
            bench_loading_abort(&loading);
        }
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    if (loading_active) {
        bench_loading_step(&loading, 0.25f, "Initialising audio device");
    }
    if (!audio_device_init()) {
        if (loading_active) {
            bench_loading_abort(&loading);
        }
        bench_overlay_destroy(overlay);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    BenchMetrics metrics;
    bench_reset_metrics(&metrics);

    bench_overlay_configure(overlay, g_audio_rows, (int)SDL_arraysize(g_audio_rows),
                            g_audio_keybinds, (int)SDL_arraysize(g_audio_keybinds));

    audio_device_play();
    if (loading_active) {
        bench_loading_step(&loading, 1.0f, "Audio bench ready");
        bench_loading_mark_idle(&loading, "GL modules idle - audio focus");
        bench_loading_finish(&loading);
        loading_active = SDL_FALSE;
    }
    printf("SDL2 audio bench started\n");

    SDL_bool running = SDL_TRUE;
    while (running) {
        const Uint64 frame_start_counter = SDL_GetPerformanceCounter();

        running = audio_handle_input(&metrics, overlay);
        if (!running) {
            break;
        }

        const double delta_seconds = bench_get_delta_seconds(&last_counter, perf_freq);

        // Reduce render frequency when paused
        static Uint32 last_render = 0;
        Uint32 now = SDL_GetTicks();
        const SDL_bool is_playing = audio_device_is_playing();
        const Uint32 render_interval = is_playing ? 16 : 33; // 60fps vs 30fps

        if (now - last_render < render_interval) {
            SDL_Delay(1); // Small delay to prevent 100% CPU
            continue;
        }
        last_render = now;

        metrics.draw_calls = 0;
        metrics.vertices_rendered = 0;
        metrics.triangles_rendered = 0;

        const int margin = 12;
        const int available_height = SCREEN_H - margin * 2;

        // Split available space: 30% for new UI area, 70% for waveform
        const int ui_area_height = (int)(available_height * 0.3f);
        const int waveform_height = available_height - ui_area_height - margin;
        const int ui_area_y = margin;
        const int waveform_y = ui_area_y + ui_area_height + margin;

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer, 8, 10, 18, 255);
        SDL_RenderClear(renderer);
        metrics.draw_calls++;

        // Draw new UI area
        if (ui_area_height > 20) {
            waveform_draw_ui_area(renderer,
                                  &metrics,
                                  margin,
                                  ui_area_y,
                                  SCREEN_W - margin * 2,
                                  ui_area_height);
        }

        // Draw main waveform visualizer
        if (waveform_height > 36) {
            waveform_draw(renderer,
                          &metrics,
                          margin,
                          waveform_y,
                          SCREEN_W - margin * 2,
                          waveform_height);
        }

        bench_overlay_present(overlay, renderer, &metrics, 0, 0);
        SDL_RenderPresent(renderer);

        bench_update_metrics(&metrics, delta_seconds * 1000.0);
        bench_frame_limit_wait(frame_start_counter);

        AudioSnapshot snapshot;
        audio_device_get_snapshot(&snapshot);
        const char *driver = SDL_GetCurrentAudioDriver();

        char driver_label[48];
        snprintf(driver_label, sizeof(driver_label), "Driver: %s", driver ? driver : "unknown");

        Uint32 bytes_per_frame = 0;
        if (snapshot.spec.channels > 0 && snapshot.spec.format != 0) {
            bytes_per_frame = (Uint32)(SDL_AUDIO_BITSIZE(snapshot.spec.format) / 8 * snapshot.spec.channels);
        }
        const char *channel_label = bench_audio_channel_label(snapshot.spec.channels);
        const char *format_label = audio_format_label(snapshot.spec.format);
        double bitrate_kbps = 0.0;
        if (snapshot.spec.freq > 0 && bytes_per_frame > 0) {
            bitrate_kbps = ((double)snapshot.spec.freq * (double)bytes_per_frame * 8.0) / 1000.0;
        }
        char format_line[80];
        snprintf(format_line, sizeof(format_line), "%d Hz | %s (%d ch) | %s | %.0f kbps",
                 snapshot.spec.freq, channel_label, snapshot.spec.channels, format_label, bitrate_kbps);

        char callback_line[80];
        char time_line[96];
        char cursor_line[96];
        if (bytes_per_frame > 0 && snapshot.spec.freq > 0) {
            const Uint32 callback_frames = snapshot.spec.samples;
            const Uint32 callback_bytes = (Uint32)(callback_frames * bytes_per_frame);
            const double callback_ms = (callback_frames > 0) ?
                ((double)callback_frames / (double)snapshot.spec.freq) * 1000.0 : 0.0;
            char callback_bytes_str[32];
            bench_format_bytes_human(callback_bytes, callback_bytes_str, sizeof(callback_bytes_str));
            snprintf(callback_line, sizeof(callback_line), "Callback %u frames (%.2f ms) | %s per fill",
                     callback_frames, callback_ms, callback_bytes_str);

            const double seconds_played = (double)snapshot.frames_played / (double)snapshot.spec.freq;
            const double seconds_total = (double)snapshot.length_bytes /
                                         (double)(bytes_per_frame * snapshot.spec.freq);
            char time_played[32], time_total[32];
            audio_format_time_string(seconds_played, time_played, sizeof(time_played));
            audio_format_time_string(seconds_total, time_total, sizeof(time_total));
            const Uint64 total_frames = (bytes_per_frame > 0) ? (snapshot.length_bytes / bytes_per_frame) : 0;
            snprintf(time_line, sizeof(time_line), "Time %s / %s | Frame %llu / %llu",
                     time_played, time_total, (unsigned long long)snapshot.frames_played,
                     (unsigned long long)total_frames);

            const Uint32 remaining_bytes = (snapshot.length_bytes > snapshot.position_bytes) ?
                (snapshot.length_bytes - snapshot.position_bytes) : 0;
            char played_str[32], remaining_str[32];
            bench_format_bytes_human(snapshot.position_bytes, played_str, sizeof(played_str));
            bench_format_bytes_human(remaining_bytes, remaining_str, sizeof(remaining_str));
            snprintf(cursor_line, sizeof(cursor_line), "Cursor %s | Remaining %s", played_str, remaining_str);
        } else {
            snprintf(callback_line, sizeof(callback_line), "Audio device not initialised");
            time_line[0] = '\0';
            cursor_line[0] = '\0';
        }

        char volume_line[64];
        snprintf(volume_line, sizeof(volume_line), "Volume %.0f%% | Loop %s | Playing %s",
                 snapshot.volume * 100.0f, snapshot.loop ? "ON" : "OFF", snapshot.playing ? "YES" : "NO");
        char mode_line[48];
        snprintf(mode_line, sizeof(mode_line), "Draw Method: %s", waveform_get_mode_name());

        const char *custom_values[] = {
            driver_label, format_line, callback_line, time_line, cursor_line, volume_line, mode_line,
        };
        bench_overlay_update(overlay, &metrics, custom_values, (int)SDL_arraysize(custom_values));
    }

    audio_device_stop(SDL_FALSE);
    audio_device_shutdown();

    bench_driver_shutdown();
    bench_overlay_destroy(overlay);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
