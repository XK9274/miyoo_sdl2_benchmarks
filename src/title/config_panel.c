#include "title/config_panel.h"

#include <stdio.h>

static const struct { int w; int h; const char *label; } g_resolutions[TITLE_RES_COUNT] = {
    {BENCH_NATIVE_W, BENCH_NATIVE_H, "640x480 (Native)"},
    {480, 320, "480x320"},
    {320, 240, "320x240"},
};

void title_resolution_dims(TitleLogicalRes res, int *out_w, int *out_h)
{
    int index = (res >= 0 && res < TITLE_RES_COUNT) ? (int)res : TITLE_RES_NATIVE;
    if (out_w) {
        *out_w = g_resolutions[index].w;
    }
    if (out_h) {
        *out_h = g_resolutions[index].h;
    }
}

const char *title_config_row_label(TitleConfigRow row)
{
    switch (row) {
        case TITLE_CONFIG_RESOLUTION:  return "Resolution";
        case TITLE_CONFIG_VSYNC:       return "VSync Mode";
        case TITLE_CONFIG_FRAME_LIMIT: return "Frame Limit";
        case TITLE_CONFIG_INPUT_MODE:  return "Input Mode";
        default:                       return "";
    }
}

void title_config_row_value_text(const TitleState *state, TitleConfigRow row, char *buf, size_t buf_size)
{
    if (!state || !buf || buf_size == 0) {
        return;
    }

    switch (row) {
        case TITLE_CONFIG_RESOLUTION:
            snprintf(buf, buf_size, "%s", g_resolutions[state->logical_res].label);
            break;
        case TITLE_CONFIG_VSYNC:
            switch (state->vsync_mode) {
                case BENCH_VSYNC_STATUS_OFF:      snprintf(buf, buf_size, "Off"); break;
                case BENCH_VSYNC_STATUS_ADAPTIVE: snprintf(buf, buf_size, "Adaptive"); break;
                case BENCH_VSYNC_STATUS_STRICT:   snprintf(buf, buf_size, "Strict"); break;
                default:                          snprintf(buf, buf_size, "?"); break;
            }
            break;
        case TITLE_CONFIG_FRAME_LIMIT:
            if (state->frame_limit_fps <= 0) {
                snprintf(buf, buf_size, "Off");
            } else {
                snprintf(buf, buf_size, "%d FPS", state->frame_limit_fps);
            }
            break;
        case TITLE_CONFIG_INPUT_MODE:
            snprintf(buf, buf_size, "%s",
                    state->input_mode == BENCH_INPUT_SOURCE_JOYSTICK ? "Joystick" : "Keyboard");
            break;
        default:
            buf[0] = '\0';
            break;
    }
}

SDL_bool title_config_row_disabled(TitleConfigRow row)
{
    return row == TITLE_CONFIG_RESOLUTION;
}
