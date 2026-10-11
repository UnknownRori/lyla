#include "config.h"
#include "raylib.h"
#include "rconfig.h"
#include <stdlib.h>
#include <stdio.h>

static rori_config_t config;
Config g_config;

static inline u8   config_get_fps()
{
    i32 value;
    if (!rconfig_get_properties_i32(&config, "APP", "fps", &value)) {
        return 0;
    }

    return  (value < 255 && value > 0) ?
            value : 0;
}
static inline bool config_vsync()
{
    bool value = true;
    if (!rconfig_get_properties_bool(&config, "APP", "vsync", &value)) {
        return true;
    }

    return  value;
}

static inline bool config_always_on_top()
{
    bool value = true;
    if (!rconfig_get_properties_bool(&config, "APP", "always-on-top", &value)) {
        return true;
    }

    return  value;
}

static inline bool config_live_wallpaper()
{
    bool value = true;
    if (!rconfig_get_properties_bool(&config, "APP", "live_wallpaper", &value)) {
        return true;
    }

    return  value;
}

static inline bool config_bass_hit()
{
    bool value = true;
    if (!rconfig_get_properties_bool(&config, "FX", "bass_hit", &value)) {
        return true;
    }

    return  value;
}

static inline bool config_bloom()
{
    bool value = true;
    if (!rconfig_get_properties_bool(&config, "FX", "bloom", &value)) {
        return true;
    }

    return  value;
}

bool config_load()
{
    #define return_defer(X) do { result = (X); goto defer; } while (0)
    bool result = true;
    char* cfg = LoadFileText("config.cfg");
    if (cfg == NULL) {
        rconfig_init_default(&config);
        return_defer(false);
    }

    rconfig_parse_buffer(&config, cfg);
defer:
    g_config.fps            = config_get_fps();
    g_config.vsync          = config_vsync();
    g_config.always_on_top  = config_always_on_top();
    g_config.live_wallpaper = config_live_wallpaper();
    g_config.bass_hit       = config_bass_hit();
    g_config.bloom          = config_bloom();
    TraceLog(LOG_INFO, "vsync=%d", g_config.vsync);
    return result;
}


bool config_save()
{
    rori_config_t config;
    rconfig_init_default(&config);
    rconfig_set_properties_i32(&config, "APP", "fps", g_config.fps);
    rconfig_set_properties_bool(&config, "APP", "vsync", g_config.vsync);
    rconfig_set_properties_bool(&config, "APP", "always-on-top", g_config.always_on_top);
    rconfig_set_properties_bool(&config, "APP", "live_wallpaper", g_config.live_wallpaper);
    rconfig_set_properties_bool(&config, "FX", "bass_hit", g_config.bass_hit);
    rconfig_set_properties_bool(&config, "FX", "bloom", g_config.bloom);

    const usize BUFFER_SIZE = 1024*1024*10; // 10Mb
    char* big_ars_buffer = malloc(BUFFER_SIZE);
    usize size = 0;
    if (!rconfig_save_buffer(&config, big_ars_buffer, BUFFER_SIZE, &size)) {
        // TODO : Remove this later
        TraceLog(LOG_WARNING, "buffer too small");
    }
    FILE* f = fopen("config.cfg", "wb");
    if (f == NULL) {
        TraceLog(LOG_WARNING, "buffer too small");
    }
    fwrite(big_ars_buffer, 1, size, f);
    fclose(f);
    return true;
}
