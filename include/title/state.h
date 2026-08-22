#ifndef TITLE_STATE_H
#define TITLE_STATE_H

#include <SDL2/SDL.h>

#include "bench_common.h"

#define TITLE_SUITE_COUNT 6

typedef struct {
    const char *label;
    const char *bin_name;
    const char *info;
} TitleSuiteEntry;

typedef enum {
    TITLE_RES_NATIVE = 0,
    TITLE_RES_480x320,
    TITLE_RES_320x240,
    TITLE_RES_COUNT
} TitleLogicalRes;

typedef enum {
    TITLE_CONFIG_RESOLUTION = 0,
    TITLE_CONFIG_VSYNC,
    TITLE_CONFIG_FRAME_LIMIT,
    TITLE_CONFIG_INPUT_MODE,
    TITLE_CONFIG_COUNT
} TitleConfigRow;

typedef enum {
    TITLE_FOCUS_LIST = 0,
    TITLE_FOCUS_CONFIG
} TitleFocusPane;

typedef enum {
    TITLE_MODE_MENU = 0,
    TITLE_MODE_CHILD_ERROR,
    TITLE_MODE_INFO_MODAL
} TitleMode;

typedef struct {
    TitleSuiteEntry suites[TITLE_SUITE_COUNT];
    int selected_suite;

    TitleLogicalRes logical_res;
    BenchVSyncStatus vsync_mode;
    BenchInputSource input_mode;
    int frame_limit_fps; /* 0 = off, else 30 or 60 */

    TitleFocusPane focus;
    int config_row;
    SDL_bool editing; /* config row is in edit mode -- Left/Right change its value */

    TitleMode mode;
    char error_message[160];
    int info_modal_suite;
} TitleState;

void title_state_init(TitleState *state);

/* Moves selection within the currently focused pane (delta: -1 or +1). */
void title_state_move_selection(TitleState *state, int delta);

/* Moves focus toward list (delta<0) or config (delta>0); clears edit mode. */
void title_state_move_focus_horizontal(TitleState *state, int delta);

/* Enters/exits edit mode for the focused config row; no-op if list pane focused. */
void title_state_toggle_edit(TitleState *state);

/* Cycles the value of the currently focused config row (delta: -1 or +1); no-op if list pane focused. */
void title_state_cycle_config(TitleState *state, int delta);

const TitleSuiteEntry *title_state_selected_suite(const TitleState *state);

/* True when the list selection is on the trailing Quit row (index TITLE_SUITE_COUNT). */
SDL_bool title_state_quit_selected(const TitleState *state);

void title_state_set_child_error(TitleState *state, const char *bin_name, SDL_bool crashed, int code_or_signal);

void title_state_clear_error(TitleState *state);

/* Opens the info modal for the currently list-selected suite; no-op on the Quit row. */
void title_state_open_info_modal(TitleState *state);

void title_state_close_info_modal(TitleState *state);

#endif /* TITLE_STATE_H */
