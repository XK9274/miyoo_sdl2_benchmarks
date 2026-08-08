#include "common/driver_support.h"

#include <stdio.h>
#include <string.h>

#include "controller_input.h"

#define MMIYOO_JOY_BUTTON_SLOTS 22
#define BENCH_DRIVER_REFRESH_INTERVAL_MS 2000

/* Index matches the MMIYOO_Button bit position reported by the Miyoo SDL2
 * joystick backend. POWER (index 19) has no bench action. Keyboard-emulation
 * equivalent: src/video/mmiyoo/SDL_event_mmiyoo.c. */
static const SDL_Keycode g_joy_button_map[MMIYOO_JOY_BUTTON_SLOTS] = {
    BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT,
    BTN_A, BTN_B, BTN_X, BTN_Y,
    BTN_L1, BTN_R1, BTN_L2, BTN_R2,
    BTN_SELECT, BTN_START, SDLK_ESCAPE,
    BTN_QUICK_SAVE, BTN_QUICK_LOAD, BTN_FAST_FORWARD, BTN_EXIT,
    0, BTN_VSYNC_TOGGLE, BTN_VSYNC_MODE_TOGGLE
};

static SDL_Joystick *g_joystick = NULL;
static SDL_Haptic *g_haptic = NULL;
static SDL_Window *g_window = NULL;
static SDL_Renderer *g_renderer = NULL;
static BenchDriverStatus g_status;
static SDL_mutex *g_status_mutex = NULL;

static SDL_Thread *g_refresh_thread = NULL;
static SDL_atomic_t g_refresh_running;

/* Does the actual (possibly blocking, e.g. axp_test popen on-device) power
 * query. Only ever called from bench_driver_init (once, synchronously, to
 * seed initial data) or the background thread -- never from a caller's
 * render loop. */
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

    SDL_LockMutex(g_status_mutex);
    g_status.power_state = state;
    g_status.power_info_valid = (state != SDL_POWERSTATE_UNKNOWN);
    g_status.battery_percent = percent;
    g_status.charging = (state == SDL_POWERSTATE_CHARGING || state == SDL_POWERSTATE_CHARGED);
    if (g_window) {
        g_status.display_w = display_w;
        g_status.display_h = display_h;
    }
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
    g_status.vsync_enabled = SDL_TRUE; /* forced on by SDL_CreateRenderer(-1, ...) */
    g_status.vsync_adaptive = SDL_TRUE;
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
        const Uint8 button = event->jbutton.button;
        if (button < MMIYOO_JOY_BUTTON_SLOTS) {
            const SDL_Keycode mapped = g_joy_button_map[button];
            if (mapped != 0) {
                SDL_LockMutex(g_status_mutex);
                g_status.input_source = BENCH_INPUT_SOURCE_JOYSTICK;
                g_status.joystick_event_count++;
                SDL_UnlockMutex(g_status_mutex);
                *out_sym = mapped;
                *out_pressed = (event->type == SDL_JOYBUTTONDOWN);
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
    if (!g_renderer) {
        return;
    }

    SDL_LockMutex(g_status_mutex);
    const SDL_bool want_enabled = !g_status.vsync_enabled;
    SDL_UnlockMutex(g_status_mutex);

    if (SDL_RenderSetVSync(g_renderer, want_enabled ? 1 : 0) == 0) {
        SDL_LockMutex(g_status_mutex);
        g_status.vsync_enabled = want_enabled;
        SDL_UnlockMutex(g_status_mutex);
    } else {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "bench_driver_toggle_vsync: SDL_RenderSetVSync failed: %s", SDL_GetError());
    }
}

void bench_driver_toggle_vsync_mode(void)
{
    const char *current = SDL_GetHint(BENCH_HINT_MMIYOO_VSYNC_ADAPTIVE);
    const SDL_bool is_adaptive = !(current && strcmp(current, "0") == 0);

    SDL_SetHint(BENCH_HINT_MMIYOO_VSYNC_ADAPTIVE, is_adaptive ? "0" : "1");

    SDL_LockMutex(g_status_mutex);
    g_status.vsync_adaptive = !is_adaptive;
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

void bench_driver_format_status_line(char *buf, size_t buf_size)
{
    if (!buf || buf_size == 0) {
        return;
    }

    BenchDriverStatus status;
    bench_driver_get_status(&status);

    char battery_part[32];
    if (status.battery_percent >= 0) {
        snprintf(battery_part, sizeof(battery_part), "BAT %d%%%s",
                 status.battery_percent, status.charging ? " CHG" : "");
    } else {
        snprintf(battery_part, sizeof(battery_part), "BAT n/a");
    }

    const char *current_hint = SDL_GetHint(BENCH_HINT_MMIYOO_INPUT_MODE);
    const SDL_bool forced_keyboard = (current_hint && strcmp(current_hint, BENCH_INPUT_MODE_KEYBOARD) == 0);

    char vsync_part[24];
    if (status.vsync_enabled) {
        snprintf(vsync_part, sizeof(vsync_part), "ON/%s", status.vsync_adaptive ? "Adaptive" : "Strict");
    } else {
        snprintf(vsync_part, sizeof(vsync_part), "OFF");
    }

    snprintf(buf, buf_size, "%s | JOY: %s | RUMBLE: %s | MODE: %s (START) | SRC: %s (kb %u / joy %u) | VSYNC: %s (VOL+/-) | %dx%d",
             battery_part,
             status.joystick_attached ? status.joystick_name : "none",
             status.rumble_supported ? "OK" : "n/a",
             forced_keyboard ? "Keyboard" : "Joystick",
             status.input_source == BENCH_INPUT_SOURCE_JOYSTICK ? "Joystick" : "Keyboard",
             status.keyboard_event_count,
             status.joystick_event_count,
             vsync_part,
             status.display_w, status.display_h);
}
