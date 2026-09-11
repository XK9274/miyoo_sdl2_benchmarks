#ifndef LINES_BENCH_RENDER_H
#define LINES_BENCH_RENDER_H

#include "lines_bench/state.h"

void lines_render_init(LinesBenchState *state, SDL_Renderer *renderer);
void lines_render_cleanup(LinesBenchState *state);

int lines_render_cube_count(void);
int lines_render_anomaly_count(void);

void lines_render_scene(LinesBenchState *state,
                        SDL_Renderer *renderer,
                        BenchMetrics *metrics,
                        double delta_seconds);

#endif /* LINES_BENCH_RENDER_H */
