#include "audio_bench/overlay.h"

#include <float.h>

#include <SDL2/SDL_atomic.h>

#include "common/format.h"
#include "common/overlay_grid.h"

static SDL_Thread *s_overlay_thread = NULL;
static SDL_atomic_t s_overlay_running;
static BenchOverlay *s_overlay = NULL;
static BenchMetrics *s_metrics = NULL;

static void format_time_string(double seconds,
                               char *buffer,
                               size_t buffer_size)
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
        SDL_snprintf(buffer, buffer_size, "%02d:%02d.%03d", mins, secs, millis);
    } else {
        SDL_snprintf(buffer, buffer_size, "%02d:%02d", mins, secs);
    }
}

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

static int overlay_thread_func(void *data)
{
    (void)data;

    while (SDL_AtomicGet(&s_overlay_running)) {
        if (!s_overlay) {
            SDL_Delay(16);
            continue;
        }

        AudioSnapshot snapshot;
        audio_device_get_snapshot(&snapshot);

        const char *driver = SDL_GetCurrentAudioDriver();
        const SDL_Color accent = {255, 215, 0, 255};
        const SDL_Color primary = {255, 255, 255, 255};
        const SDL_Color cyan = {0, 200, 255, 255};
        const SDL_Color green = {0, 255, 160, 255};
        const SDL_Color amber = {255, 180, 120, 255};
        const SDL_Color info = {255, 200, 0, 255};

        OverlayGrid grid;
        overlay_grid_init(&grid, 2, 10);  // 2 columns, 10 rows (standardized)
        overlay_grid_set_background(&grid, (SDL_Color){0, 0, 0, 210});

        // Row 0 - Headers (centered)
        overlay_grid_set_cell(&grid, 0, 0, accent, 1, "SDL2 Audio Bench");
        overlay_grid_set_cell(&grid, 0, 1, accent, 1, "Control Scheme");

        // Row 1 - Driver info left, first control right
        overlay_grid_set_cell(&grid, 1, 0, primary, 0, "Driver: %s", driver ? driver : "unknown");
        overlay_grid_set_cell(&grid, 1, 1, primary, 0, "A - Play/Pause");

        // Row 2 - FPS metrics
        if (s_metrics) {
            overlay_grid_set_cell(&grid, 2, 0, primary, 0,
                                "FPS %.1f (min %.1f / max %.1f / avg %.1f)",
                                s_metrics->current_fps,
                                (s_metrics->min_fps == DBL_MAX) ? 0.0 : s_metrics->min_fps,
                                s_metrics->max_fps,
                                s_metrics->avg_fps);
        } else {
            overlay_grid_set_cell(&grid, 2, 0, primary, 0, "FPS tracking unavailable");
        }
        overlay_grid_set_cell(&grid, 2, 1, primary, 0, "B - Restart Track");

        // Row 3 - Audio format info
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

        overlay_grid_set_cell(&grid, 3, 0, green, 0,
                            "%d Hz | %s (%d ch) | %s | %.0f kbps",
                            snapshot.spec.freq,
                            channel_label,
                            snapshot.spec.channels,
                            format_label,
                            bitrate_kbps);
        overlay_grid_set_cell(&grid, 3, 1, primary, 0, "LEFT/L2 - Seek -5s");

        // Row 4 - Callback info
        if (bytes_per_frame > 0 && snapshot.spec.freq > 0) {
            const Uint32 callback_frames = snapshot.spec.samples;
            const Uint32 callback_bytes = (Uint32)(callback_frames * bytes_per_frame);
            double callback_ms = 0.0;
            if (callback_frames > 0) {
                callback_ms = ((double)callback_frames / (double)snapshot.spec.freq) * 1000.0;
            }
            char callback_bytes_str[32];
            bench_format_bytes_human(callback_bytes, callback_bytes_str, sizeof(callback_bytes_str));
            overlay_grid_set_cell(&grid, 4, 0, cyan, 0,
                                "Callback %u frames (%.2f ms) | %s per fill",
                                callback_frames,
                                callback_ms,
                                callback_bytes_str);
            overlay_grid_set_cell(&grid, 4, 1, primary, 0, "RIGHT/R2 - Seek +5s");

            // Row 5 - Time and frame info
            const double seconds_played = (double)snapshot.frames_played /
                                          (double)snapshot.spec.freq;
            const double seconds_total = (double)snapshot.length_bytes /
                                         (double)(bytes_per_frame * snapshot.spec.freq);
            char time_played[32];
            char time_total[32];
            format_time_string(seconds_played, time_played, sizeof(time_played));
            format_time_string(seconds_total, time_total, sizeof(time_total));

            const Uint64 total_frames = (bytes_per_frame > 0)
                ? (snapshot.length_bytes / bytes_per_frame)
                : 0;
            overlay_grid_set_cell(&grid, 5, 0, primary, 0,
                                "Time %s / %s | Frame %llu / %llu",
                                time_played,
                                time_total,
                                (unsigned long long)snapshot.frames_played,
                                (unsigned long long)total_frames);
            overlay_grid_set_cell(&grid, 5, 1, primary, 0, "UP/DOWN - Volume");

            // Row 6 - Cursor and remaining data
            const Uint32 remaining_bytes = (snapshot.length_bytes > snapshot.position_bytes)
                ? (snapshot.length_bytes - snapshot.position_bytes)
                : 0;
            char remaining_str[32];
            char played_str[32];
            bench_format_bytes_human(snapshot.position_bytes, played_str, sizeof(played_str));
            bench_format_bytes_human(remaining_bytes, remaining_str, sizeof(remaining_str));
            overlay_grid_set_cell(&grid, 6, 0, green, 0,
                                "Cursor %s | Remaining %s",
                                played_str,
                                remaining_str);
            overlay_grid_set_cell(&grid, 6, 1, info, 0, "START/ESC - Exit");
        } else {
            overlay_grid_set_cell(&grid, 4, 0, cyan, 0, "Audio device not initialised");
            overlay_grid_set_cell(&grid, 4, 1, primary, 0, "RIGHT/R2 - Seek +5s");
            // Clear rows 5-6 when no audio
            overlay_grid_clear_row(&grid, 5);
            overlay_grid_set_cell(&grid, 6, 1, info, 0, "START/ESC - Exit");
        }

        // Row 7 - Volume and status info
        overlay_grid_set_cell(&grid, 7, 0, amber, 0,
                            "Volume %.0f%% | Loop %s | Playing %s",
                            snapshot.volume * 100.0f,
                            snapshot.loop ? "ON" : "OFF",
                            snapshot.playing ? "YES" : "NO");
        overlay_grid_set_cell(&grid, 7, 1, info, 0, "Y - Toggle Mode | X - Reset");

        // Rows 8-9 remain empty for future expansion

        overlay_grid_submit_to_overlay(&grid, s_overlay);

        SDL_Delay(16);
    }
    return 0;
}

void audio_overlay_start(BenchOverlay *overlay, BenchMetrics *metrics)
{
    s_overlay = overlay;
    s_metrics = metrics;
    SDL_AtomicSet(&s_overlay_running, 1);

    s_overlay_thread = SDL_CreateThread(overlay_thread_func, "audio_overlay", NULL);
    if (!s_overlay_thread) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Failed to create overlay update thread: %s",
                     SDL_GetError());
        SDL_AtomicSet(&s_overlay_running, 0);
    }
}

void audio_overlay_stop(void)
{
    if (s_overlay_thread) {
        SDL_AtomicSet(&s_overlay_running, 0);
        SDL_WaitThread(s_overlay_thread, NULL);
        s_overlay_thread = NULL;
    }
    s_overlay = NULL;
    s_metrics = NULL;
}
