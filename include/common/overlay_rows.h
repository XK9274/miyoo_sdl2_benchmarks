#ifndef COMMON_OVERLAY_ROWS_H
#define COMMON_OVERLAY_ROWS_H

#include <SDL2/SDL.h>

#include "common/types.h"

typedef struct BenchOverlay BenchOverlay;

typedef enum {
    OVERLAY_ROW_FPS,
    OVERLAY_ROW_FRAME_TIME,
    OVERLAY_ROW_DRAW_CALLS,
    OVERLAY_ROW_VERTICES,
    OVERLAY_ROW_TRIANGLES,
    OVERLAY_ROW_TEXTURE_SWITCHES,
    OVERLAY_ROW_CPU_PERCENT,
    OVERLAY_ROW_RAM_USAGE,
    OVERLAY_ROW_MMA_USAGE,
    OVERLAY_ROW_MEMORY,
    OVERLAY_ROW_RESOURCE_OPS,
    OVERLAY_ROW_TIMING_OVERHEAD,
    /* Debug-build-only; silently skipped in a release build. */
    OVERLAY_ROW_CMDQUEUE_TIME,
    OVERLAY_ROW_PRESENT_TIME,
    OVERLAY_ROW_FILL_TIME,
    OVERLAY_ROW_COPY_TIME,
    OVERLAY_ROW_GEOMETRY_TIME,
    OVERLAY_ROW_LINES_TIME,
    OVERLAY_ROW_MISC_TIME,
    OVERLAY_ROW_GEOMETRY_STATS,
    /* Suite-specific: custom_label is a static format string with exactly
     * one %s placeholder, filled from bench_overlay_update's custom_values. */
    OVERLAY_ROW_CUSTOM
} OverlayRowKind;

#define OVERLAY_ROWS_MAX 24
#define OVERLAY_KEYBINDS_MAX 12
#define OVERLAY_CUSTOM_VALUE_LEN 96

typedef struct {
    OverlayRowKind kind;
    SDL_Color color;
    int alignment; /* 0=left, 1=center, 2=right */
    const char *custom_label;
} OverlayRowSpec;

typedef struct {
    const char *label;
    const char *action;
} OverlayKeybind;

/* Registers a suite's row/keybind layout. Call once at init, and again only
 * when the set actually changes (e.g. a scene switch changing which
 * keybinds apply). */
void bench_overlay_configure(BenchOverlay *overlay,
                              const OverlayRowSpec *rows, int row_count,
                              const OverlayKeybind *keybinds, int keybind_count);

/* Pushes live values through every configured row. custom_values[i]
 * corresponds positionally to the i-th OVERLAY_ROW_CUSTOM entry registered
 * via bench_overlay_configure. Call every frame. */
void bench_overlay_update(BenchOverlay *overlay, const BenchMetrics *metrics,
                          const char *const *custom_values, int custom_value_count);

#endif /* COMMON_OVERLAY_ROWS_H */
