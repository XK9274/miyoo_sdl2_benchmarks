#ifndef COMMON_ROLLING_CHART_H
#define COMMON_ROLLING_CHART_H

#include <SDL2/SDL.h>

/* Wall-clock rolling window (seconds) for the overlay's FPS/frame-time
 * sparklines. Read once and cached; default 1.5s. */
#define BENCH_ENV_OVERLAY_CHART_WINDOW_S "BENCH_OVERLAY_CHART_WINDOW_S"

#define ROLLING_CHART_MAX_SAMPLES 256

typedef struct {
    Uint64 counter_ticks;
    float value;
} RollingChartSample;

typedef struct {
    RollingChartSample samples[ROLLING_CHART_MAX_SAMPLES];
    int next_index;
    int count;
} RollingChart;

void rolling_chart_init(RollingChart *chart);
void rolling_chart_push(RollingChart *chart, float value);

/* Reads BENCH_ENV_OVERLAY_CHART_WINDOW_S on first call, cached afterward. */
double rolling_chart_window_seconds(void);

/* Draws a single-color polyline of the samples within the last
 * rolling_chart_window_seconds() onto surface, scaled to fit area, plus a
 * faint centered guideline. Direct pixel writes -- surface must be
 * SDL_PIXELFORMAT_RGBA32 and already locked if SDL_MUSTLOCK requires it. */
void rolling_chart_render(const RollingChart *chart, SDL_Surface *surface,
                          SDL_Rect area, SDL_Color color);

#endif /* COMMON_ROLLING_CHART_H */
