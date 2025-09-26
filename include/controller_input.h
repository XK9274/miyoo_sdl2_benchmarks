#ifndef CONTROLLER_INPUT_H
#define CONTROLLER_INPUT_H

#include <SDL2/SDL.h>

/*
 * Unified Miyoo Mini key mapping for SDL tests.
 * These macros should be used in every benchmark so controls stay consistent.
 */
#define BTN_UP             SDLK_UP
#define BTN_DOWN           SDLK_DOWN
#define BTN_LEFT           SDLK_LEFT
#define BTN_RIGHT          SDLK_RIGHT

#define BTN_A              SDLK_SPACE
#define BTN_B              SDLK_LCTRL
#define BTN_X              SDLK_LSHIFT
#define BTN_Y              SDLK_LALT

#define BTN_L1             SDLK_e
#define BTN_R1             SDLK_t
#define BTN_L2             SDLK_TAB
#define BTN_R2             SDLK_BACKSPACE

#define BTN_SELECT         SDLK_RCTRL
#define BTN_START          SDLK_RETURN
#define BTN_MENU           SDLK_HOME
#define BTN_MENU_ONION     SDLK_HOME

#define BTN_QUICK_SAVE     SDLK_0
#define BTN_QUICK_LOAD     SDLK_1
#define BTN_FAST_FORWARD   SDLK_2
#define BTN_EXIT           SDLK_3

#endif /* CONTROLLER_INPUT_H */
