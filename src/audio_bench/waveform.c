#include "audio_bench/waveform.h"

#include <math.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_atomic.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define WAVEFORM_CAPACITY 4096
#define WAVEFORM_DRAW_MAX_POINTS 100
#define WAVEFORM_PUSH_BATCH 32

static float waveform_buffer[WAVEFORM_CAPACITY];
static SDL_atomic_t waveform_head;
static SDL_atomic_t waveform_size;

static float low_freq_buffer[WAVEFORM_CAPACITY];
static float mid_freq_buffer[WAVEFORM_CAPACITY];
static float high_freq_buffer[WAVEFORM_CAPACITY];

static float low_filter_prev = 0.0f;
static float mid_filter_prev = 0.0f;
static float high_filter_prev = 0.0f;

typedef struct WavePoint {
    float avg;
    float span;
    float energy;
    float progress;
    float center_x;
    int rect_x;
    int rect_w;
} WavePoint;

static WavePoint wave_points[WAVEFORM_DRAW_MAX_POINTS];
static float waveform_smoothed_avg[WAVEFORM_DRAW_MAX_POINTS];
static float waveform_smoothed_span[WAVEFORM_DRAW_MAX_POINTS];
static float waveform_smoothed_energy[WAVEFORM_DRAW_MAX_POINTS];
static int waveform_smoothed_count = 0;

static float peak_heights[WAVEFORM_DRAW_MAX_POINTS];
static Uint32 peak_last_update[WAVEFORM_DRAW_MAX_POINTS];
static int peaks_initialized = 0;

typedef enum {
    VIZ_MODE_BARS = 0,
    VIZ_MODE_CURVES,
    VIZ_MODE_DOTS,
    VIZ_MODE_RIBBONS,
    VIZ_MODE_COUNT
} viz_mode_t;

static viz_mode_t current_viz_mode = VIZ_MODE_BARS;
static viz_mode_t previous_viz_mode = VIZ_MODE_BARS;

static float left_level = 0.0f;
static float right_level = 0.0f;
static float left_peak = 0.0f;
static float right_peak = 0.0f;
static float audio_energy = 0.0f;
static float audio_pulse = 0.0f;

static const float WAVEFORM_METER_GAIN = 0.85f; // Halves previous 1.7 multiplier so the meters stop pegging

static float waveform_color_phase = 0.0f;
static Uint32 last_color_tick = 0;

static void waveform_push_batch(const float *samples, int count);
static int waveform_copy_frequency_band(float *out, int max_samples, int band);
static float waveform_clamp(float value, float min_val, float max_val);
static float waveform_wrap_unit(float value);
static SDL_Color waveform_sample_gradient(float t);
static void waveform_reset_peaks(void);
static void waveform_render_bars(SDL_Renderer *target,
                                 BenchMetrics *metrics,
                                 int y,
                                 int h,
                                 const WavePoint *points,
                                 int count,
                                 float amplitude_px,
                                 float color_phase);
static void waveform_render_curves(SDL_Renderer *target,
                                   BenchMetrics *metrics,
                                   int y,
                                   int h,
                                   const WavePoint *points,
                                   int count,
                                   float amplitude_px,
                                   float color_phase);
static void waveform_render_dots(SDL_Renderer *target,
                                 BenchMetrics *metrics,
                                 int y,
                                 int h,
                                 const WavePoint *points,
                                 int count,
                                 float amplitude_px,
                                 float color_phase);
static void waveform_render_ribbons(SDL_Renderer *target,
                                    BenchMetrics *metrics,
                                    int y,
                                    int h,
                                    const WavePoint *points,
                                    int count,
                                    float amplitude_px,
                                    float color_phase);
static float waveform_sample_from_bytes(const Uint8 *src,
                                        SDL_AudioFormat format,
                                        SDL_bool is_signed);
static void waveform_draw_meter_box(SDL_Renderer *target,
                                    BenchMetrics *metrics,
                                    int x,
                                    int y,
                                    int size,
                                    float level,
                                    float peak,
                                    SDL_bool is_left);
static void waveform_render_preview(SDL_Renderer *target,
                                    BenchMetrics *metrics,
                                    int x,
                                    int y,
                                    int w,
                                    int h,
                                    viz_mode_t mode);

static void waveform_push_batch(const float *samples, int count)
{
    if (!samples || count <= 0) {
        return;
    }

    int head = SDL_AtomicGet(&waveform_head);
    int size = SDL_AtomicGet(&waveform_size);

    const float low_alpha = 0.15f;
    const float mid_alpha = 0.60f;
    const float high_alpha = 0.90f;

    for (int i = 0; i < count; ++i) {
        const float sample = samples[i];

        low_filter_prev = low_filter_prev * (1.0f - low_alpha) + sample * low_alpha;
        const float low_freq = low_filter_prev;

        mid_filter_prev = mid_filter_prev * (1.0f - mid_alpha) + sample * mid_alpha;
        const float mid_freq = mid_filter_prev - low_filter_prev;

        high_filter_prev = high_filter_prev * (1.0f - high_alpha) + sample * high_alpha;
        const float high_freq = sample - high_filter_prev;

        waveform_buffer[head] = sample;
        low_freq_buffer[head] = low_freq * 2.0f;
        mid_freq_buffer[head] = mid_freq * 1.5f;
        high_freq_buffer[head] = high_freq * 3.0f;

        head++;
        if (head == WAVEFORM_CAPACITY) {
            head = 0;
        }
        if (size < WAVEFORM_CAPACITY) {
            size++;
        }
    }

    SDL_MemoryBarrierRelease();
    SDL_AtomicSet(&waveform_head, head);
    if (size > WAVEFORM_CAPACITY) {
        size = WAVEFORM_CAPACITY;
    }
    SDL_AtomicSet(&waveform_size, size);
}

