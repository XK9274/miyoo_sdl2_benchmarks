#ifndef COMMON_GL_EFFECT_LIBRARY_H
#define COMMON_GL_EFFECT_LIBRARY_H

#include <SDL2/SDL.h>

#define GL_EFFECT_LIBRARY_COUNT 15
#define GL_EFFECT_LIBRARY_SOFT_WAVES 1

int gl_effect_library_count(void);
const char *gl_effect_library_name(int index);
const char *gl_effect_library_fragment_source(int index);
SDL_bool gl_effect_library_index_from_name(const char *name, int *out_index);

#endif /* COMMON_GL_EFFECT_LIBRARY_H */
