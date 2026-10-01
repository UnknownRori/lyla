#include "./playlist.h"
#include "music/track.h"
#include "resources.h"
#include "rstb_da.h"
#include <stdio.h>
#include <rconfig.h>
#include <string.h>

void playlist_init(Playlist* playlist)
{
    memset(playlist, 0, sizeof(*playlist));
    rstb_da_reserve(playlist, 16);
}

void playlist_append(Playlist* playlist, Track track)
{
    rstb_da_append(playlist, track);
}

void playlist_next(Playlist* playlist)
{
    playlist_set(playlist, playlist->current+1);
}

void playlist_set(Playlist* playlist, size_t index)
{
    playlist->current = index % playlist->count;
}

void playlist_get(Playlist* playlist, Track* track, size_t index)
{
    *track = playlist->items[index];
}

void playlist_get_current(Playlist* playlist, Track* track)
{
    playlist_get(playlist, track, playlist->current);
}

void playlist_clear(Playlist* playlist)
{
    rstb_da_reset(playlist);
}

void playlist_load_ini(Playlist* playlist, const char* filepath)
{
    char* buffer = LoadFileText(filepath);
    if (!buffer) return;

    rori_config_t cfg;
    rconfig_init_default(&cfg);
    
    if (rconfig_parse_buffer(&cfg, buffer)) {
        int index = 1;
        char section[16];
        
        char temp[2024];
        while (true) {
            snprintf(section, sizeof(section), "%d", index);
            
            if (rconfig_get_properties_cstr(&cfg, section, "file", temp, sizeof(temp))) {
                char full_path[2024];
                snprintf(full_path, sizeof(full_path), "%s", temp);

                Track track = {0};
                if (track_load(&track, full_path)) {
                    
                    if (rconfig_get_properties_cstr(&cfg, section, "thumbnail", temp, sizeof(temp))) {
                        track_set_thumbnail(&track, temp);
                    }

                    if (rconfig_get_properties_cstr(&cfg, section, "link", temp, sizeof(temp))) {
                        track_set_qrcode(&track, temp);
                    }
                    if (rconfig_get_properties_cstr(&cfg, section, "title", temp, sizeof(temp))) {
                        track.title = strdup(temp);
                    }

                    if (rconfig_get_properties_cstr(&cfg, section, "album", temp, sizeof(temp))) {
                        track.album = strdup(temp);
                    }

                    if (rconfig_get_properties_cstr(&cfg, section, "artist", temp, sizeof(temp))) {
                        track.artist = strdup(temp);
                    }


                    playlist_append(playlist, track);
                }
            } else {
                break;
            }
            index++;
        }
    }
    
    rconfig_unload(&cfg);
    UnloadFileText(buffer);
}