static int waveform_copy_frequency_band(float *out, int max_samples, int band)
{
    if (!out || max_samples <= 0) {
        return 0;
    }

    const int head = SDL_AtomicGet(&waveform_head);
    int available = SDL_AtomicGet(&waveform_size);
    SDL_MemoryBarrierAcquire();

    if (available > WAVEFORM_CAPACITY) {
        available = WAVEFORM_CAPACITY;
    }

    const int count = SDL_min(available, max_samples);
    int start = head - count;
    while (start < 0) {
        start += WAVEFORM_CAPACITY;
    }

    float *source_buffer;
    switch (band) {
        case 0: source_buffer = low_freq_buffer; break;
        case 1: source_buffer = mid_freq_buffer; break;
        case 2: source_buffer = high_freq_buffer; break;
        default: source_buffer = waveform_buffer; break;
    }

    for (int i = 0; i < count; ++i) {
        const int idx = (start + i) % WAVEFORM_CAPACITY;
        out[i] = source_buffer[idx];
    }

    return count;
}

static float waveform_clamp(float value, float min_val, float max_val)
{
    if (value < min_val) {
        return min_val;
    }
    if (value > max_val) {
        return max_val;
    }
    return value;
}

static float waveform_wrap_unit(float value)
{
    value -= floorf(value);
    if (value < 0.0f) {
        value += 1.0f;
    }
    return value;
}

static SDL_Color waveform_sample_gradient(float t)
{
    static const SDL_Color gradient[] = {
        {255, 80, 80, 255},
        {255, 180, 64, 255},
        {80, 220, 255, 255},
        {160, 90, 255, 255},
        {255, 90, 200, 255}
    };

    const int stop_count = (int)(SDL_arraysize(gradient) - 1);
    t = waveform_clamp(t, 0.0f, 1.0f);
    const float scaled = t * (float)stop_count;
    int index = (int)scaled;
    if (index >= stop_count) {
        index = stop_count - 1;
    }
    const float weight = scaled - (float)index;

    const SDL_Color c0 = gradient[index];
    const SDL_Color c1 = gradient[index + 1];
    SDL_Color result;
    result.r = (Uint8)SDL_clamp((int)((float)c0.r + (float)(c1.r - c0.r) * weight), 0, 255);
    result.g = (Uint8)SDL_clamp((int)((float)c0.g + (float)(c1.g - c0.g) * weight), 0, 255);
    result.b = (Uint8)SDL_clamp((int)((float)c0.b + (float)(c1.b - c0.b) * weight), 0, 255);
    result.a = 255;
    return result;
}

static void waveform_reset_peaks(void)
{
    SDL_memset(peak_heights, 0, sizeof(peak_heights));
    SDL_memset(peak_last_update, 0, sizeof(peak_last_update));
    peaks_initialized = 1;
}

void waveform_reset(void)
{
    SDL_memset(waveform_buffer, 0, sizeof(waveform_buffer));
    SDL_memset(low_freq_buffer, 0, sizeof(low_freq_buffer));
    SDL_memset(mid_freq_buffer, 0, sizeof(mid_freq_buffer));
    SDL_memset(high_freq_buffer, 0, sizeof(high_freq_buffer));

    low_filter_prev = 0.0f;
    mid_filter_prev = 0.0f;
    high_filter_prev = 0.0f;

    SDL_AtomicSet(&waveform_head, 0);
    SDL_AtomicSet(&waveform_size, 0);
    SDL_MemoryBarrierRelease();

    SDL_memset(waveform_smoothed_avg, 0, sizeof(waveform_smoothed_avg));
    SDL_memset(waveform_smoothed_span, 0, sizeof(waveform_smoothed_span));
    SDL_memset(waveform_smoothed_energy, 0, sizeof(waveform_smoothed_energy));
    waveform_smoothed_count = 0;

    waveform_color_phase = 0.0f;
    last_color_tick = 0;

    left_level = right_level = 0.0f;
    left_peak = right_peak = 0.0f;
    audio_energy = audio_pulse = 0.0f;

    peaks_initialized = 0;
}

static float waveform_sample_from_bytes(const Uint8 *src,
                                        SDL_AudioFormat format,
                                        SDL_bool is_signed)
{
    if (!src) {
        return 0.0f;
    }

    const int bits = SDL_AUDIO_BITSIZE(format);
    if (SDL_AUDIO_ISFLOAT(format) && bits == 32) {
        float sample;
        SDL_memcpy(&sample, src, sizeof(sample));
        return sample;
    }

    if (bits == 8) {
        if (is_signed) {
            const Sint8 sample = *(const Sint8 *)src;
            return (float)sample / 127.0f;
        }
        const Uint8 sample = *src;
        return ((float)sample / 255.0f) * 2.0f - 1.0f;
    }

    if (bits == 16) {
        Sint16 sample = *(const Sint16 *)src;
        if ((SDL_AUDIO_ISBIGENDIAN(format) && SDL_BYTEORDER == SDL_LIL_ENDIAN) ||
            (SDL_AUDIO_ISLITTLEENDIAN(format) && SDL_BYTEORDER == SDL_BIG_ENDIAN)) {
            sample = (Sint16)SDL_Swap16((Uint16)sample);
        }
        return (float)sample / 32768.0f;
    }

    if (bits == 32 && !SDL_AUDIO_ISFLOAT(format)) {
        Sint32 sample = *(const Sint32 *)src;
        if ((SDL_AUDIO_ISBIGENDIAN(format) && SDL_BYTEORDER == SDL_LIL_ENDIAN) ||
            (SDL_AUDIO_ISLITTLEENDIAN(format) && SDL_BYTEORDER == SDL_BIG_ENDIAN)) {
            sample = (Sint32)SDL_Swap32((Uint32)sample);
        }
        return (float)sample / 2147483648.0f;
    }

    return 0.0f;
}

