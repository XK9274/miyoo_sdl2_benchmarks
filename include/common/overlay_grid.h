#ifndef COMMON_OVERLAY_GRID_H
#define COMMON_OVERLAY_GRID_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "common/types.h"

// Forward declaration
typedef struct BenchOverlay BenchOverlay;

#define OVERLAY_GRID_MAX_ROWS 10
#define OVERLAY_GRID_MAX_COLS 4
#define OVERLAY_CELL_TEXT_SIZE 192

typedef struct OverlayCell {
    char text[OVERLAY_CELL_TEXT_SIZE];
    SDL_Color color;
    int alignment;      // 0=left, 1=center, 2=right
    SDL_bool visible;
} OverlayCell;

typedef struct OverlayGridConfig {
    int cols;           // Number of columns (typically 2)
    int rows;           // Number of rows (standardized to 10)
    int spacing_x;      // Pixels between columns
    int spacing_y;      // Pixels between rows (line height)
    int padding;        // Edge padding
} OverlayGridConfig;

typedef struct OverlayGrid {
    OverlayGridConfig config;
    OverlayCell cells[OVERLAY_GRID_MAX_ROWS][OVERLAY_GRID_MAX_COLS];
    SDL_Color background;
    SDL_bool dirty;
} OverlayGrid;

// Grid management functions
void overlay_grid_init(OverlayGrid *grid, int cols, int rows);
void overlay_grid_set_spacing(OverlayGrid *grid, int x_spacing, int y_spacing, int padding);
void overlay_grid_set_background(OverlayGrid *grid, SDL_Color background);

// Cell manipulation functions
void overlay_grid_set_cell(OverlayGrid *grid, int row, int col,
                          SDL_Color color, int alignment, const char *fmt, ...);
void overlay_grid_set_row(OverlayGrid *grid, int row,
                         SDL_Color left_color, const char *left_text,
                         SDL_Color right_color, const char *right_text);
void overlay_grid_clear_cell(OverlayGrid *grid, int row, int col);
void overlay_grid_clear_row(OverlayGrid *grid, int row);

// Integration with existing overlay system
void overlay_grid_submit_to_overlay(OverlayGrid *grid, BenchOverlay *overlay);

#endif /* COMMON_OVERLAY_GRID_H */