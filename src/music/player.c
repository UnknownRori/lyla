#include "player.h"

#include <math.h>
#include <stdio.h>
#include <raylib.h>
#include <string.h>

static FFT_Analyzer *analyzer;
static Track* music = NULL;
static bool loaded = false;
static bool paused = true;
static char name[256];
static f32 saved_volume = 0.5f;

static f32 fast_energy = 0.0f;
static f32 slow_energy = 0.0f;

f32 player_get_fast_energy(void) { return fast_energy; }
f32 player_get_slow_energy(void) { return slow_energy; }

static void audio_callback(void *buffer, unsigned int frames)
{
    f32 (*fs)[2] = buffer;
    f32 sum_squares = 0.0f;

    for (unsigned int i = 0; i < frames; ++i) {
        f32 sample = fs[i][0];
        fft_analyzer_push(analyzer, sample);
        sum_squares += sample * sample;
    }

    if (frames > 0) {
        f32 current_energy = sqrtf(sum_squares / frames);
        
        f32 alpha_fast = 0.05f;
        fast_energy = fast_energy * (1.0f - alpha_fast) + current_energy * alpha_fast;

        f32 alpha_slow = 0.0008f; 
        slow_energy = slow_energy * (1.0f - alpha_slow) + current_energy * alpha_slow;
    }
}

static void unload_current(void)
{
    if (!loaded) return;
    DetachAudioStreamProcessor(music->music.stream, audio_callback);
    StopMusicStream(music->music);
    loaded = false;
    music = NULL;
}

void player_init(FFT_Analyzer *a)
{
    analyzer = a;
    SetMasterVolume(0.5f);
}

void player_shutdown(void)
{
    unload_current();
}

bool player_play_track(Track* track)
{
    unload_current();
    music = track;
    if (!IsMusicValid(music->music)) {
        music->music = LoadMusicStream(music->path);
    }
    
    const char* display_name = music->title ? music->title : GetFileNameWithoutExt(music->path);
    strncpy(name, display_name, sizeof(name));
    
    fft_analyzer_reset(analyzer);
    
    AttachAudioStreamProcessor(music->music.stream, audio_callback);
    if (!IsMusicValid(music->music)) return false;
    PlayMusicStream(music->music);
    loaded = true;
    paused = false;
    slow_energy = false;
    fast_energy = false;
    
    return true;
}


void player_update(void)
{
    if (!loaded) return;
    UpdateMusicStream(music->music);
    if (!paused && !IsMusicStreamPlaying(music->music)) PlayMusicStream(music->music);
}

bool player_has_track(void) { return loaded; }
const char *player_name(void) { return name; }
bool player_paused(void) { return paused; }
Track* player_get_track(void) { return loaded ? music : NULL; }

void player_toggle_pause(void)
{
    if (!loaded && !IsMusicValid(music->music)) return;
    paused = !paused;
    if (paused) PauseMusicStream(music->music); else ResumeMusicStream(music->music);
}

f32 player_time(void)   { return loaded ? GetMusicTimePlayed(music->music) : 0; }
f32 player_length(void) { return loaded ? GetMusicTimeLength(music->music) : 0; }

f32 player_progress(void)
{
    f32 len = player_length();
    return len > 0 ? player_time()/len : 0;
}

void player_reset_progress()
{
    SeekMusicStream(music->music, 0.0);
}

void player_seek_by(f32 seconds)
{
    if (!loaded && !IsMusicValid(music->music)) return;
    f32 t = player_time() + seconds;
    f32 len = player_length();
    if (t < 0) t = 0;
    if (t > len) t = len;
    SeekMusicStream(music->music, t);
}

void player_seek_fraction(f32 t)
{
    if (!loaded && !IsMusicValid(music->music)) return;
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    SeekMusicStream(music->music, t*player_length());
}

f32 player_volume(void) { return GetMasterVolume(); }
const char *player_artist(void) { return music->artist; }
const char *player_album(void) {return music->album; }

void player_change_volume(f32 delta)
{
    f32 v = GetMasterVolume() + delta;
    if (v < 0) v = 0;
    if (v > 1) v = 1;
    SetMasterVolume(v);
}

void player_toggle_mute(void)
{
    f32 v = GetMasterVolume();
    if (v > 0) {
        saved_volume = v;
        SetMasterVolume(0);
    } else {
        SetMasterVolume(saved_volume);
    }
}
