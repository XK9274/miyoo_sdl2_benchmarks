#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <stdio.h>

#include "title/config_panel.h"
#include "title/input.h"
#include "title/launcher.h"
#include "title/menu.h"
#include "title/state.h"

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    TitleContext ctx;
    if (!title_context_init(&ctx)) {
        fprintf(stderr, "sdl2_title: failed to initialise\n");
        return 1;
    }

    TitleState state;
    title_state_init(&state);

    SDL_bool running = SDL_TRUE;
    while (running) {
        const TitleAction action = title_handle_input(&state);

        if (action == TITLE_ACTION_QUIT) {
            running = SDL_FALSE;
            break;
        }

        if (action == TITLE_ACTION_LAUNCH) {
            const TitleSuiteEntry *suite = title_state_selected_suite(&state);
            if (suite) {
                TitleLaunchResult result;
                if (!title_launch_suite(&state, suite->bin_name, &ctx, &result)) {
                    fprintf(stderr, "sdl2_title: failed to relaunch title context after suite exit\n");
                    running = SDL_FALSE;
                    break;
                }

                if (result.exec_failed) {
                    title_state_set_child_error(&state, suite->bin_name, SDL_FALSE, 127);
                } else if (result.crashed) {
                    title_state_set_child_error(&state, suite->bin_name, SDL_TRUE, result.signal_number);
                } else if (result.exit_code != 0) {
                    title_state_set_child_error(&state, suite->bin_name, SDL_FALSE, result.exit_code);
                }
            }
        }

        title_menu_render(ctx.renderer, ctx.title_font, ctx.ui_font, &state);

        /* Menu is idle most of the time; a modest cap keeps CPU use sane
         * without needing the common frame-limit helper (that's for the
         * benchmark suites' presentation pacing, not this UI). */
        SDL_Delay(16);
    }

    title_context_shutdown(&ctx);
    return 0;
}
