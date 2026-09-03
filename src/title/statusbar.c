#define _POSIX_C_SOURCE 200809L

#include "title/statusbar.h"

#include <time.h>

#include "common/driver_support.h"
#include "title/battery_icon.h"
#include "title/config_panel.h"
#include "title/render_util.h"

/* Deliberate icon glyph size -- independent of any text metric. */
#define TITLE_BATTERY_ICON_HEIGHT 14

void title_draw_segmented_line(SDL_Renderer *renderer, TTF_Font *font,
                               const TitleStatusSegment *segments, int count,
                               int center_x, int y,
                               const char *separator, SDL_Color sep_color)
{
    if (!renderer || !font || !segments || count <= 0) {
        return;
    }

    int sep_w = 0, h = 0;
    TTF_SizeUTF8(font, separator, &sep_w, &h);

    int total_w = 0;
    int seg_w[16];
    int usable = (count > 16) ? 16 : count;
    for (int i = 0; i < usable; i++) {
        TTF_SizeUTF8(font, segments[i].text, &seg_w[i], &h);
        total_w += seg_w[i];
        if (i + 1 < usable) {
            total_w += sep_w;
        }
    }

    int x = center_x - total_w / 2;
    for (int i = 0; i < usable; i++) {
        title_draw_text(renderer, font, segments[i].text, x, y, segments[i].color, SDL_FALSE);
        x += seg_w[i];
        if (i + 1 < usable) {
            title_draw_text(renderer, font, separator, x, y, sep_color, SDL_FALSE);
            x += sep_w;
        }
    }
}

void title_draw_led(SDL_Renderer *renderer, int x, int y, int size, SDL_bool ok)
{
    const SDL_Color outline = {120, 124, 132, 255};
    const SDL_Color center_on = {70, 195, 95, 255};
    const SDL_Color center_off = {215, 70, 65, 255};
    const SDL_Color center_color = ok ? center_on : center_off;

    SDL_Rect outer = {x, y, size, size};
    SDL_SetRenderDrawColor(renderer, outline.r, outline.g, outline.b, outline.a);
    SDL_RenderFillRect(renderer, &outer);

    SDL_Rect inner = {x + 2, y + 2, size - 4, size - 4};
    SDL_SetRenderDrawColor(renderer, center_color.r, center_color.g, center_color.b, center_color.a);
    SDL_RenderFillRect(renderer, &inner);
}

