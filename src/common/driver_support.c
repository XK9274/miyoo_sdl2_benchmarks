#include "common/driver_support.h"

#include <stdio.h>
#include <string.h>

#include "controller_input.h"

#define MMIYOO_JOY_BUTTON_SLOTS 22
#define BENCH_DRIVER_REFRESH_INTERVAL_MS 2000

#define MMIYOO_JOY_BUTTON_X      6
#define MMIYOO_JOY_BUTTON_SELECT 12
#define MMIYOO_JOY_BUTTON_MENU   14

/* Indexed by the MMIYOO_Button bit positions reported by the joystick backend. */
static const SDL_Keycode g_joy_button_map[MMIYOO_JOY_BUTTON_SLOTS] = {
    BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT,
    BTN_A, BTN_B, BTN_X, BTN_Y,
    BTN_L1, BTN_R1, BTN_L2, BTN_R2,
    BTN_SELECT, BTN_START, BTN_MENU,
    BTN_QUICK_SAVE, BTN_QUICK_LOAD, BTN_FAST_FORWARD, BTN_EXIT,
    0, 0, 0
};

static SDL_Joystick *g_joystick = NULL;
static SDL_Haptic *g_haptic = NULL;
static SDL_Window *g_window = NULL;
static SDL_Renderer *g_renderer = NULL;
static BenchDriverStatus g_status;
static SDL_mutex *g_status_mutex = NULL;

static SDL_Thread *g_refresh_thread = NULL;
static SDL_atomic_t g_refresh_running;

/* Refreshes slow device status outside the render loop. */
static void bench_driver_refresh_status_locked(void)
{
    int seconds = -1;
    int percent = -1;
    const SDL_PowerState state = SDL_GetPowerInfo(&seconds, &percent);
    int display_w = 0;
    int display_h = 0;
    if (g_window) {
        SDL_GetWindowSize(g_window, &display_w, &display_h);
    }

    /* Miyoo SDL2's unset/unrecognized default is off. */
    const char *vsync_mode_hint = SDL_GetHint(BENCH_HINT_MMIYOO_VSYNC_MODE);
    BenchVSyncStatus vsync_status = BENCH_VSYNC_STATUS_OFF;
    if (vsync_mode_hint && strcmp(vsync_mode_hint, BENCH_VSYNC_MODE_ADAPTIVE) == 0) {
        vsync_status = BENCH_VSYNC_STATUS_ADAPTIVE;
    } else if (vsync_mode_hint && strcmp(vsync_mode_hint, BENCH_VSYNC_MODE_STRICT) == 0) {
        vsync_status = BENCH_VSYNC_STATUS_STRICT;
    }

    SDL_bool vsync_verified_active = SDL_FALSE;
    if (g_renderer) {
        SDL_RendererInfo info;
        if (SDL_GetRendererInfo(g_renderer, &info) == 0) {
            vsync_verified_active = (info.flags & SDL_RENDERER_PRESENTVSYNC) ? SDL_TRUE : SDL_FALSE;
        }
    }

    SDL_LockMutex(g_status_mutex);
    g_status.power_state = state;
    g_status.power_info_valid = (state != SDL_POWERSTATE_UNKNOWN);
    g_status.battery_percent = percent;
    g_status.charging = (state == SDL_POWERSTATE_CHARGING || state == SDL_POWERSTATE_CHARGED);
    if (g_window) {
        g_status.display_w = display_w;
        g_status.display_h = display_h;
    }
    g_status.vsync_status = vsync_status;
    g_status.vsync_verified_active = vsync_verified_active;
    SDL_UnlockMutex(g_status_mutex);
}

static int bench_driver_refresh_thread(void *userdata)
{
    (void)userdata;

    while (SDL_AtomicGet(&g_refresh_running)) {
        bench_driver_refresh_status_locked();
        for (int waited_ms = 0; waited_ms < BENCH_DRIVER_REFRESH_INTERVAL_MS && SDL_AtomicGet(&g_refresh_running); waited_ms += 100) {
            SDL_Delay(100);
        }
    }
    return 0;
}

