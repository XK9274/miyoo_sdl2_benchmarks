#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <SDL2/SDL.h>

#define BENCH_SCREEN_W 640
#define BENCH_SCREEN_H 480
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

// BenchOverlayBuilder struct removed - use OverlayGrid from overlay_grid.h instead

#endif /* COMMON_TYPES_H */
