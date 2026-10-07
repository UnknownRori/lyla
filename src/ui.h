#pragma once

#include <raylib.h>
#include "music/track.h"
#include "music/playlist.h"

#define HUD_BACKGROUND_COLOR 0x181818FF
#define HUD_TIMELINE_HEIGHT  12

Rectangle hud_visualizer_area(int w, int h);

void hud_draw_timeline(int w, int h);

void hud_draw_play_time(int w, int h);
void hud_draw_track_info(Track* track, int w, int h, f32 elapsed, f32 duration);

bool hud_playlist_typing(void);
void hud_playlist_free_search(void);
bool hud_update_playlist(Playlist* playlist, void (*play_next_song)(void));
void hud_render_playlist(Playlist* playlist, int w, int h);

void hud_draw_idle(int w, int h);

void hud_notify_load_error(void);
void hud_draw_notifications(int w, int h);

void hud_background_init();
void hud_background_shutdown();
void hud_background(Texture* thumbptr, int w, int h, f32 dt, f32 music_elapsed, f32 music_duration);

void hud_overlay_pause(int w, int h);
void hud_overlay_mode_on(int w, int h);

typedef enum {
    CONFIRM_NONE = 0,
    CONFIRM_SAVE,
    CONFIRM_DISCARD,
    CONFIRM_CANCEL,
} ConfirmResult;

void hud_confirm_open(const char* title, const char* message,
                      const char* save_label,
                      const char* discard_label,
                      const char* cancel_label);

bool hud_confirm_is_open(void);
ConfirmResult hud_confirm_update(void);
void hud_confirm_render(int w, int h);