void waveform_capture_stream(const Uint8 *stream,
                             Uint32 len,
                             Uint32 bytes_per_frame,
                             SDL_AudioFormat format,
                             int channels)
{
    if (!stream || len == 0 || bytes_per_frame == 0) {
        return;
    }

    const Uint32 frame_count = len / bytes_per_frame;
    if (frame_count == 0) {
        return;
    }

    const int channel_count = SDL_max(1, channels);
    const int bytes_per_sample = bytes_per_frame / channel_count;
    const SDL_bool is_signed = SDL_AUDIO_ISSIGNED(format) ? SDL_TRUE : SDL_FALSE;
    const float inv_channels = 1.0f / (float)channel_count;

    float batch[WAVEFORM_PUSH_BATCH];
    int batch_count = 0;

    float local_left_peak = 0.0f;
    float local_right_peak = 0.0f;

    for (Uint32 frame = 0; frame < frame_count; ++frame) {
        const Uint8 *frame_ptr = stream + frame * bytes_per_frame;
        float mixed = 0.0f;

        for (int ch = 0; ch < channel_count; ++ch) {
            const Uint8 *sample_ptr = frame_ptr + ch * bytes_per_sample;
            const float sample = waveform_sample_from_bytes(sample_ptr, format, is_signed);
            mixed += sample;

            const float abs_sample = fabsf(sample);
            if (ch == 0) {
                if (abs_sample > local_left_peak) {
                    local_left_peak = abs_sample;
                }
            } else if (ch == 1) {
                if (abs_sample > local_right_peak) {
                    local_right_peak = abs_sample;
                }
            }
        }

        mixed *= inv_channels;
        mixed = waveform_clamp(mixed, -1.2f, 1.2f);

        batch[batch_count++] = mixed;
        if (batch_count == WAVEFORM_PUSH_BATCH) {
            waveform_push_batch(batch, batch_count);
            batch_count = 0;
        }
    }

    if (batch_count > 0) {
        waveform_push_batch(batch, batch_count);
    }

    const float combined_peak = SDL_max(local_left_peak, local_right_peak);
    const float attack = 0.55f;
    const float release = 0.93f;

    if (local_left_peak > left_level) {
        left_level += (local_left_peak - left_level) * attack;
    } else {
        left_level *= release;
    }
    if (local_right_peak > right_level) {
        right_level += (local_right_peak - right_level) * attack;
    } else {
        right_level *= release;
    }

    left_level = waveform_clamp(left_level, 0.0f, 1.0f);
    right_level = waveform_clamp(right_level, 0.0f, 1.0f);

    left_peak = SDL_max(local_left_peak, left_peak * 0.96f);
    right_peak = SDL_max(local_right_peak, right_peak * 0.96f);

    audio_energy = audio_energy * 0.85f + combined_peak * 0.15f;
    audio_pulse = SDL_max(combined_peak, audio_pulse * 0.90f);

    audio_energy = waveform_clamp(audio_energy, 0.0f, 1.0f);
    audio_pulse = waveform_clamp(audio_pulse, 0.0f, 1.0f);
}

