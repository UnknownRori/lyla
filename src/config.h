#pragma once

#include "types.h"

typedef struct {
    u8   fps;
    bool vsync;
    bool always_on_top;
    bool live_wallpaper;
    bool bass_hit;
    bool bloom;
} Config;

extern Config g_config;

bool config_load();
