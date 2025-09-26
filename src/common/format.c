#include "common/format.h"

#include <SDL2/SDL.h>

const char *bench_audio_channel_label(int channels)
{
    switch (channels) {
        case 1: return "Mono";
        case 2: return "Stereo";
        case 3: return "2.1";
        case 4: return "Quad";
        case 5: return "4.1";
        case 6: return "5.1";
        case 7: return "6.1";
        case 8: return "7.1";
        default: return (channels > 0) ? "Multi" : "Unknown";
    }
}

void bench_format_bytes_human(size_t bytes,
                              char *buffer,
                              size_t buffer_size)
{
    if (!buffer || buffer_size == 0) {
        return;
    }

    const char *units[] = {"bytes", "KB", "MB", "GB"};
    double value = (double)bytes;
    int unit_idx = 0;

    while (value >= 1024.0 && unit_idx < (int)(SDL_arraysize(units) - 1)) {
        value /= 1024.0;
        unit_idx++;
    }

    if (unit_idx == 0) {
        SDL_snprintf(buffer, buffer_size, "%zu %s", bytes, units[unit_idx]);
    } else {
        SDL_snprintf(buffer, buffer_size, "%.2f %s", value, units[unit_idx]);
    }
}
