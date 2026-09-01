#ifndef COMMON_ASSET_PATH_H
#define COMMON_ASSET_PATH_H

#include <SDL2/SDL.h>
#include <stddef.h>

/* Bundled font filenames, under assets/ -- single named source for both so
 * nothing else hardcodes the actual asset names. */
#define BENCH_APP_FONT_FILE "ThaleahFat.ttf"
#define BENCH_OVERLAY_FONT_FILE "Metrophobic-Regular.ttf"

/* Resolves "<running-binary-dir>/../assets/<relative_name>" into out.
 * Returns SDL_FALSE if the binary's own path can't be read or out is too small. */
SDL_bool bench_resolve_asset_path(const char *relative_name, char *out, size_t out_size);

#endif /* COMMON_ASSET_PATH_H */
