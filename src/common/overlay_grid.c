#include "common/overlay_grid.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "common/overlay.h"

void overlay_grid_init(OverlayGrid *grid, int cols, int rows)
{
    if (!grid) {
        return;
    }

    memset(grid, 0, sizeof(*grid));

    grid->config.cols = (cols > 0 && cols <= OVERLAY_GRID_MAX_COLS) ? cols : 2;
    grid->config.rows = (rows > 0 && rows <= OVERLAY_GRID_MAX_ROWS) ? rows : OVERLAY_GRID_MAX_ROWS;
    grid->config.spacing_x = 8;     // Default column spacing
    grid->config.spacing_y = 20;    // Default row spacing
    grid->config.padding = 8;       // Default edge padding
    grid->background = (SDL_Color){0, 0, 0, 210};
    grid->dirty = SDL_TRUE;

    // Initialize all cells as empty and visible
    for (int row = 0; row < OVERLAY_GRID_MAX_ROWS; ++row) {
        for (int col = 0; col < OVERLAY_GRID_MAX_COLS; ++col) {
            grid->cells[row][col].text[0] = '\0';
            grid->cells[row][col].color = (SDL_Color){255, 255, 255, 255};
            grid->cells[row][col].alignment = 0;  // Left align by default
            grid->cells[row][col].visible = SDL_TRUE;
        }
    }
}

void overlay_grid_set_spacing(OverlayGrid *grid, int x_spacing, int y_spacing, int padding)
{
    if (!grid) {
        return;
    }

    grid->config.spacing_x = (x_spacing > 0) ? x_spacing : 8;
    grid->config.spacing_y = (y_spacing > 0) ? y_spacing : 20;
    grid->config.padding = (padding >= 0) ? padding : 8;
    grid->dirty = SDL_TRUE;
}

void overlay_grid_set_background(OverlayGrid *grid, SDL_Color background)
{
    if (!grid) {
        return;
    }

    grid->background = background;
    grid->dirty = SDL_TRUE;
}

void overlay_grid_set_cell(OverlayGrid *grid, int row, int col,
                          SDL_Color color, int alignment, const char *fmt, ...)
{
    if (!grid || !fmt) {
        return;
    }
    if (row < 0 || row >= grid->config.rows) {
        return;
    }
    if (col < 0 || col >= grid->config.cols) {
        return;
    }

    OverlayCell *cell = &grid->cells[row][col];
    cell->color = color;
    cell->alignment = (alignment >= 0 && alignment <= 2) ? alignment : 0;
    cell->visible = SDL_TRUE;

    va_list args;
    va_start(args, fmt);
    vsnprintf(cell->text, sizeof(cell->text), fmt, args);
    va_end(args);

    grid->dirty = SDL_TRUE;
}

void overlay_grid_set_row(OverlayGrid *grid, int row,
                         SDL_Color left_color, const char *left_text,
                         SDL_Color right_color, const char *right_text)
{
    if (!grid) {
        return;
    }
    if (row < 0 || row >= grid->config.rows) {
        return;
    }

    // Set left column
    if (left_text) {
        overlay_grid_set_cell(grid, row, 0, left_color, 0, "%s", left_text);
    } else {
        overlay_grid_clear_cell(grid, row, 0);
    }

    // Set right column if we have at least 2 columns
    if (grid->config.cols >= 2) {
        if (right_text) {
            overlay_grid_set_cell(grid, row, 1, right_color, 0, "%s", right_text);
        } else {
            overlay_grid_clear_cell(grid, row, 1);
        }
    }
}

void overlay_grid_clear_cell(OverlayGrid *grid, int row, int col)
{
    if (!grid) {
        return;
    }
    if (row < 0 || row >= grid->config.rows) {
        return;
    }
    if (col < 0 || col >= grid->config.cols) {
        return;
    }

    OverlayCell *cell = &grid->cells[row][col];
    cell->text[0] = '\0';
    cell->visible = SDL_TRUE;
    grid->dirty = SDL_TRUE;
}

void overlay_grid_clear_row(OverlayGrid *grid, int row)
{
    if (!grid) {
        return;
    }
    if (row < 0 || row >= grid->config.rows) {
        return;
    }

    for (int col = 0; col < grid->config.cols; ++col) {
        overlay_grid_clear_cell(grid, row, col);
    }
}

void overlay_grid_submit_to_overlay(OverlayGrid *grid, BenchOverlay *overlay)
{
    if (!grid || !overlay) {
        return;
    }

    // Convert grid directly to BenchOverlayLine array, bypassing broken builder
    BenchOverlayLine lines[BENCH_OVERLAY_MAX_LINES];
    int line_count = 0;

    // Convert grid to lines array - each row becomes 2 lines
    for (int row = 0; row < grid->config.rows && line_count < BENCH_OVERLAY_MAX_LINES; ++row) {
        for (int col = 0; col < grid->config.cols && line_count < BENCH_OVERLAY_MAX_LINES; ++col) {
            const OverlayCell *cell = &grid->cells[row][col];
            BenchOverlayLine *line = &lines[line_count];

            // Copy cell data to line
            strncpy(line->text, cell->text, sizeof(line->text) - 1);
            line->text[sizeof(line->text) - 1] = '\0';
            line->color = cell->color;
            line->column = col;
            line->alignment = cell->alignment;

            line_count++;
        }
    }

    // Submit directly to overlay, bypassing broken builder
    bench_overlay_submit(overlay, lines, line_count, grid->background);
    grid->dirty = SDL_FALSE;
}