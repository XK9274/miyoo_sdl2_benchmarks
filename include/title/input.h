#ifndef TITLE_INPUT_H
#define TITLE_INPUT_H

#include "title/state.h"

typedef enum {
    TITLE_ACTION_NONE = 0,
    TITLE_ACTION_LAUNCH,
    TITLE_ACTION_QUIT
} TitleAction;

/* Polls pending SDL events and updates state accordingly. Returns the
 * highest-priority action that occurred this poll (launch or quit). */
TitleAction title_handle_input(TitleState *state);

#endif /* TITLE_INPUT_H */
