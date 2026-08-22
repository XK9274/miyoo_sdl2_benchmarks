#ifndef TITLE_INPUT_H
#define TITLE_INPUT_H

#include "title/state.h"

typedef enum {
    TITLE_ACTION_NONE = 0,
    TITLE_ACTION_LAUNCH,
    TITLE_ACTION_QUIT
} TitleAction;

/* Polls pending SDL events, updates state, returns the resulting action if any. */
TitleAction title_handle_input(TitleState *state);

#endif /* TITLE_INPUT_H */
