#pragma once

#if defined(_WIN32)
#include <wchar.h>

// TODO : Crossplatform
int  media_init();
int  media_set_track(
    const wchar_t* title, 
    const wchar_t* artist,
    const wchar_t* album, 
    const wchar_t* cover_path
);
int  media_set_track_utf8(
    const char* title, 
    const char* artist,
    const char* album, 
    const char* cover_path
);
int  media_set_playing(int playing);
int  media_clear(void);
void media_shutdown(void);
#else
int  media_init();
int  media_set_track(
    const char* title, 
    const char* artist,
    const char* album, 
    const char* cover_path
);
int  media_set_track_utf8(
    const char* title, 
    const char* artist,
    const char* album, 
    const char* cover_path
);
int  media_set_playing(int playing);
int  media_clear(void);
void media_shutdown(void);
#endif
