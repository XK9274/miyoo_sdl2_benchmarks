#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <stdio.h>
#include <stdlib.h>

#include "title/config_panel.h"
#include "title/input.h"
#include "title/launcher.h"
#include "title/menu.h"
#include "title/profile_run.h"
#include "title/state.h"

/* Set (to any value) to skip the menu and run every eligible entry once, then exit. */
#define BENCH_ENV_PROFILE_AUTORUN "BENCH_PROFILE_AUTORUN"

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

    const SDL_bool autorun = SDL_getenv(BENCH_ENV_PROFILE_AUTORUN) != NULL;
    if (autorun) {
        const char *duration_str = SDL_getenv(BENCH_ENV_PROFILE_DURATION_S);
        if (duration_str) {
            state.profile_duration_s = atoi(duration_str);
            if (state.profile_duration_s < TITLE_PROFILE_DURATION_MIN_S) {
                state.profile_duration_s = TITLE_PROFILE_DURATION_MIN_S;
            } else if (state.profile_duration_s > TITLE_PROFILE_DURATION_MAX_S) {
                state.profile_duration_s = TITLE_PROFILE_DURATION_MAX_S;
            }
        }
        title_state_profile_select_open(&state);
        if (title_profile_run_begin(&state)) {
            state.mode = TITLE_MODE_PROFILE_RUNNING;
        } else {
            fprintf(stderr, "sdl2_title: autorun could not start profiler run\n");
            title_context_shutdown(&ctx);
            return 1;
        }
    }

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

        if (action == TITLE_ACTION_PROFILE_SELECT) {
            title_state_profile_select_open(&state);
        }

        if (action == TITLE_ACTION_PROFILE_START) {
            if (title_profile_run_begin(&state)) {
                state.mode = TITLE_MODE_PROFILE_RUNNING;
            } else {
                state.mode = TITLE_MODE_CHILD_ERROR;
                snprintf(state.error_message, sizeof(state.error_message),
                        "Could not start profiler run (check logs/ is writable)");
            }
        }

        if (state.mode == TITLE_MODE_PROFILE_RUNNING) {
            title_menu_render(&ctx, &state);
            if (!title_profile_run_step(&state, &ctx)) {
                fprintf(stderr, "sdl2_title: failed to relaunch title context after profiler suite exit\n");
                running = SDL_FALSE;
                break;
            }
            if (state.profile_queue_index >= state.profile_queue_count) {
                if (autorun) {
                    running = SDL_FALSE;
                    break;
                }
                state.mode = TITLE_MODE_MENU;
            }
            last_ticks = SDL_GetTicks();
            continue;
        }

        title_menu_render(&ctx, &state);
    }

    title_context_shutdown(&ctx);
    return 0;
}
