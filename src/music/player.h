#pragma once

#include <stdbool.h>
#include "visualizer/fft.h"
#include "track.h"

void player_init(FFT_Analyzer *analyzer);
void player_shutdown(void);

bool player_play_track(Track* track);

void player_update(void);

bool        player_has_track(void);
const char* player_name(void);
const char* player_artist(void);
const char* player_album(void);
bool        player_paused(void);
void        player_toggle_pause(void);

Track*      player_get_track(void);

float player_time(void);
float player_length(void);
float player_progress(void);
void  player_reset_progress();
void  player_seek_by(float seconds);
void  player_seek_fraction(float t);

float player_get_fast_energy(void);
float player_get_slow_energy(void);

float player_volume(void);
void  player_change_volume(float delta);
void  player_toggle_mute(void);