/* Header bar: FPS (left), title (centered), clock+battery+percent (right) -- one aligned row. */
void title_statusbar_render_header(SDL_Renderer *renderer,
                                   TTF_Font *left_font, TTF_Font *title_font, TTF_Font *accent_font,
                                   TitleBatteryFill *battery_fill, TitleBatteryGlow *battery_glow,
                                   const TitleBackendStatus *backend,
                                   const char *left_text, const char *title_text)
{
    if (!renderer || !backend) {
        return;
    }

    const SDL_Color white = {235, 235, 240, 255};
    const SDL_Color accent_color = {220, 224, 232, 255};
    const int mid = TITLE_STATUSBAR_HEADER_HEIGHT / 2;

    if (left_font && left_text) {
        int left_h = 0;
        TTF_SizeUTF8(left_font, left_text, NULL, &left_h);
        title_draw_text(renderer, left_font, left_text, 12, mid - left_h / 2, accent_color, SDL_FALSE);
    }

    if (title_font && title_text) {
        int title_h = 0;
        TTF_SizeUTF8(title_font, title_text, NULL, &title_h);
        title_draw_text(renderer, title_font, title_text, BENCH_NATIVE_W / 2, mid - title_h / 2, white, SDL_TRUE);
    }

    if (!accent_font) {
        return;
    }

    /* Live status, not the one-time context-init snapshot -- reacts to plug/unplug immediately. */
    BenchDriverStatus live_status;
    bench_driver_get_status(&live_status);

    time_t now = time(NULL);
    struct tm local_time;
    localtime_r(&now, &local_time);
    char clock_text[16];
    strftime(clock_text, sizeof(clock_text), "%H:%M:%S", &local_time);
    int clock_w = 0, clock_h = 0;
    TTF_SizeUTF8(accent_font, clock_text, &clock_w, &clock_h);

    char percent_text[8];
    if (live_status.battery_percent >= 0) {
        SDL_snprintf(percent_text, sizeof(percent_text), "%d%%", live_status.battery_percent);
    } else {
        SDL_snprintf(percent_text, sizeof(percent_text), "n/a");
    }
    int percent_w = 0, percent_h = 0;
    TTF_SizeUTF8(accent_font, percent_text, &percent_w, &percent_h);

    const int icon_h = TITLE_BATTERY_ICON_HEIGHT;
    const int icon_w = title_battery_icon_width(icon_h);

    const int cluster_gap = 10;
    const int icon_percent_gap = 4;
    const int cluster_w = clock_w + cluster_gap + icon_w + icon_percent_gap + percent_w;
    const int right_x = BENCH_NATIVE_W - 12;

    const int clock_x = right_x - cluster_w;
    const int icon_x = clock_x + clock_w + cluster_gap;
    const int percent_x = icon_x + icon_w + icon_percent_gap;

    title_draw_text(renderer, accent_font, clock_text, clock_x, mid - clock_h / 2, accent_color, SDL_FALSE);
    title_draw_battery_icon(renderer, battery_fill, battery_glow, icon_x, mid - icon_h / 2,
                            icon_h, live_status.battery_percent, live_status.charging);
    title_draw_text(renderer, accent_font, percent_text, percent_x, mid - percent_h / 2, accent_color, SDL_FALSE);
}

/* Tagline (SDL/platform info), backend LEDs, then the version string -- stacked below top_y. */
void title_statusbar_render_status_header(SDL_Renderer *renderer, TTF_Font *small_font,
                                          const TitleBackendStatus *backend, int top_y,
                                          const char *version_text)
{
    if (!renderer || !small_font || !backend) {
        return;
    }

    const SDL_Color sep_color = {90, 95, 110, 255};
    const SDL_Color info_color = {225, 228, 235, 255};
    const SDL_Color led_label = {225, 228, 235, 255};

    /* Tagline: backend/platform info line. */
    char sdl_ver[24], display[24], renderer_str[40], cpu_ram[20], mma_pool[28];
    SDL_snprintf(sdl_ver, sizeof(sdl_ver), "SDL %d.%d.%d/%s",
                backend->sdl_major, backend->sdl_minor, backend->sdl_patch, backend->video_driver);
    SDL_snprintf(display, sizeof(display), "%dx%d@%dHz",
                backend->display_w, backend->display_h, backend->display_refresh_hz);
    SDL_snprintf(renderer_str, sizeof(renderer_str), "Renderer: %s", backend->renderer_name);
    SDL_snprintf(cpu_ram, sizeof(cpu_ram), "%dc/%dMB", backend->cpu_count, backend->ram_mb);
    SDL_snprintf(mma_pool, sizeof(mma_pool), "MMA %.1f/%.1fMB",
                backend->mma_pool_used_bytes / (1024.0 * 1024.0),
                backend->mma_pool_max_bytes / (1024.0 * 1024.0));

    const TitleStatusSegment info_segments[] = {
        {sdl_ver, info_color}, {display, info_color}, {renderer_str, info_color},
        {cpu_ram, info_color}, {mma_pool, info_color},
    };
    title_draw_segmented_line(renderer, small_font, info_segments, 5,
                              BENCH_NATIVE_W / 2, top_y, " | ", sep_color);

    int tagline_h = 0;
    TTF_SizeUTF8(small_font, sdl_ver, NULL, &tagline_h);

    /* LEDs, centered, below the tagline. */
    const int led_row_top = top_y + tagline_h + 1;
    const int led_size = 10;

    const struct { const char *label; SDL_bool ok; } leds[] = {
        {"Render", backend->render_ok},
        {"Joystick", backend->joystick_ok},
        {"Haptic", backend->haptic_ok},
        {"Power", backend->power_ok},
        {"Audio", backend->audio_ok},
        {"GL", backend->gl_ok},
    };
    const size_t led_count = sizeof(leds) / sizeof(leds[0]);

    int led_label_w[8];
    int total_led_w = 0;
    for (size_t i = 0; i < led_count; i++) {
        int label_h = 0;
        TTF_SizeUTF8(small_font, leds[i].label, &led_label_w[i], &label_h);
        total_led_w += led_size + 4 + led_label_w[i];
        if (i + 1 < led_count) {
            total_led_w += 14;
        }
    }

    int label_h_ref = 0;
    TTF_SizeUTF8(small_font, "Render", NULL, &label_h_ref);
    const int row_height = SDL_max(led_size, label_h_ref);
    const int led_y = led_row_top + (row_height - led_size) / 2;

    int x = BENCH_NATIVE_W / 2 - total_led_w / 2;
    for (size_t i = 0; i < led_count; i++) {
        int label_h = 0;
        TTF_SizeUTF8(small_font, leds[i].label, NULL, &label_h);
        const int label_y = led_row_top + (row_height - label_h) / 2;

        title_draw_led(renderer, x, led_y, led_size, leds[i].ok);
        title_draw_text(renderer, small_font, leds[i].label, x + led_size + 4, label_y, led_label, SDL_FALSE);

        x += led_size + 4 + led_label_w[i] + 14;
    }

    if (version_text) {
        const SDL_Color version_color = {150, 155, 168, 255};
        title_draw_text(renderer, small_font, version_text, BENCH_NATIVE_W / 2,
                        led_row_top + row_height + 3, version_color, SDL_TRUE);
    }
}

