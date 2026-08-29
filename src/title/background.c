#include "title/background.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#include <SDL2/SDL_image.h>

#include "title/launcher.h"

SDL_Texture *title_background_load(SDL_Renderer *renderer)
{
    if (!renderer) {
        return NULL;
    }

    char bin_dir[PATH_MAX];
    if (!title_get_bin_dir(bin_dir, sizeof(bin_dir))) {
        return NULL;
    }

    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/../assets/title_bg.png", bin_dir);

    SDL_Surface *surface = IMG_Load(path);
    if (!surface) {
        fprintf(stderr, "title: failed to load background %s: %s\n", path, IMG_GetError());
        return NULL;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (!texture) {
        fprintf(stderr, "title: failed to create background texture: %s\n", SDL_GetError());
    }
    return texture;
}