SDL_bool bench_driver_init(SDL_Window *window, SDL_Renderer *renderer)
{
    memset(&g_status, 0, sizeof(g_status));
    g_status.input_source = BENCH_INPUT_SOURCE_KEYBOARD;
    g_status.battery_percent = -1;
    g_window = window;
    g_renderer = renderer;
    g_status_mutex = SDL_CreateMutex();

    if (SDL_InitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC) != 0) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "bench_driver_init: joystick/haptic subsystem unavailable: %s",
                    SDL_GetError());
    } else {
        SDL_JoystickEventState(SDL_ENABLE);
        g_status.joystick_count = SDL_NumJoysticks();
        if (g_status.joystick_count > 0) {
            g_joystick = SDL_JoystickOpen(0);
            if (g_joystick) {
                g_status.joystick_attached = SDL_TRUE;
                const char *name = SDL_JoystickName(g_joystick);
                if (name) {
                    strncpy(g_status.joystick_name, name, sizeof(g_status.joystick_name) - 1);
                }

                g_haptic = SDL_HapticOpenFromJoystick(g_joystick);
                if (g_haptic) {
                    if (SDL_HapticRumbleSupported(g_haptic) && SDL_HapticRumbleInit(g_haptic) == 0) {
                        g_status.rumble_supported = SDL_TRUE;
                    } else {
                        SDL_HapticClose(g_haptic);
                        g_haptic = NULL;
                    }
                }
            }
        }
    }

    bench_driver_refresh_status_locked();

    SDL_AtomicSet(&g_refresh_running, 1);
    g_refresh_thread = SDL_CreateThread(bench_driver_refresh_thread, "bench_driver_status", NULL);
    if (!g_refresh_thread) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "bench_driver_init: failed to start status thread: %s", SDL_GetError());
        SDL_AtomicSet(&g_refresh_running, 0);
    }

    return SDL_TRUE;
}

void bench_driver_shutdown(void)
{
    if (g_refresh_thread) {
        SDL_AtomicSet(&g_refresh_running, 0);
        SDL_WaitThread(g_refresh_thread, NULL);
        g_refresh_thread = NULL;
    }

    if (g_haptic) {
        SDL_HapticClose(g_haptic);
        g_haptic = NULL;
    }
    if (g_joystick) {
        SDL_JoystickClose(g_joystick);
        g_joystick = NULL;
    }
    g_window = NULL;
    g_renderer = NULL;

    if (g_status_mutex) {
        SDL_DestroyMutex(g_status_mutex);
        g_status_mutex = NULL;
    }
}

SDL_bool bench_driver_translate_button_event(const SDL_Event *event,
                                             SDL_Keycode *out_sym,
                                             SDL_bool *out_pressed)
{
    if (!event || !out_sym || !out_pressed) {
        return SDL_FALSE;
    }

    if (event->type == SDL_KEYDOWN || event->type == SDL_KEYUP) {
        SDL_LockMutex(g_status_mutex);
        g_status.input_source = BENCH_INPUT_SOURCE_KEYBOARD;
        g_status.keyboard_event_count++;
        SDL_UnlockMutex(g_status_mutex);
        *out_sym = event->key.keysym.sym;
        *out_pressed = (event->type == SDL_KEYDOWN);
        return SDL_TRUE;
    }

    if (event->type == SDL_JOYBUTTONDOWN || event->type == SDL_JOYBUTTONUP) {
        /* MENU held as a modifier: X = vsync off/adaptive toggle. MENU's own
           tap action still fires on release, but only if no combo was used
           during that hold. */
        static SDL_bool menu_held = SDL_FALSE;
        static SDL_bool menu_combo_used = SDL_FALSE;
        const Uint8 button = event->jbutton.button;
        const SDL_bool pressed = (event->type == SDL_JOYBUTTONDOWN);

        if (button == MMIYOO_JOY_BUTTON_MENU) {
            if (pressed) {
                menu_held = SDL_TRUE;
                menu_combo_used = SDL_FALSE;
                return SDL_FALSE;
            }

            menu_held = SDL_FALSE;
            if (menu_combo_used) {
                menu_combo_used = SDL_FALSE;
                return SDL_FALSE;
            }

            SDL_LockMutex(g_status_mutex);
            g_status.input_source = BENCH_INPUT_SOURCE_JOYSTICK;
            g_status.joystick_event_count++;
            SDL_UnlockMutex(g_status_mutex);
            *out_sym = BTN_MENU;
            *out_pressed = SDL_TRUE;
            return SDL_TRUE;
        }

        if (menu_held && button == MMIYOO_JOY_BUTTON_X) {
            if (pressed) {
                menu_combo_used = SDL_TRUE;
                SDL_LockMutex(g_status_mutex);
                g_status.input_source = BENCH_INPUT_SOURCE_JOYSTICK;
                g_status.joystick_event_count++;
                SDL_UnlockMutex(g_status_mutex);
                *out_sym = BTN_VSYNC_TOGGLE;
                *out_pressed = SDL_TRUE;
                return SDL_TRUE;
            }
            return SDL_FALSE;
        }

        if (button < MMIYOO_JOY_BUTTON_SLOTS) {
            const SDL_Keycode mapped = g_joy_button_map[button];
            if (mapped != 0) {
                SDL_LockMutex(g_status_mutex);
                g_status.input_source = BENCH_INPUT_SOURCE_JOYSTICK;
                g_status.joystick_event_count++;
                SDL_UnlockMutex(g_status_mutex);
                *out_sym = mapped;
                *out_pressed = pressed;
                return SDL_TRUE;
            }
        }
    }

    return SDL_FALSE;
}