void waveform_draw(SDL_Renderer *target,
                   BenchMetrics *metrics,
                   int x,
                   int y,
                   int w,
                   int h)
{
    if (!target || w <= 4 || h <= 4) {
        return;
    }

    const int max_points = SDL_min(WAVEFORM_DRAW_MAX_POINTS, SDL_max(16, w / 2));

    float bass_samples[WAVEFORM_DRAW_MAX_POINTS];
    float mid_samples[WAVEFORM_DRAW_MAX_POINTS];
    float treble_samples[WAVEFORM_DRAW_MAX_POINTS];

    const int bass_count = waveform_copy_frequency_band(bass_samples, max_points, 0);
    const int mid_count = waveform_copy_frequency_band(mid_samples, max_points, 1);
    const int treble_count = waveform_copy_frequency_band(treble_samples, max_points, 2);

    const int count = SDL_max(bass_count, SDL_max(mid_count, treble_count));
    if (count < 2) {
        return;
    }

    SDL_Rect backdrop = {x, y, w, h};
    SDL_SetRenderDrawBlendMode(target, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(target, 10, 14, 26, 245);
    SDL_RenderFillRect(target, &backdrop);
    if (metrics) {
        metrics->draw_calls++;
    }

    SDL_SetRenderDrawColor(target, 36, 48, 78, 255);
    SDL_RenderDrawRect(target, &backdrop);
    if (metrics) {
        metrics->draw_calls++;
        metrics->vertices_rendered += 8;
    }

    SDL_Rect clip_rect = {x + 1, y + 1, w - 2, h - 2};
    SDL_RenderSetClipRect(target, &clip_rect);

    const int points_to_draw = SDL_min(max_points, count);
    const float samples_per_point = (float)count / (float)points_to_draw;
    const float amplitude_px = (float)(h - 2) * 0.8f;

    if (previous_viz_mode != current_viz_mode) {
        peaks_initialized = 0;
        previous_viz_mode = current_viz_mode;
    }

    if (waveform_smoothed_count != points_to_draw) {
        SDL_memset(waveform_smoothed_avg, 0, sizeof(waveform_smoothed_avg));
        SDL_memset(waveform_smoothed_span, 0, sizeof(waveform_smoothed_span));
        SDL_memset(waveform_smoothed_energy, 0, sizeof(waveform_smoothed_energy));
        waveform_smoothed_count = points_to_draw;
        peaks_initialized = 0;
    }

    if (!peaks_initialized && current_viz_mode == VIZ_MODE_BARS) {
        waveform_reset_peaks();
    }

    Uint32 now = SDL_GetTicks();
    if (last_color_tick == 0) {
        last_color_tick = now;
    }
    const Uint32 elapsed = now - last_color_tick;
    if (elapsed > 0) {
        const float delta = SDL_min((float)elapsed / 1000.0f, 0.1f);
        waveform_color_phase = waveform_wrap_unit(waveform_color_phase + delta * (0.1f + audio_energy * 0.6f));
        last_color_tick = now;
    }

    for (int point = 0; point < points_to_draw; ++point) {
        const float sample_start = point * samples_per_point;
        const float sample_end = sample_start + samples_per_point;
        int idx_start = (int)sample_start;
        int idx_end = (int)SDL_min((float)count, ceilf(sample_end));
        if (idx_end <= idx_start) {
            idx_end = idx_start + 1;
        }

        float min_v = 1.0f;
        float max_v = -1.0f;
        float accum = 0.0f;
        float energy = 0.0f;
        int accum_count = 0;

        for (int s = idx_start; s < idx_end; ++s) {
            const float blend = waveform_clamp((float)s / (float)count, 0.0f, 1.0f);
            float sample = bass_samples[s] * (1.0f - blend) +
                           mid_samples[s] * 0.6f +
                           treble_samples[s] * blend * 0.7f;
            sample = waveform_clamp(sample * 1.3f, -1.0f, 1.0f);

            min_v = SDL_min(min_v, sample);
            max_v = SDL_max(max_v, sample);
            accum += sample;
            const float abs_sample = fabsf(sample);
            if (abs_sample > energy) {
                energy = abs_sample;
            }
            ++accum_count;
        }

        float avg_v = (accum_count > 0) ? (accum / (float)accum_count) : bass_samples[idx_start];
        avg_v = waveform_clamp(avg_v, -1.0f, 1.0f);

        const float target_span = SDL_max(max_v - min_v, 0.02f);
        const float smooth = 0.25f;
        float prev_avg = waveform_smoothed_avg[point];
        float prev_span = waveform_smoothed_span[point];
        float prev_energy = waveform_smoothed_energy[point];

        if (prev_span == 0.0f && prev_avg == 0.0f && prev_energy == 0.0f) {
            prev_avg = avg_v;
            prev_span = target_span;
            prev_energy = energy;
        } else {
            prev_avg += (avg_v - prev_avg) * smooth;
            prev_span += (target_span - prev_span) * smooth;
            prev_energy += (energy - prev_energy) * 0.30f;
        }

        waveform_smoothed_avg[point] = prev_avg;
        waveform_smoothed_span[point] = prev_span;
        waveform_smoothed_energy[point] = prev_energy;

        const float denom = (points_to_draw > 1) ? (float)(points_to_draw - 1) : 1.0f;
        const float px0 = (float)x + ((float)point / denom) * (float)w;
        const float px1 = (float)x + ((float)(point + 1) / denom) * (float)w;

        int rect_x = (int)floorf(px0);
        int rect_w = (int)ceilf(px1) - rect_x;
        if (rect_w <= 0) {
            rect_w = 1;
        } else if (rect_w > 5) {
            rect_w = 5;
        }

        WavePoint *wp = &wave_points[point];
        wp->avg = prev_avg;
        wp->span = prev_span;
        wp->energy = waveform_clamp(prev_energy, 0.0f, 1.0f);
        wp->progress = (float)point / SDL_max(points_to_draw - 1, 1);
        wp->center_x = (px0 + px1) * 0.5f;
        wp->rect_x = rect_x;
        wp->rect_w = rect_w;
    }

    switch (current_viz_mode) {
        case VIZ_MODE_BARS:
            waveform_render_bars(target, metrics, y, h, wave_points, points_to_draw, amplitude_px, waveform_color_phase);
            break;
        case VIZ_MODE_CURVES:
            waveform_render_curves(target, metrics, y, h, wave_points, points_to_draw, amplitude_px, waveform_color_phase);
            break;
        case VIZ_MODE_DOTS:
            waveform_render_dots(target, metrics, y, h, wave_points, points_to_draw, amplitude_px, waveform_color_phase);
            break;
        case VIZ_MODE_RIBBONS:
            waveform_render_ribbons(target, metrics, y, h, wave_points, points_to_draw, amplitude_px, waveform_color_phase);
            break;
        default:
            waveform_render_dots(target, metrics, y, h, wave_points, points_to_draw, amplitude_px, waveform_color_phase);
            break;
    }

    SDL_RenderSetClipRect(target, NULL);
}

static void waveform_render_bars(SDL_Renderer *target,
                                 BenchMetrics *metrics,
                                 int y,
                                 int h,
                                 const WavePoint *points,
                                 int count,
                                 float amplitude_px,
                                 float color_phase)
{
    if (!peaks_initialized) {
        waveform_reset_peaks();
    }

    const int bottom_y = y + h - 2;
    for (int i = 0; i < count; ++i) {
        const WavePoint *wp = &points[i];
        const float magnitude = fabsf(wp->avg);
        int bar_height = (int)(magnitude * amplitude_px);
        if (bar_height < 1) {
            bar_height = 1;
        }
        if (bar_height > h - 2) {
            bar_height = h - 2;
        }

        const int rect_h = bar_height;
        const int rect_y = bottom_y - rect_h;

        float peak = peak_heights[i];
        if (bar_height > (int)peak) {
            peak = (float)bar_height;
        } else {
            peak *= 0.94f;
        }
        peak_heights[i] = peak;

        const float gradient_pos = waveform_wrap_unit(wp->progress + color_phase);
        SDL_Color base = waveform_sample_gradient(gradient_pos);
        const float brightness = waveform_clamp(0.35f + wp->energy * 0.55f + audio_pulse * 0.35f, 0.1f, 1.3f);
        const Uint8 r = (Uint8)SDL_clamp((int)(base.r * brightness), 0, 255);
        const Uint8 g = (Uint8)SDL_clamp((int)(base.g * brightness), 0, 255);
        const Uint8 b = (Uint8)SDL_clamp((int)(base.b * brightness), 0, 255);

        SDL_Rect bar_rect = {wp->rect_x, rect_y, wp->rect_w, rect_h};
        SDL_SetRenderDrawColor(target, r, g, b, 230);
        SDL_RenderFillRect(target, &bar_rect);

        if (metrics) {
            metrics->draw_calls++;
            metrics->vertices_rendered += 6;
        }

        const int peak_y = bottom_y - (int)peak;
        SDL_Rect peak_rect = {wp->rect_x, peak_y, wp->rect_w, 2};
        SDL_SetRenderDrawColor(target,
                               SDL_min(255, r + 40),
                               SDL_min(255, g + 40),
                               SDL_min(255, b + 40),
                               255);
        SDL_RenderFillRect(target, &peak_rect);
        if (metrics) {
            metrics->draw_calls++;
            metrics->vertices_rendered += 4;
        }
    }
}

static void waveform_render_curves(SDL_Renderer *target,
                                   BenchMetrics *metrics,
                                   int y,
                                   int h,
                                   const WavePoint *points,
                                   int count,
                                   float amplitude_px,
                                   float color_phase)
{
    const int mid_y = y + h / 2;
    SDL_SetRenderDrawBlendMode(target, SDL_BLENDMODE_ADD);

    for (int i = 1; i < count; ++i) {
        const WavePoint *prev = &points[i - 1];
        const WavePoint *curr = &points[i];

        const int x0 = (int)prev->center_x;
        const int x1 = (int)curr->center_x;
        const int y0 = mid_y - (int)(prev->avg * amplitude_px * 0.85f);
        const int y1 = mid_y - (int)(curr->avg * amplitude_px * 0.85f);

        const float segment_progress = waveform_wrap_unit(((prev->progress + curr->progress) * 0.5f) + color_phase * 0.6f);
        SDL_Color base = waveform_sample_gradient(segment_progress);
        const float energy = waveform_clamp((prev->energy + curr->energy) * 0.5f, 0.0f, 1.0f);
        const float brightness = waveform_clamp(0.30f + energy * 0.65f + audio_pulse * 0.35f, 0.2f, 1.3f);
        const Uint8 r = (Uint8)SDL_clamp((int)(base.r * brightness), 0, 255);
        const Uint8 g = (Uint8)SDL_clamp((int)(base.g * brightness), 0, 255);
        const Uint8 b = (Uint8)SDL_clamp((int)(base.b * brightness), 0, 255);

        SDL_SetRenderDrawColor(target, r, g, b, 210);
        SDL_RenderDrawLine(target, x0, y0, x1, y1);
        SDL_RenderDrawLine(target, x0, mid_y + (mid_y - y0), x1, mid_y + (mid_y - y1));

        if (metrics) {
            metrics->draw_calls += 2;
            metrics->vertices_rendered += 4;
        }
    }

    SDL_SetRenderDrawBlendMode(target, SDL_BLENDMODE_BLEND);
}

static void waveform_render_dots(SDL_Renderer *target,
                                 BenchMetrics *metrics,
                                 int y,
                                 int h,
                                 const WavePoint *points,
                                 int count,
                                 float amplitude_px,
                                 float color_phase)
{
    const int mid_y = y + h / 2;
    SDL_SetRenderDrawBlendMode(target, SDL_BLENDMODE_BLEND);

    for (int i = 0; i < count; ++i) {
        const WavePoint *wp = &points[i];
        const float brightness = waveform_clamp(0.35f + wp->energy * 0.65f + audio_pulse * 0.45f, 0.2f, 1.4f);
        const float gradient_pos = waveform_wrap_unit(wp->progress * 0.85f + color_phase * 0.9f);
        SDL_Color base = waveform_sample_gradient(gradient_pos);
        const Uint8 r = (Uint8)SDL_clamp((int)(base.r * brightness), 0, 255);
        const Uint8 g = (Uint8)SDL_clamp((int)(base.g * brightness), 0, 255);
        const Uint8 b = (Uint8)SDL_clamp((int)(base.b * brightness), 0, 255);

        const float offset = wp->avg * amplitude_px * 0.6f;
        const int center_y = mid_y - (int)offset;
        const int mirror_y = mid_y + (int)offset;
        const int size = SDL_max(2, (int)(4.0f + wp->energy * 8.0f));
        const int half = size / 2;

        SDL_Rect dot = {(int)wp->center_x - half, center_y - half, size, size};
        SDL_SetRenderDrawColor(target, r, g, b, 220);
        SDL_RenderFillRect(target, &dot);

        dot.y = mirror_y - half;
        SDL_SetRenderDrawColor(target,
                               SDL_min(255, r + 30),
                               SDL_min(255, g + 30),
                               SDL_min(255, b + 30),
                               220);
        SDL_RenderFillRect(target, &dot);

        if (metrics) {
            metrics->draw_calls += 2;
            metrics->vertices_rendered += 8;
        }
    }
}

static void waveform_render_ribbons(SDL_Renderer *target,
                                    BenchMetrics *metrics,
                                    int y,
                                    int h,
                                    const WavePoint *points,
                                    int count,
                                    float amplitude_px,
                                    float color_phase)
{
    if (count < 2) {
        return;
    }

    WavePoint simplified[WAVEFORM_DRAW_MAX_POINTS];
    int simplified_count = 0;

    const float min_dx = 1.35f;
    const float min_avg = 0.02f;
    const float min_energy = 0.03f;

    for (int i = 0; i < count; ++i) {
        const WavePoint *wp = &points[i];
        if (simplified_count == 0) {
            simplified[simplified_count++] = *wp;
            continue;
        }

        const WavePoint *prev = &simplified[simplified_count - 1];
        const float dx = fabsf(wp->center_x - prev->center_x);
        const float davg = fabsf(wp->avg - prev->avg);
        const float denergy = fabsf(wp->energy - prev->energy);

        if (dx < min_dx && davg < min_avg && denergy < min_energy) {
            continue;
        }

        simplified[simplified_count++] = *wp;
    }

    if (simplified_count < 2) {
        simplified[simplified_count++] = points[count - 1];
    } else if (simplified[simplified_count - 1].center_x != points[count - 1].center_x) {
        simplified[simplified_count++] = points[count - 1];
    }

    WavePoint decimated[WAVEFORM_DRAW_MAX_POINTS];
    const int target_segments = 22;
    const int mid_y = y + h / 2;
    const float base_amp = amplitude_px * 0.66f;

    const WavePoint *render_points = simplified;
    int render_count = simplified_count;

    if (render_count > target_segments) {
        int step = (render_count - 1) / target_segments + 1;
        int out = 0;
        for (int i = 0; i < render_count; i += step) {
            decimated[out++] = simplified[i];
        }
        if (decimated[out - 1].center_x != simplified[render_count - 1].center_x) {
            decimated[out++] = simplified[render_count - 1];
        }
        render_points = decimated;
        render_count = SDL_max(out, 2);
    }

    SDL_SetRenderDrawBlendMode(target, SDL_BLENDMODE_ADD);

    SDL_Vertex verts[(WAVEFORM_DRAW_MAX_POINTS - 1) * 6];
    int v = 0;

    for (int i = 0; i < render_count - 1; ++i) {
        const WavePoint *a = &render_points[i];
        const WavePoint *b = &render_points[i + 1];

        const float avg_progress = waveform_wrap_unit(((a->progress + b->progress) * 0.5f) + color_phase * 0.65f);
        SDL_Color base = waveform_sample_gradient(avg_progress);
        const float combined_energy = waveform_clamp((a->energy + b->energy) * 0.5f + audio_pulse * 0.25f, 0.0f, 1.15f);
        const float brightness = waveform_clamp(0.28f + combined_energy * 0.6f + audio_energy * 0.25f, 0.22f, 1.3f);
        const Uint8 r = (Uint8)SDL_clamp((int)(base.r * brightness), 0, 255);
        const Uint8 g = (Uint8)SDL_clamp((int)(base.g * brightness), 0, 255);
        const Uint8 bcol = (Uint8)SDL_clamp((int)(base.b * brightness), 0, 255);
        const Uint8 a_col = (Uint8)SDL_clamp((int)(165 + combined_energy * 55.0f), 130, 235);

        const float top_a = (float)mid_y - a->avg * base_amp;
        const float top_b = (float)mid_y - b->avg * base_amp;
        const float bottom_a = (float)mid_y + a->avg * base_amp * 0.32f;
        const float bottom_b = (float)mid_y + b->avg * base_amp * 0.32f;

        SDL_Color fill = (SDL_Color){r, g, bcol, a_col};

        verts[v++] = (SDL_Vertex){.position = {a->center_x, top_a}, .color = fill, .tex_coord = {0.0f, 0.0f}};
        verts[v++] = (SDL_Vertex){.position = {a->center_x, bottom_a}, .color = fill, .tex_coord = {0.0f, 0.0f}};
        verts[v++] = (SDL_Vertex){.position = {b->center_x, top_b}, .color = fill, .tex_coord = {0.0f, 0.0f}};
        verts[v++] = (SDL_Vertex){.position = {a->center_x, bottom_a}, .color = fill, .tex_coord = {0.0f, 0.0f}};
        verts[v++] = (SDL_Vertex){.position = {b->center_x, bottom_b}, .color = fill, .tex_coord = {0.0f, 0.0f}};
        verts[v++] = (SDL_Vertex){.position = {b->center_x, top_b}, .color = fill, .tex_coord = {0.0f, 0.0f}};
    }

    SDL_RenderGeometry(target, NULL, verts, v, NULL, 0);

    if (metrics) {
        metrics->draw_calls++;
        metrics->vertices_rendered += v;
        metrics->triangles_rendered += v / 3;
    }

    SDL_SetRenderDrawBlendMode(target, SDL_BLENDMODE_BLEND);
}

static void waveform_render_preview(SDL_Renderer *target,
                                    BenchMetrics *metrics,
                                    int x,
                                    int y,
                                    int w,
                                    int h,
                                    viz_mode_t mode)
{
    if (w <= 4 || h <= 4) {
        return;
    }

    const int max_points = SDL_min(WAVEFORM_DRAW_MAX_POINTS, SDL_max(12, w / 3));

    float bass_samples[WAVEFORM_DRAW_MAX_POINTS];
    float mid_samples[WAVEFORM_DRAW_MAX_POINTS];
    float treble_samples[WAVEFORM_DRAW_MAX_POINTS];

    const int bass_count = waveform_copy_frequency_band(bass_samples, max_points, 0);
    const int mid_count = waveform_copy_frequency_band(mid_samples, max_points, 1);
    const int treble_count = waveform_copy_frequency_band(treble_samples, max_points, 2);

    const int count = SDL_max(bass_count, SDL_max(mid_count, treble_count));
    if (count < 2) {
        return;
    }

    SDL_SetRenderDrawBlendMode(target, SDL_BLENDMODE_BLEND);

    SDL_Rect clip = {x, y, w, h};
    SDL_RenderSetClipRect(target, &clip);

    const int points_to_draw = SDL_min(max_points, count);
    const float samples_per_point = (float)count / (float)points_to_draw;
    const float amplitude_px = (float)(h - 2) * 0.7f;

    WavePoint preview_points[WAVEFORM_DRAW_MAX_POINTS];
    float prev_avg = 0.0f;
    float prev_span = 0.0f;
    float prev_energy = 0.0f;
    SDL_bool first = SDL_TRUE;

    for (int point = 0; point < points_to_draw; ++point) {
        const float sample_start = point * samples_per_point;
        const float sample_end = sample_start + samples_per_point;

        int idx_start = (int)sample_start;
        int idx_end = (int)SDL_min((float)count, ceilf(sample_end));
        if (idx_end <= idx_start) {
            idx_end = idx_start + 1;
        }

        float min_v = 1.0f;
        float max_v = -1.0f;
        float accum = 0.0f;
        float energy = 0.0f;
        int accum_count = 0;

        for (int s = idx_start; s < idx_end; ++s) {
            const float blend = waveform_clamp((float)s / (float)count, 0.0f, 1.0f);
            float sample = bass_samples[s] * (1.0f - blend) +
                           mid_samples[s] * 0.6f +
                           treble_samples[s] * blend * 0.7f;
            sample = waveform_clamp(sample * 1.2f, -1.0f, 1.0f);

            min_v = SDL_min(min_v, sample);
            max_v = SDL_max(max_v, sample);
            accum += sample;
            const float abs_sample = fabsf(sample);
            if (abs_sample > energy) {
                energy = abs_sample;
            }
            ++accum_count;
        }

        float avg_v = (accum_count > 0) ? (accum / (float)accum_count) : 0.0f;
        avg_v = waveform_clamp(avg_v, -1.0f, 1.0f);

        const float target_span = SDL_max(max_v - min_v, 0.02f);
        if (first) {
            prev_avg = avg_v;
            prev_span = target_span;
            prev_energy = energy;
            first = SDL_FALSE;
        } else {
            prev_avg += (avg_v - prev_avg) * 0.35f;
            prev_span += (target_span - prev_span) * 0.35f;
            prev_energy += (energy - prev_energy) * 0.45f;
        }

        const float denom = (points_to_draw > 1) ? (float)(points_to_draw - 1) : 1.0f;
        const float px0 = (float)x + ((float)point / denom) * (float)w;
        const float px1 = (float)x + ((float)(point + 1) / denom) * (float)w;

        int rect_x = (int)floorf(px0);
        int rect_w = (int)ceilf(px1) - rect_x;
        if (rect_w <= 0) {
            rect_w = 1;
        }

        WavePoint *wp = &preview_points[point];
        wp->avg = prev_avg;
        wp->span = prev_span;
        wp->energy = waveform_clamp(prev_energy, 0.0f, 1.0f);
        wp->progress = (float)point / SDL_max(points_to_draw - 1, 1);
        wp->center_x = (px0 + px1) * 0.5f;
        wp->rect_x = rect_x;
        wp->rect_w = rect_w;
    }

    switch (mode) {
        case VIZ_MODE_BARS:
            waveform_render_bars(target, metrics, y, h, preview_points, points_to_draw, amplitude_px, waveform_color_phase);
            break;
        case VIZ_MODE_CURVES:
            waveform_render_curves(target, metrics, y, h, preview_points, points_to_draw, amplitude_px, waveform_color_phase);
            break;
        case VIZ_MODE_DOTS:
            waveform_render_dots(target, metrics, y, h, preview_points, points_to_draw, amplitude_px, waveform_color_phase);
            break;
        case VIZ_MODE_RIBBONS:
            waveform_render_ribbons(target, metrics, y, h, preview_points, points_to_draw, amplitude_px, waveform_color_phase);
            break;
        default:
            waveform_render_dots(target, metrics, y, h, preview_points, points_to_draw, amplitude_px, waveform_color_phase);
            break;
    }

    SDL_RenderSetClipRect(target, NULL);
}

void waveform_toggle_mode(void)
{
    current_viz_mode = (current_viz_mode + 1) % VIZ_MODE_COUNT;
    peaks_initialized = 0;
}

const char *waveform_get_mode_name(void)
{
    switch (current_viz_mode) {
        case VIZ_MODE_BARS: return "Bars (rect)";
        case VIZ_MODE_CURVES: return "Curves (line)";
        case VIZ_MODE_DOTS: return "Dots (rect)";
        case VIZ_MODE_RIBBONS: return "Ribbons (geom)";
        default: return "Unknown";
    }
}


static void waveform_draw_meter_box(SDL_Renderer *target,
                                    BenchMetrics *metrics,
                                    int x,
                                    int y,
                                    int size,
                                    float level,
                                    float peak,
                                    SDL_bool is_left)
{
    SDL_Rect box = {x, y, size, size};
    SDL_SetRenderDrawColor(target, 12, 18, 32, 255);
    SDL_RenderFillRect(target, &box);
    SDL_SetRenderDrawColor(target, 70, 96, 140, 255);
    SDL_RenderDrawRect(target, &box);
    if (metrics) {
        metrics->draw_calls += 2;
        metrics->vertices_rendered += 12;
    }

    float scaled_level = waveform_clamp(level * WAVEFORM_METER_GAIN, 0.0f, 1.0f);
    scaled_level = sqrtf(scaled_level);
    int bar_height = (int)((float)(size - 6) * scaled_level);
    if (bar_height < 1) {
        bar_height = 1;
    }
    int bar_y = y + size - 3 - bar_height;

    const Uint8 base_r = is_left ? 40 : 255;
    const Uint8 base_g = is_left ? 220 : 140;
    const Uint8 base_b = is_left ? 150 : 60;

    SDL_Rect fill = {x + 3, bar_y, size - 6, bar_height};
    SDL_SetRenderDrawColor(target, base_r, base_g, base_b, 235);
    SDL_RenderFillRect(target, &fill);
    if (metrics) {
        metrics->draw_calls++;
        metrics->vertices_rendered += 6;
    }

    float scaled_peak = waveform_clamp(peak * WAVEFORM_METER_GAIN, 0.0f, 1.0f);
    scaled_peak = sqrtf(scaled_peak);
    const int peak_y = y + size - 3 - (int)((float)(size - 6) * scaled_peak);
    SDL_Rect peak_rect = {x + 3, peak_y - 1, size - 6, 2};
    SDL_SetRenderDrawColor(target,
                           SDL_min(255, base_r + 40),
                           SDL_min(255, base_g + 40),
                           SDL_min(255, base_b + 40),
                           255);
    SDL_RenderFillRect(target, &peak_rect);
    if (metrics) {
        metrics->draw_calls++;
        metrics->vertices_rendered += 4;
    }
}

void waveform_draw_ui_area(SDL_Renderer *target,
                          BenchMetrics *metrics,
                          int x,
                          int y,
                          int w,
                          int h)
{
    if (!target || w <= 4 || h <= 4) {
        return;
    }

    SDL_Rect ui_backdrop = {x, y, w, h};
    SDL_SetRenderDrawBlendMode(target, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(target, 18, 24, 40, 235);
    SDL_RenderFillRect(target, &ui_backdrop);
    SDL_SetRenderDrawColor(target, 64, 84, 128, 255);
    SDL_RenderDrawRect(target, &ui_backdrop);
    if (metrics) {
        metrics->draw_calls += 2;
        metrics->vertices_rendered += 12;
    }

    const int box_size = SDL_min(h - 8, 52);
    const int box_y = y + (h - box_size) / 2;

    waveform_draw_meter_box(target, metrics, x + 8, box_y, box_size, left_level, left_peak, SDL_TRUE);
    waveform_draw_meter_box(target, metrics, x + 16 + box_size, box_y, box_size, right_level, right_peak, SDL_FALSE);

    const int fill_x = x + 32 + box_size * 2;
    const int fill_w = w - (box_size * 2 + 40);
    const int fill_h = h - 12;
    const int fill_y = y + 6;

    if (fill_w > 24 && fill_h > 24) {
        SDL_Rect preview_backdrop = {fill_x, fill_y, fill_w, fill_h};
        SDL_SetRenderDrawColor(target, 8, 12, 20, 230);
        SDL_RenderFillRect(target, &preview_backdrop);
        SDL_SetRenderDrawColor(target, 50, 70, 110, 255);
        SDL_RenderDrawRect(target, &preview_backdrop);
        if (metrics) {
            metrics->draw_calls += 2;
            metrics->vertices_rendered += 12;
        }

        const viz_mode_t preview_mode = (current_viz_mode + 1) % VIZ_MODE_COUNT;
        waveform_render_preview(target,
                                metrics,
                                fill_x + 2,
                                fill_y + 2,
                                fill_w - 4,
                                fill_h - 4,
                                preview_mode);
    }
}
