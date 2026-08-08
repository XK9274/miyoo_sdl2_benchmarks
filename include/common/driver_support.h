#ifndef COMMON_DRIVER_SUPPORT_H
#define COMMON_DRIVER_SUPPORT_H

#include <SDL2/SDL.h>

#include "common/overlay.h"

typedef enum {
    BENCH_INPUT_SOURCE_KEYBOARD = 0,
    BENCH_INPUT_SOURCE_JOYSTICK
} BenchInputSource;

/* Must match the driver's SDL_HINT_MMIYOO_INPUT_MODE / MMIYOO_INPUT_MODE_*
 * constants in sdl2_miyoo's src/core/mmiyoo/SDL_mmiyoo.h -- duplicated here
 * since the benchmark can't include that driver-private header, only link
 * against its public SDL2 API. Unset hint defaults to joystick. */
#define BENCH_HINT_MMIYOO_INPUT_MODE "SDL_MMIYOO_INPUT_MODE"
#define BENCH_INPUT_MODE_KEYBOARD "keyboard"
#define BENCH_INPUT_MODE_JOYSTICK "joystick"

/* Must match SDL_HINT_MMIYOO_VSYNC_ADAPTIVE in sdl2_miyoo's SDL_mmiyoo.h. */
#define BENCH_HINT_MMIYOO_VSYNC_ADAPTIVE "SDL_MMIYOO_VSYNC_ADAPTIVE"

typedef struct {
    SDL_bool joystick_attached;
    char joystick_name[64];
    int joystick_count;

    SDL_bool rumble_supported;
    SDL_bool rumble_active;

    BenchInputSource input_source;
    Uint32 joystick_event_count;
    Uint32 keyboard_event_count;

    SDL_bool power_info_valid;
    SDL_PowerState power_state;
    int battery_percent;
    SDL_bool charging;

    int display_w;
    int display_h;

    SDL_bool vsync_enabled;
    SDL_bool vsync_adaptive;
} BenchDriverStatus;

/* Opens joystick 0 and its haptic device if present, and spawns a background
 * thread that polls SDL_GetPowerInfo/window size at a slow (~2s) cadence.
 * The power query can shell out on-device (axp_test) and block for tens of
 * milliseconds, so it must never run on the caller's render thread -- that
 * is why this is a background thread rather than a per-frame call. Call
 * once after SDL_Init/SDL_CreateWindow/SDL_CreateRenderer. */
SDL_bool bench_driver_init(SDL_Window *window, SDL_Renderer *renderer);
void bench_driver_shutdown(void);

/* Maps an SDL_KEYDOWN or SDL_JOYBUTTONDOWN event onto the shared BTN_* keycode
 * space from controller_input.h, so a suite's existing keyboard switch can
 * also handle joystick input unchanged. Returns 0 if the event maps to no
 * bench action. Updates the tracked input source as a side effect. */
SDL_Keycode bench_driver_translate_event(const SDL_Event *event);

/* Same mapping as above, but for suites that track held button state (press
 * AND release), such as space_bench's movement/fire input. Returns SDL_TRUE
 * if the event is a mapped key or joystick button transition, filling
 * out_sym/out_pressed; SDL_FALSE if the event is irrelevant. */
SDL_bool bench_driver_translate_button_event(const SDL_Event *event,
                                             SDL_Keycode *out_sym,
                                             SDL_bool *out_pressed);

/* Best-effort rumble pulse; no-ops if no haptic device was opened. */
void bench_driver_rumble_pulse(float strength, Uint32 duration_ms);

/* Flips the driver's live SDL_MMIYOO_INPUT_MODE hint between keyboard and
 * joystick. The driver only ever posts events from whichever mode is
 * active, so this is a real switch, not just a display preference. */
void bench_driver_toggle_input_mode(void);

/* Toggles vsync on the renderer passed to bench_driver_init. */
void bench_driver_toggle_vsync(void);

/* Toggles adaptive vs strict vsync-wait mode. No effect if vsync is off. */
void bench_driver_toggle_vsync_mode(void);

/* Copies the current status snapshot out under lock. */
void bench_driver_get_status(BenchDriverStatus *out_status);

/* Fills an 8-cell (2 row x 4 col, row-major) status grid for
 * bench_overlay_set_status_grid: battery, joystick, rumble, input mode,
 * input source, event counts, vsync, and display resolution. Unused cells
 * are left as empty strings. */
void bench_driver_format_status_grid(char fields[BENCH_STATUS_GRID_CELLS][BENCH_STATUS_FIELD_LEN]);

#endif /* COMMON_DRIVER_SUPPORT_H */
