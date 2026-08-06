#ifndef SPACE_BENCH_OVERLAY_H
#define SPACE_BENCH_OVERLAY_H

#include "bench_common.h"
#include "space_bench/state.h"

#define SPACE_HUD_LINE_HEIGHT 18
#define SPACE_HUD_STRIP_HEIGHT (SPACE_HUD_LINE_HEIGHT * 2 + 6)

/* Draws the two-line perf/status strip across the top of the screen directly
 * to the renderer. No grid, no background overlay thread -- space_bench is a
 * game, not a passive metrics bench, so the rest of the screen stays clear
 * for play. */
void space_hud_render(SDL_Renderer *renderer,
                      const SpaceBenchState *state,
                      const BenchMetrics *metrics);

#endif /* SPACE_BENCH_OVERLAY_H */
