#ifndef COMMON_DRIVER_SUPPORT_H
#define COMMON_DRIVER_SUPPORT_H

#include <SDL2/SDL.h>

#include "common/overlay.h"

typedef enum {
    BENCH_INPUT_SOURCE_KEYBOARD = 0,
    BENCH_INPUT_SOURCE_JOYSTICK
} BenchInputSource;

typedef enum {
    BENCH_VSYNC_STATUS_OFF = 0,
    BENCH_VSYNC_STATUS_ADAPTIVE,
    BENCH_VSYNC_STATUS_STRICT
} BenchVSyncStatus;

/* Mirrors the Miyoo SDL2 input-mode hint values. */
#define BENCH_HINT_MMIYOO_INPUT_MODE "SDL_MMIYOO_INPUT_MODE"
#define BENCH_INPUT_MODE_KEYBOARD "keyboard"
#define BENCH_INPUT_MODE_JOYSTICK "joystick"

/* Mirrors the Miyoo SDL2 vsync-mode hint values. */
#define BENCH_HINT_MMIYOO_VSYNC_MODE "SDL_MMIYOO_VSYNC_MODE"
#define BENCH_VSYNC_MODE_OFF      "off"
#define BENCH_VSYNC_MODE_ADAPTIVE "adaptive"
#define BENCH_VSYNC_MODE_STRICT   "strict"

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

    /* Requested SDL_MMIYOO_VSYNC_MODE value. */
    BenchVSyncStatus vsync_status;

    /* Driver-confirmed presentation pacing. */
    SDL_bool vsync_verified_active;
} BenchDriverStatus;

/* Opens input/haptics and starts the slow status refresh thread. */
SDL_bool bench_driver_init(SDL_Window *window, SDL_Renderer *renderer);
void bench_driver_shutdown(void);

/* Maps key/joystick button presses into the shared BTN_* keycode space. */
SDL_Keycode bench_driver_translate_event(const SDL_Event *event);

/* Maps key/joystick button transitions for held-input suites. */
SDL_bool bench_driver_translate_button_event(const SDL_Event *event,
                                             SDL_Keycode *out_sym,
                                             SDL_bool *out_pressed);

/* Best-effort rumble pulse; no-ops if no haptic device was opened. */
void bench_driver_rumble_pulse(float strength, Uint32 duration_ms);

/* Flips the driver's live SDL_MMIYOO_INPUT_MODE hint between keyboard and
 * joystick. The driver only ever posts events from whichever mode is
 * active, so this is a real switch, not just a display preference. */
void bench_driver_toggle_input_mode(void);

/* Toggles SDL_MMIYOO_VSYNC_MODE between "off" and "adaptive". */
void bench_driver_toggle_vsync(void);

/* Copies the current status snapshot out under lock. */
void bench_driver_get_status(BenchDriverStatus *out_status);

/* Formats the 2x4 driver status grid. */
void bench_driver_format_status_grid(char fields[BENCH_STATUS_GRID_CELLS][BENCH_STATUS_FIELD_LEN]);

#endif /* COMMON_DRIVER_SUPPORT_H */
