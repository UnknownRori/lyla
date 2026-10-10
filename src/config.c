#include "config.h"
#include "raylib.h"
#include "rconfig.h"

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
        return 0;
    }

    return  value;
}

static inline bool config_always_on_top()
{
    bool value = true;
    if (!rconfig_get_properties_bool(&config, "APP", "always-on-top", &value)) {
        return 0;
    }

    return  value;
}

static inline bool config_live_wallpaper()
{
    bool value = true;
    if (!rconfig_get_properties_bool(&config, "APP", "fps", &value)) {
        return 0;
    }

    return  value;
}

static inline bool config_bass_hit()
{
    bool value = true;
    if (!rconfig_get_properties_bool(&config, "FX", "bass_hit", &value)) {
        return 0;
    }

    return  value;
}

static inline bool config_bloom()
{
    bool value = true;
    if (!rconfig_get_properties_bool(&config, "FX", "bloom", &value)) {
        return 0;
    }

    return  value;
}

bool config_load()
{
    char* cfg = LoadFileText("config.cfg");
    if (cfg == NULL) return false;

    rconfig_parse_buffer(&config, cfg);
    g_config.fps            = config_get_fps();
    g_config.vsync          = config_vsync();
    g_config.always_on_top  = config_always_on_top();
    g_config.live_wallpaper = config_live_wallpaper();
    g_config.bass_hit       = config_bass_hit();
    g_config.bloom          = config_bloom();
    return true;
}

