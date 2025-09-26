#include "render_suite/resources.h"

#include <stdio.h>
#include <stdlib.h>

SDL_Texture *rs_create_checker_texture(SDL_Renderer *renderer, int w, int h)
{
    SDL_Texture *texture = SDL_CreateTexture(renderer,
                                             SDL_PIXELFORMAT_RGBA8888,
                                             SDL_TEXTUREACCESS_STREAMING,
                                             w,
                                             h);
    if (!texture) {
        printf("Failed to create checker texture: %s\n", SDL_GetError());
        return NULL;
    }

    Uint32 *pixels = (Uint32 *)malloc((size_t)w * (size_t)h * sizeof(Uint32));
    if (!pixels) {
        SDL_DestroyTexture(texture);
        return NULL;
    }

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const int checker = ((x / 16) + (y / 16)) & 1;
            const Uint8 color = checker ? 220 : 40;
            pixels[y * w + x] = (0xFFu << 24) | (color << 16) | (color << 8) | color;
        }
    }

    SDL_UpdateTexture(texture, NULL, pixels, w * (int)sizeof(Uint32));
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    free(pixels);
    return texture;
}
