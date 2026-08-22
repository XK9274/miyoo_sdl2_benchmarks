#ifndef TITLE_CONFIG_PANEL_H
#define TITLE_CONFIG_PANEL_H

#include "title/state.h"

/* Physical/logical size for a TitleLogicalRes option. */
void title_resolution_dims(TitleLogicalRes res, int *out_w, int *out_h);

const char *title_config_row_label(TitleConfigRow row);

/* Formats the current value of a config row into buf, e.g. "640x480 (Native)", "Adaptive", "60 FPS", "Joystick". */
void title_config_row_value_text(const TitleState *state, TitleConfigRow row, char *buf, size_t buf_size);

/* True if this row can't currently be changed (shown greyed out, edit/cycle are no-ops). */
SDL_bool title_config_row_disabled(TitleConfigRow row);

#endif /* TITLE_CONFIG_PANEL_H */
