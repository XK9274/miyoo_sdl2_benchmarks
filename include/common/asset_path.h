#ifndef COMMON_ASSET_PATH_H
#define COMMON_ASSET_PATH_H

#include <SDL2/SDL.h>
#include <stddef.h>

/* Resolves "<running-binary-dir>/../assets/<relative_name>" into out.
 * Returns SDL_FALSE if the binary's own path can't be read or out is too small. */
SDL_bool bench_resolve_asset_path(const char *relative_name, char *out, size_t out_size);

#endif /* COMMON_ASSET_PATH_H */
