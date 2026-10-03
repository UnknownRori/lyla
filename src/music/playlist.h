#pragma once

#include "./player.h"
#include <rstb_da.h>

typedef struct {
    Track   *items;
    size_t  capacity;
    size_t  count;
    size_t  current;
} Playlist;

void playlist_init(Playlist* playlist);
void playlist_append(Playlist* playlist, Track track);
void playlist_next(Playlist* playlist);
void playlist_set(Playlist* playlist, size_t index);
void playlist_get(Playlist* playlist, Track* track, size_t index);
void playlist_get_current(Playlist* playlist, Track* track);
void playlist_clear(Playlist* playlist);

void playlist_load_ini(Playlist* playlist, const char* filepath);
bool playlist_save_ini(Playlist* playlist, const char* filepath);