#define TITLE_KEYBIND_COUNT 6

/* Keybind legend, contextual to focus/edit state. */
void title_statusbar_render_footer(SDL_Renderer *renderer, TTF_Font *ui_font, TTF_Font *small_font,
                                   const TitleBackendStatus *backend, const TitleState *state)
{
    if (!renderer || !backend || !state) {
        return;
    }
    if (!small_font) {
        small_font = ui_font;
    }
    (void)small_font;

    const int top = BENCH_NATIVE_H - TITLE_STATUSBAR_FOOTER_HEIGHT;
    const SDL_Color color_a = {210, 214, 225, 255};
    const SDL_Color color_b = {130, 180, 255, 255};
    const SDL_Color sep_color = {90, 95, 110, 255};

    const SDL_bool config_focused = (state->focus == TITLE_FOCUS_CONFIG);
    const char *left_right_text = (config_focused && state->editing)
        ? "Left/Right: Change Value" : "Left/Right: Switch Category";
    const char *a_text = !config_focused ? "A: Launch" : (state->editing ? "A: Done" : "A: Edit");

    const char *const keybind_texts[TITLE_KEYBIND_COUNT] = {
        "Up/Down: Navigate", left_right_text, "L1/R1: Switch Panel", a_text, "Select: Info", "Exit: Quit",
    };
    TitleStatusSegment keybind_segments[TITLE_KEYBIND_COUNT];
    for (int i = 0; i < TITLE_KEYBIND_COUNT; i++) {
        keybind_segments[i].text = keybind_texts[i];
        keybind_segments[i].color = (i % 2 == 0) ? color_a : color_b;
    }
    title_draw_segmented_line(renderer, ui_font, keybind_segments, TITLE_KEYBIND_COUNT,
                              BENCH_NATIVE_W / 2, top + (TITLE_STATUSBAR_FOOTER_HEIGHT - 20) / 2, " | ", sep_color);
}
