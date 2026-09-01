#define _POSIX_C_SOURCE 200809L

#include "common/asset_path.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static SDL_bool bench_get_bin_dir(char *out_dir, size_t out_size)
{
    char exe_path[PATH_MAX];
    const ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len <= 0) {
        return SDL_FALSE;
    }
    exe_path[len] = '\0';

    char *slash = strrchr(exe_path, '/');
    if (!slash) {
        return SDL_FALSE;
    }
    *slash = '\0';

    if (strlen(exe_path) >= out_size) {
        return SDL_FALSE;
    }
    strcpy(out_dir, exe_path);
    return SDL_TRUE;
}

SDL_bool bench_resolve_asset_path(const char *relative_name, char *out, size_t out_size)
{
    if (!relative_name || !out || out_size == 0) {
        return SDL_FALSE;
    }

    char bin_dir[PATH_MAX];
    if (!bench_get_bin_dir(bin_dir, sizeof(bin_dir))) {
        return SDL_FALSE;
    }

    const int written = snprintf(out, out_size, "%s/../assets/%s", bin_dir, relative_name);
    return (written > 0 && (size_t)written < out_size) ? SDL_TRUE : SDL_FALSE;
}
