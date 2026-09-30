#pragma once

#include "types.h"

void platform_tray_init();
void platform_tray_update();
void platform_tray_shutdown();

bool platform_tray_exit_signal();
