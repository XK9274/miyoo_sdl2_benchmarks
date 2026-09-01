#include "common/rolling_chart.h"

#include <stdlib.h>

void rolling_chart_init(RollingChart *chart)
{
    if (!chart) {
        return;
    }
    SDL_zerop(chart);
}

void rolling_chart_push(RollingChart *chart, float value)
{
    if (!chart) {
        return;
    }
    chart->samples[chart->next_index].counter_ticks = SDL_GetPerformanceCounter();
    chart->samples[chart->next_index].value = value;
    chart->next_index = (chart->next_index + 1) % ROLLING_CHART_MAX_SAMPLES;
    if (chart->count < ROLLING_CHART_MAX_SAMPLES) {
        chart->count++;
    }
}

double rolling_chart_window_seconds(void)
{
    static double window_s = 0.0;
    static SDL_bool loaded = SDL_FALSE;
    if (!loaded) {
        const char *env = SDL_getenv(BENCH_ENV_OVERLAY_CHART_WINDOW_S);
        window_s = env ? SDL_atof(env) : 1.5;
        if (window_s <= 0.0) {
            window_s = 1.5;
        }
        loaded = SDL_TRUE;
    }
    return window_s;
}

static void rolling_chart_set_pixel(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
    if (x < 0 || y < 0 || x >= surface->w || y >= surface->h) {
        return;
    }
    Uint8 *row = (Uint8 *)surface->pixels + (size_t)y * surface->pitch;
    ((Uint32 *)row)[x] = pixel;
}

static void rolling_chart_draw_line(SDL_Surface *surface, int x0, int y0, int x1, int y1, Uint32 pixel)
{
    int dx = abs(x1 - x0);
    int sx = (x0 < x1) ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;

    for (;;) {
        rolling_chart_set_pixel(surface, x0, y0, pixel);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        const int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void rolling_chart_render(const RollingChart *chart, SDL_Surface *surface,
                          SDL_Rect area, SDL_Color color)
{
    if (!chart || !surface || area.w <= 1 || area.h <= 1) {
        return;
    }

    const Uint32 guide_pixel = SDL_MapRGBA(surface->format, 255, 255, 255, 46);
    const int guide_y = area.y + area.h / 2;
    rolling_chart_draw_line(surface, area.x, guide_y, area.x + area.w - 1, guide_y, guide_pixel);

    if (chart->count < 2) {
        return;
    }

    const Uint64 freq = SDL_GetPerformanceFrequency();
    const Uint64 window_ticks = (Uint64)(rolling_chart_window_seconds() * (double)freq);
    const Uint64 now = SDL_GetPerformanceCounter();
    const Uint64 cutoff = (now > window_ticks) ? (now - window_ticks) : 0;

    float min_v = 0.0f;
    float max_v = 0.0f;
    SDL_bool have_range = SDL_FALSE;
    const int oldest_index = (chart->next_index - chart->count + ROLLING_CHART_MAX_SAMPLES) % ROLLING_CHART_MAX_SAMPLES;
    for (int i = 0; i < chart->count; ++i) {
        const RollingChartSample *s = &chart->samples[(oldest_index + i) % ROLLING_CHART_MAX_SAMPLES];
        if (s->counter_ticks < cutoff) {
            continue;
        }
        if (!have_range) {
            min_v = max_v = s->value;
            have_range = SDL_TRUE;
        } else {
            if (s->value < min_v) min_v = s->value;
            if (s->value > max_v) max_v = s->value;
        }
    }
    if (!have_range) {
        return;
    }
    if (max_v - min_v < 0.0001f) {
        max_v = min_v + 1.0f;
    }

    const Uint32 line_pixel = SDL_MapRGBA(surface->format, color.r, color.g, color.b, 255);
    int prev_x = 0, prev_y = 0;
    SDL_bool has_prev = SDL_FALSE;

    for (int i = 0; i < chart->count; ++i) {
        const RollingChartSample *s = &chart->samples[(oldest_index + i) % ROLLING_CHART_MAX_SAMPLES];
        if (s->counter_ticks < cutoff) {
            continue;
        }
        const double age_s = (double)(now - s->counter_ticks) / (double)freq;
        const double window_s = rolling_chart_window_seconds();
        const double t = 1.0 - (age_s / window_s);
        const int x = area.x + (int)(t * (double)(area.w - 1));
        const float normalized = (s->value - min_v) / (max_v - min_v);
        const int y = area.y + area.h - 1 - (int)(normalized * (float)(area.h - 1));

        if (has_prev) {
            rolling_chart_draw_line(surface, prev_x, prev_y, x, y, line_pixel);
        }
        prev_x = x;
        prev_y = y;
        has_prev = SDL_TRUE;
    }
}
