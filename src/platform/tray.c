#if defined(_WIN32)
#include <stdio.h>
#include "platform/wallpaper.h"
#include "types.h"

#define TRAY_WINAPI 1
#include "../../external/tray/tray.h"

static bool exit_signal = false;

static void exit_tray(struct tray_menu* item)
{
    exit_signal = true;
}

static void desktop_attach(struct tray_menu* item)
{
    if (IsWallpaperAttached()) {
        DetachWallpaper();
        return;
    }
    AttachAsWallpaper();
}

struct tray tray = {
    .icon = "icon.ico",
    .menu = (struct tray_menu[]){
        {"Toggle as Desktop Wallpaper", 0, 0, desktop_attach, NULL},
        {"Exit", 0, 0, exit_tray, NULL},
        {NULL},
    },
};


void platform_tray_init()
{
    tray_init(&tray);
}
void platform_tray_update()
{
    tray_loop(0);
}

void platform_tray_shutdown()
{
    tray_exit();
}

bool platform_tray_exit_signal()
{
    return exit_signal;
}
#else

#include <stdbool.h>

// TODO : Thinking if it actually useful to have system tray in this app
void platform_tray_init() {}
void platform_tray_update() {}
void platform_tray_shutdown() {}
bool platform_tray_exit_signal() { return false; }
#endif

