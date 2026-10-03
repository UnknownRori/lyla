#include "./playlist.h"
#include "music/tag.h"
#include "music/track.h"
#include "raylib.h"
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

static char* ini_dup(rori_config_t* cfg, const char* section, const char* key)
{
    char temp[4096];
    if (!rconfig_get_properties_cstr(cfg, section, key, temp, sizeof(temp))) return NULL;
    if (temp[0] == '\0') return NULL;
    return strdup(temp);
}

void playlist_load_ini(Playlist* playlist, const char* filepath)
{
    char* buffer = LoadFileText(filepath);
    if (!buffer) return;

    rori_config_t cfg;
    rconfig_init_default(&cfg);

    if (rconfig_parse_buffer(&cfg, buffer)) {
        char section[32];

        for (int index = 1; ; index++) {
            snprintf(section, sizeof(section), "%d", index);

            char* file = ini_dup(&cfg, section, "file");
            if (!file) break;

            Track track = {0};
            if (!track_load(&track, file)) goto defer_1;

            char* title     = ini_dup(&cfg, section, "title");
            char* album     = ini_dup(&cfg, section, "album");
            char* artist    = ini_dup(&cfg, section, "artist");
            char* thumbnail = ini_dup(&cfg, section, "thumbnail");
            char* link      = ini_dup(&cfg, section, "link");

            unsigned need = 0;

            #define SET_METADATA(X, META) do { if ( (X) != NULL && strcmp((X), "") != 0 ) { track.X = (X); } else { need |= (META); } } while (0)
            SET_METADATA(title, TAG_META_TITLE);
            SET_METADATA(artist, TAG_META_ARTIST);
            SET_METADATA(album, TAG_META_ALBUM);

            if (thumbnail && strcmp(thumbnail, "") != 0) {
                track.thumbnail_path = thumbnail;
                track_set_thumbnail(&track, thumbnail);
            } else {
                need |= TAG_META_COVER;
            }

            if (link && strcmp(link, "") != 0) {
                track.qrcode_link = link;
                track_set_qrcode(&track, link);
            }

            tag_meta_load(&track, file, need);

            playlist_append(playlist, track);
        defer_1:
            free(file);
        }
    }

    rconfig_unload(&cfg);
    UnloadFileText(buffer);
}

bool playlist_save_ini(Playlist* playlist, const char* filepath)
{
#define return_defer(X) do { result = X; goto defer; } while (0)
    bool result = true;
    rori_config_t save;
    rconfig_init_default(&save);
    char section[16];
    for (usize i = 0; i < playlist->count; i++) {
        Track* track = &playlist->items[i];
        snprintf(section, 16, "%zu", i + 1);
        rconfig_get_or_create_section(&save, section);
        #define SAVE_PROPERTY(key, X) do { if (track->X != NULL) rconfig_set_properties_cstr(&save, section, key, track->X); } while (0)
        SAVE_PROPERTY("title", title);
        SAVE_PROPERTY("file", path);
        SAVE_PROPERTY("album", album);
        SAVE_PROPERTY("artist", artist);
        SAVE_PROPERTY("thumbnail", thumbnail_path);
        SAVE_PROPERTY("link", qrcode_link);
        #undef SAVE_PROPERTY
    }
    // TODO : Properly save
    const usize BUFFER_SIZE = 1024*1024*10; // 10Mb
    char* big_ars_buffer = malloc(BUFFER_SIZE);
    usize size = 0;
    if (!rconfig_save_buffer(&save, big_ars_buffer, BUFFER_SIZE, &size)) {
        // TODO : Remove this later
        TraceLog(LOG_WARNING, "buffer too small");
        return_defer(false);
    }
    FILE* f = fopen(filepath, "wb");
    if (f == NULL) {
        TraceLog(LOG_WARNING, "buffer too small");
        return_defer(false);
    }
    fwrite(big_ars_buffer, 1, size, f);
    fclose(f);
defer:
    if (big_ars_buffer) free(big_ars_buffer);
    rconfig_unload(&save);
    return result;
#undef return_defer
}
