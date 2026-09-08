#include <SDL2/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "common/types.h"

#define MB_MAX_BUTTONS 3
#define MB_TITLE_POOL_COUNT 4
#define MB_MESSAGE_POOL_COUNT 5
#define MB_BUTTON_LABEL_POOL_COUNT 8

static const char *const g_title_pool[MB_TITLE_POOL_COUNT] = {
    "Low Battery",
    "Save Complete",
    "Confirm Action",
    "Heads Up",
};

static const char *const g_message_pool[MB_MESSAGE_POOL_COUNT] = {
    "Battery is running low, save your progress soon.",
    "Your save file was written successfully.",
    "This will overwrite an existing save slot.",
    "Something unexpected happened, but it's not fatal.",
    "Are you sure you want to continue?",
};

static const char *const g_button_label_pool[MB_BUTTON_LABEL_POOL_COUNT] = {
    "OK", "YES", "NO", "CANCEL", "RETRY", "IGNORE", "QUIT", "SAVE",
};

static int
rand_range(int min, int max)
{
    return min + (rand() % (max - min + 1));
}

/* Light inverse of the driver's own fixed dark default (bg #171717, light
 * text, #E8837F selected accent) -- same accent, neutrals flipped. */
static const SDL_MessageBoxColorScheme g_light_scheme = {
    {
        {0xF5, 0xF2, 0xED},
        {0x1B, 0x17, 0x10},
        {0xD6, 0xCF, 0xC2},
        {0xED, 0xE7, 0xDC},
        {0xE8, 0x83, 0x7F},
    }
};

static const SDL_MessageBoxColorScheme g_amber_scheme = {
    {
        {0x1C, 0x19, 0x17},
        {0xF2, 0xED, 0xE4},
        {0x4A, 0x41, 0x3A},
        {0x2A, 0x25, 0x21},
        {0xE8, 0xA3, 0x3D},
    }
};

static int
build_random_buttons(SDL_MessageBoxButtonData *buttons)
{
    int count = rand_range(1, MB_MAX_BUTTONS);
    int escape_index = (count > 1) ? rand_range(1, count - 1) : -1;
    int i;

    for (i = 0; i < count; ++i) {
        buttons[i].flags = 0;
        buttons[i].buttonid = i;
        buttons[i].text = g_button_label_pool[rand() % MB_BUTTON_LABEL_POOL_COUNT];
    }

    buttons[0].flags |= SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT;
    if (escape_index >= 0) {
        buttons[escape_index].flags |= SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT;
    } else {
        buttons[0].flags |= SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT;
    }

    return count;
}

static void
print_and_show(const char *label, const SDL_MessageBoxData *data)
{
    int buttonid = -1;
    int i;

    printf("[messagebox_probe] %s\n", label);
    printf("  title       : %s\n", data->title);
    printf("  message     : %s\n", data->message);
    printf("  colorScheme : %s\n", data->colorScheme ? "custom" : "NULL (driver default)");
    printf("  buttons     : %d\n", data->numbuttons);
    for (i = 0; i < data->numbuttons; ++i) {
        printf("    [%d] \"%s\"%s%s\n", data->buttons[i].buttonid, data->buttons[i].text,
               (data->buttons[i].flags & SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT) ? " (default)" : "",
               (data->buttons[i].flags & SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT) ? " (escape)" : "");
    }

    if (SDL_ShowMessageBox(data, &buttonid) < 0) {
        printf("  SDL_ShowMessageBox failed: %s\n\n", SDL_GetError());
        return;
    }
    printf("  -> buttonid %d\n\n", buttonid);
}

int main(int argc, char *argv[])
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_MessageBoxButtonData buttons[MB_MAX_BUTTONS];
    SDL_MessageBoxData data;

    (void)argc;
    (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow("SDL2 Message Box Probe",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              BENCH_NATIVE_W, BENCH_NATIVE_H,
                              SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_SetRenderDrawColor(renderer, 24, 26, 32, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    srand((unsigned int)time(NULL));

    SDL_zero(data);
    buttons[0].flags = SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT | SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT;
    buttons[0].buttonid = 0;
    buttons[0].text = "OK";
    data.flags = SDL_MESSAGEBOX_INFORMATION;
    data.window = window;
    data.title = "How this dialog gets here";
    data.message = "This app calls SDL_ShowMessageBox() directly. SDL's common "
                   "video layer (SDL_video.c) forwards the call to this driver's "
                   "SDL_VideoDevice::ShowMessageBox hook, MMIYOO_ShowMessageBox() "
                   "-- no SDL_Renderer is involved in drawing it.";
    data.numbuttons = 1;
    data.buttons = buttons;
    data.colorScheme = NULL;
    print_and_show("Box 1/3: explainer", &data);

    SDL_zero(data);
    data.flags = SDL_MESSAGEBOX_WARNING;
    data.window = window;
    data.title = g_title_pool[rand() % MB_TITLE_POOL_COUNT];
    data.message = g_message_pool[rand() % MB_MESSAGE_POOL_COUNT];
    data.numbuttons = build_random_buttons(buttons);
    data.buttons = buttons;
    data.colorScheme = &g_light_scheme;
    print_and_show("Box 2/3: randomized, light theme (colorScheme)", &data);

    SDL_zero(data);
    data.flags = SDL_MESSAGEBOX_ERROR;
    if (rand() % 2) {
        data.flags |= SDL_MESSAGEBOX_BUTTONS_RIGHT_TO_LEFT;
    }
    data.window = window;
    data.title = g_title_pool[rand() % MB_TITLE_POOL_COUNT];
    data.message = g_message_pool[rand() % MB_MESSAGE_POOL_COUNT];
    data.numbuttons = build_random_buttons(buttons);
    data.buttons = buttons;
    data.colorScheme = &g_amber_scheme;
    print_and_show("Box 3/3: randomized, amber theme (colorScheme)", &data);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
