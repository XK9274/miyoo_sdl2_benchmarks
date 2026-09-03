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

    Uint32 last_ticks = SDL_GetTicks();

    SDL_bool running = SDL_TRUE;
    while (running) {
        const Uint32 now_ticks = SDL_GetTicks();
        const float dt = (now_ticks - last_ticks) / 1000.0f;
        last_ticks = now_ticks;
        title_fireflies_update(&ctx.fireflies, dt);

        const TitleAction action = title_handle_input(&state);

        if (action == TITLE_ACTION_QUIT) {
            running = SDL_FALSE;
            break;
        }

        if (action == TITLE_ACTION_LAUNCH && title_state_quit_selected(&state)) {
            running = SDL_FALSE;
            break;
        }

        if (action == TITLE_ACTION_LAUNCH) {
            const TitleSuiteEntry *entry = title_state_selected_entry(&state);
            if (entry) {
                TitleLaunchResult result;
                if (!title_launch_suite(&state, entry, &ctx, &result)) {
                    fprintf(stderr, "sdl2_title: failed to relaunch title context after suite exit\n");
                    running = SDL_FALSE;
                    break;
                }
                last_ticks = SDL_GetTicks(); /* avoid a huge dt spike after the suite ran */

                if (result.exec_failed) {
                    title_state_set_child_error(&state, entry->bin_name, SDL_FALSE, 127);
                } else if (result.crashed) {
                    title_state_set_child_error(&state, entry->bin_name, SDL_TRUE, result.signal_number);
                } else if (result.exit_code != 0) {
                    title_state_set_child_error(&state, entry->bin_name, SDL_FALSE, result.exit_code);
                }
            }
        }

        title_menu_render(&ctx, &state);
    }

    title_context_shutdown(&ctx);
    return 0;
}
