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

// static void strip_quotes(char* str) 
// {
//     int j = 0;
//     for (int i = 0; str[i] != '\0'; i++) {
//         if (str[i] != '"') {
//             str[j++] = str[i];
//         }
//     }
//     str[j] = '\0';
// }

void playlist_load_ini(Playlist* playlist, const char* filepath)
{
    char* buffer = LoadFileText(filepath);
    if (!buffer) return;

    rori_config_t cfg;
    rconfig_init_default(&cfg);
    
    if (rconfig_parse_buffer(&cfg, buffer)) {
        int index = 1;
        char section[16];
        char file_val[256];
        char thumb_val[256];
        
        char temp[1024];
        while (true) {
            snprintf(section, sizeof(section), "%d", index);
            
            if (rconfig_get_properties_cstr(&cfg, section, "file", file_val, sizeof(file_val))) {
                // strip_quotes(file_val);
                
                const char* dir = GetDirectoryPath(filepath);
                char full_path[1024];
                if (strlen(dir) > 0) {
                    snprintf(full_path, sizeof(full_path), "%s/%s", dir, file_val);
                } else {
                    snprintf(full_path, sizeof(full_path), "%s", file_val);
                }

                Track track = {0};
                if (track_load(&track, full_path)) {
                    
                    if (rconfig_get_properties_cstr(&cfg, section, "thumbnail", thumb_val, sizeof(thumb_val))) {
                        // strip_quotes(thumb_val);
                        
                        char thumb_path[512];
                        if (strlen(dir) > 0) {
                            snprintf(thumb_path, sizeof(thumb_path), "%s/%s", dir, thumb_val);
                        } else {
                            snprintf(thumb_path, sizeof(thumb_path), "%s", thumb_val);
                        }
                        
                        track_set_thumbnail(&track, thumb_path);
                    }

                    if (rconfig_get_properties_cstr(&cfg, section, "link", temp, sizeof(temp))) {

                        track_set_qrcode(&track, temp);
                    }
                    if (rconfig_get_properties_cstr(&cfg, section, "title", temp, sizeof(temp))) {
                        // strip_quotes(temp);
                        track.title = strdup(temp);
                    }

                    if (rconfig_get_properties_cstr(&cfg, section, "album", temp, sizeof(temp))) {
                        // strip_quotes(temp);
                        track.album = strdup(temp);
                    }

                    if (rconfig_get_properties_cstr(&cfg, section, "artist", temp, sizeof(temp))) {
                        // strip_quotes(temp);
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
