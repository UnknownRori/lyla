#pragma once

#include <raylib.h>

typedef struct Track {
    char* path;
    char* title;
    char* artist;
    char* album;
    char* link;
    Texture2D* thumbnail;
    Texture2D* qrcode;
    Music music;
} Track;

bool track_load(Track* track, const char* path);
bool track_set_thumbnail(Track* track, const char* path);
bool track_set_qrcode(Track* track, const char* link);
void track_unload(Track* track);
