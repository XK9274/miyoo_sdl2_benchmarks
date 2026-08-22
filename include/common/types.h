#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <SDL2/SDL.h>

/* Native physical framebuffer size on Miyoo Mini hardware. Always use these for
 * SDL_CreateWindow -- MMIYOO_CreateRenderer sizes the actual render target from
 * GFX_GetFrameWidth/Height() regardless of window w/h, so the window must always
 * be created at native size. Use bench_logical_w()/bench_logical_h() (see
 * common/display_config.h) for in-scene layout math instead. */
#define BENCH_NATIVE_W 640
#define BENCH_NATIVE_H 480

/* Compat aliases retained during the logical-size refactor; prefer BENCH_NATIVE_W/H. */
#define BENCH_SCREEN_W BENCH_NATIVE_W
#define BENCH_SCREEN_H BENCH_NATIVE_H
#define BENCH_OVERLAY_MAX_LINES 20  // Used by grid system: 10 rows × 2 columns

typedef struct {
    float x;
    float y;
    float dx;
    float dy;
    Uint8 r;
    Uint8 g;
    Uint8 b;
    Uint8 a;
    float life;
} BenchParticle;

typedef struct {
    Uint64 frame_count;
    double current_fps;
    double min_fps;
    double max_fps;
    double avg_fps;
    double frame_time_ms;
    double min_frame_time_ms;
    double max_frame_time_ms;
    Uint64 draw_calls;
    Uint64 vertices_rendered;
    Uint64 triangles_rendered;
    double accumulated_frame_time_ms;

    // Extended metrics for new benchmarks
    Uint64 geometry_batches;
    Uint64 texture_switches;
    Uint64 memory_allocated_bytes;
    Uint64 memory_peak_bytes;
    Uint64 scaling_operations;
    Uint64 pixel_operations;
    Uint64 resource_allocations;
    Uint64 resource_deallocations;
    double lock_unlock_overhead_ms;
    double scaling_overhead_ms;
    double allocation_time_ms;
} BenchMetrics;

typedef struct {
    char text[192];
    SDL_Color color;
    int column;      // 0 = left (metrics), 1 = right (controls)
    int alignment;   // 0 = left, 1 = center, 2 = right
} BenchOverlayLine;

#endif /* COMMON_TYPES_H */