SDL_Keycode bench_driver_translate_event(const SDL_Event *event)
{
    SDL_Keycode sym = 0;
    SDL_bool pressed = SDL_FALSE;

    if (bench_driver_translate_button_event(event, &sym, &pressed) && pressed) {
        return sym;
    }

    return 0;
}

void bench_driver_toggle_input_mode(void)
{
    const char *current = SDL_GetHint(BENCH_HINT_MMIYOO_INPUT_MODE);
    const SDL_bool is_keyboard = (current && strcmp(current, BENCH_INPUT_MODE_KEYBOARD) == 0);

    SDL_SetHint(BENCH_HINT_MMIYOO_INPUT_MODE,
                is_keyboard ? BENCH_INPUT_MODE_JOYSTICK : BENCH_INPUT_MODE_KEYBOARD);
}

void bench_driver_toggle_vsync(void)
{
    const char *current = SDL_GetHint(BENCH_HINT_MMIYOO_VSYNC_MODE);
    const SDL_bool is_off = (current && strcmp(current, BENCH_VSYNC_MODE_OFF) == 0);

    SDL_SetHint(BENCH_HINT_MMIYOO_VSYNC_MODE, is_off ? BENCH_VSYNC_MODE_ADAPTIVE : BENCH_VSYNC_MODE_OFF);

    SDL_LockMutex(g_status_mutex);
    g_status.vsync_status = is_off ? BENCH_VSYNC_STATUS_ADAPTIVE : BENCH_VSYNC_STATUS_OFF;
    SDL_UnlockMutex(g_status_mutex);
}

void bench_driver_rumble_pulse(float strength, Uint32 duration_ms)
{
    if (!g_haptic || !g_status.rumble_supported) {
        return;
    }

    if (strength < 0.0f) {
        strength = 0.0f;
    } else if (strength > 1.0f) {
        strength = 1.0f;
    }

    SDL_HapticRumblePlay(g_haptic, strength, duration_ms);

    SDL_LockMutex(g_status_mutex);
    g_status.rumble_active = SDL_TRUE;
    SDL_UnlockMutex(g_status_mutex);
}

void bench_driver_get_status(BenchDriverStatus *out_status)
{
    if (!out_status) {
        return;
    }
    SDL_LockMutex(g_status_mutex);
    *out_status = g_status;
    SDL_UnlockMutex(g_status_mutex);
}

void bench_driver_format_status_grid(char fields[BENCH_STATUS_GRID_CELLS][BENCH_STATUS_FIELD_LEN])
{
    if (!fields) {
        return;
    }

    BenchDriverStatus status;
    bench_driver_get_status(&status);

    const char *current_hint = SDL_GetHint(BENCH_HINT_MMIYOO_INPUT_MODE);
    const SDL_bool forced_keyboard = (current_hint && strcmp(current_hint, BENCH_INPUT_MODE_KEYBOARD) == 0);

    if (status.battery_percent >= 0) {
        snprintf(fields[0], BENCH_STATUS_FIELD_LEN, "BAT %d%%%s",
                 status.battery_percent, status.charging ? " CHG" : "");
    } else {
        snprintf(fields[0], BENCH_STATUS_FIELD_LEN, "BAT n/a");
    }

    snprintf(fields[1], BENCH_STATUS_FIELD_LEN, "JOY: %s",
             status.joystick_attached ? status.joystick_name : "none");

    snprintf(fields[2], BENCH_STATUS_FIELD_LEN, "RUMBLE: %s", status.rumble_supported ? "OK" : "n/a");

    snprintf(fields[3], BENCH_STATUS_FIELD_LEN, "MODE: %s (START)", forced_keyboard ? "Keyboard" : "Joystick");

    snprintf(fields[4], BENCH_STATUS_FIELD_LEN, "SRC: %s",
             status.input_source == BENCH_INPUT_SOURCE_JOYSTICK ? "Joystick" : "Keyboard");

    snprintf(fields[5], BENCH_STATUS_FIELD_LEN, "EVT kb %u / joy %u",
             status.keyboard_event_count, status.joystick_event_count);

    const char *mode_label = "Adaptive";
    if (status.vsync_status == BENCH_VSYNC_STATUS_OFF) {
        mode_label = "Off";
    } else if (status.vsync_status == BENCH_VSYNC_STATUS_STRICT) {
        mode_label = "Strict";
    }
    snprintf(fields[6], BENCH_STATUS_FIELD_LEN, "VSYNC: %s (MENU+X)", mode_label);

    snprintf(fields[7], BENCH_STATUS_FIELD_LEN, "%dx%d", status.display_w, status.display_h);
}
