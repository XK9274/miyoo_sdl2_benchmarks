#ifndef TEXTURE_BENCH_RENDER_H
#define TEXTURE_BENCH_RENDER_H

#include "texture_bench/state.h"

void texture_render_init(TextureBenchState *state, SDL_Renderer *renderer);
void texture_render_cleanup(TextureBenchState *state);

int texture_render_instance_count(void);

void texture_render_scene(TextureBenchState *state,
                          SDL_Renderer *renderer,
                          BenchMetrics *metrics,
                          double delta_seconds);

#endif /* TEXTURE_BENCH_RENDER_H */
