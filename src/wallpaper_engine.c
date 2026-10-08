// Standalone from the main thing
// It somewhat work filled with work around:p
// specifically crafted for wallpaper engine:p

#include <raylib.h>
#include <emscripten/emscripten.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "music/player.h"
#include "music/track.h"
#include "assets/rori.c"
#include "text.h"
#include "ui.h"
#include "utils.h"
#include "visualizer/visualizer.h"

typedef enum {
    PLAYBACK_STOPPED = 0,
    PLAYBACK_PLAYING = 1,
    PLAYBACK_PAUSED  = 2
} PlaybackState;

float g_AudioData[128] = { 0 };
Texture2D marking = { 0 };
Texture2D g_AlbumArtTexture = { 0 };
Track track = { 0 };

PlaybackState g_PlaybackState = PLAYBACK_STOPPED;
float g_MediaPosition = 0.0f;
float g_MediaDuration = 0.0f;

unsigned char* g_ImageData = NULL;
int g_ImageLen = 0;
char g_ImageExt[8] = ".png";
bool g_TextureNeedsUpdate = false;
bool g_TextureNeedsClear = false;

#define WE_INPUT_BINS 64
#define VISUAL_BINS   128

#define MIN_BAR_FLOOR 0.02f 

typedef struct {
    float raw_bins[WE_INPUT_BINS];
    float expanded_bins[VISUAL_BINS];
    
    float out_smooth[VISUAL_BINS];
    float out_smear[VISUAL_BINS];
    
    float prev_bass;
    float flux_avg;
    float cooldown;
    float beat;

    float fast_energy;
    float slow_energy;
    float quiet_time;
    
    float peak_tracker[WE_INPUT_BINS];
} WE_Analyzer;

static WE_Analyzer g_Analyzer = {0};

static float catmull_rom(float p0, float p1, float p2, float p3, float t) {
    float t2 = t * t;
    float t3 = t2 * t;
    return 0.5f * ((2.0f * p1) +
                   (-p0 + p2) * t +
                   (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
                   (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
}

void we_analyzer_feed_audio(WE_Analyzer* a, const float* audio128) {
    if (!a || !audio128) return;

    float sum_squares = 0.0f;
    float max_amp = 1.0f;

    for (int i = 0; i < WE_INPUT_BINS; ++i) {
        float combined = (audio128[i] + audio128[i + 64]) * 0.5f;
        a->raw_bins[i] = combined;

        if (combined > max_amp) {
            max_amp = combined;
        }

        sum_squares += combined * combined;
    }

    float current_energy = sqrtf(sum_squares / (float)WE_INPUT_BINS);
    float alpha_fast = 0.08f;
    float alpha_slow = 0.0008f;
    a->fast_energy = a->fast_energy * (1.0f - alpha_fast) + current_energy * alpha_fast;
    a->slow_energy = a->slow_energy * (1.0f - alpha_slow) + current_energy * alpha_slow;

    float normalized_inputs[WE_INPUT_BINS];
    for (int i = 0; i < WE_INPUT_BINS; ++i) {
        float progress = (float)i / (float)(WE_INPUT_BINS - 1);
        
        float freq_weight = 1.8f - 1.45f * progress;
        float norm = (a->raw_bins[i] / max_amp) * freq_weight;
        norm = powf(fmaxf(0.0f, norm), 0.85f);
        normalized_inputs[i] = fmaxf(MIN_BAR_FLOOR, fminf(1.0f, norm));
    }

    for (int i = 0; i < VISUAL_BINS; ++i) {
        float pos = ((float)i / (float)(VISUAL_BINS - 1)) * (WE_INPUT_BINS - 1);
        int idx = (int)pos;
        float t = pos - (float)idx;

        float p0 = normalized_inputs[(idx > 0) ? idx - 1 : 0];
        float p1 = normalized_inputs[idx];
        float p2 = normalized_inputs[(idx < WE_INPUT_BINS - 1) ? idx + 1 : WE_INPUT_BINS - 1];
        float p3 = normalized_inputs[(idx < WE_INPUT_BINS - 2) ? idx + 2 : WE_INPUT_BINS - 1];

        float val = catmull_rom(p0, p1, p2, p3, t);
        a->expanded_bins[i] = fmaxf(MIN_BAR_FLOOR, fminf(1.0f, val));
    }
}

size_t we_analyzer_analyze(WE_Analyzer* a, float dt) {
    if (!a) return 0;

    for (size_t i = 0; i < VISUAL_BINS; ++i) {
        float target = a->expanded_bins[i];
        
        float k = (target > a->out_smooth[i]) ? 60.0f : 8.0f;
        a->out_smooth[i] += (target - a->out_smooth[i]) * fminf(1.0f, k * dt);
        a->out_smear[i]  += (a->out_smooth[i] - a->out_smear[i]) * 3.0f * dt;
    }

    float bass = 0.0f;
    int bass_bins = 8;
    for (int q = 0; q < bass_bins; ++q) {
        bass += a->expanded_bins[q];
    }
    bass /= (float)bass_bins;

    float flux = fmaxf(0.0f, bass - a->prev_bass);
    a->prev_bass = bass;

    a->cooldown -= dt;
    if (a->cooldown <= 0.0f && flux > (a->flux_avg * 1.4f + 0.03f)) {
        a->beat = 1.0f;
        a->cooldown = 0.15f;
    }
    a->flux_avg += (flux - a->flux_avg) * fminf(1.0f, 2.0f * dt);
    a->beat *= expf(-8.0f * dt);

    return VISUAL_BINS;
}

const float* we_analyzer_smooth(const WE_Analyzer* a) { return a->out_smooth; }
const float* we_analyzer_smear(const WE_Analyzer* a)  { return a->out_smear; }
float we_analyzer_beat(const WE_Analyzer* a)          { return a->beat; }

static void safe_free_string(char** str_ptr) {
    if (str_ptr && *str_ptr) {
        free(*str_ptr);
        *str_ptr = NULL;
    }
}

void ClearTrackInfo(void) {
    safe_free_string(&track.title);
    safe_free_string(&track.artist);
    safe_free_string(&track.album);

    g_Analyzer.fast_energy = 0.0f;
    g_Analyzer.slow_energy = 0.0f;
    g_Analyzer.quiet_time  = 0.0f;
}

void ClearAlbumArtTexture(void) {
    if (IsTextureValid(g_AlbumArtTexture)) {
        UnloadTexture(g_AlbumArtTexture);
        g_AlbumArtTexture = (Texture2D){ 0 };
    }
    track.thumbnail = NULL;
}

EMSCRIPTEN_KEEPALIVE
void UpdateAudioData(const float* audioArray) {
    if (!audioArray) return;
    we_analyzer_feed_audio(&g_Analyzer, audioArray);
}

EMSCRIPTEN_KEEPALIVE
void UpdateMediaProperties(const char* title, const char* artist, const char* album) {
    ClearTrackInfo();

    if (title && strlen(title) > 0)    track.title  = strdup(title);
    if (artist && strlen(artist) > 0)  track.artist = strdup(artist);
    if (album && strlen(album) > 0)    track.album  = strdup(album);

    char* d = strdup(TextFormat("%s %s %s", title, artist, album));
    text_collect(d);
    text_flush();
    free(d);
}

EMSCRIPTEN_KEEPALIVE
void UpdateAlbumArt(const unsigned char* data, int length, const char* ext) {
    if (!data || length <= 0) {
        g_TextureNeedsClear = true;
        return;
    }

    unsigned char* newBuf = (unsigned char*)malloc(length);
    if (!newBuf) return;

    memcpy(newBuf, data, length);

    if (g_ImageData) free(g_ImageData);
    g_ImageData = newBuf;
    g_ImageLen = length;

    if (ext && strlen(ext) > 0) {
        strncpy(g_ImageExt, ext, sizeof(g_ImageExt) - 1);
    } else {
        strcpy(g_ImageExt, ".png");
    }

    g_TextureNeedsUpdate = true;
}

EMSCRIPTEN_KEEPALIVE
void ClearAlbumArt(void) {
    g_TextureNeedsClear = true;
}

EMSCRIPTEN_KEEPALIVE
void UpdatePlaybackState(int state) {
    g_PlaybackState = (PlaybackState)state;

    if (g_PlaybackState == PLAYBACK_STOPPED) {
        ClearTrackInfo();
        g_TextureNeedsClear = true;
        g_MediaPosition = 0.0f;
        g_MediaDuration = 0.0f;
    }
}

EMSCRIPTEN_KEEPALIVE
void UpdateMediaTimeline(float position, float duration) {
    g_MediaPosition = position;
    g_MediaDuration = duration;
}

void ProcessTextureUpdates(void) {
    if (g_TextureNeedsClear) {
        ClearAlbumArtTexture();
        if (g_ImageData) {
            free(g_ImageData);
            g_ImageData = NULL;
            g_ImageLen = 0;
        }
        g_TextureNeedsClear = false;
        g_TextureNeedsUpdate = false;
    }

    if (g_TextureNeedsUpdate && g_ImageData != NULL && g_ImageLen > 0) {
        Image img = LoadImageFromMemory(g_ImageExt, g_ImageData, g_ImageLen);

        if (img.data != NULL) {
            ClearAlbumArtTexture();
            g_AlbumArtTexture = LoadTextureFromImage(img);
            UnloadImage(img);
            track.thumbnail = &g_AlbumArtTexture;
        }

        free(g_ImageData);
        g_ImageData = NULL;
        g_ImageLen = 0;
        g_TextureNeedsUpdate = false;
    }
}

bool compute_detach_state(WE_Analyzer* a, float dt, bool force_detach) {
    if (!a) return false;

    float fe = a->fast_energy;
    float se = a->slow_energy;
    float t = g_MediaPosition; 
    float prog = (g_MediaDuration > 0.0f) ? (g_MediaPosition / g_MediaDuration) : 0.0f;

    bool quiet;
    if (t < 15.0f || prog < 0.05f) {
        quiet = fe < 0.15f;
    } else if (se < 0.015f) {
        quiet = fe < 0.03f;
    } else if (prog > 0.98f) {
        quiet = fe < (se * 0.85f);
    } else {
        quiet = fe < (se * 0.35f);
    }

    a->quiet_time = quiet ? (a->quiet_time + dt) : 0.0f;

    bool detach = (a->quiet_time > 0.2f);
    detach |= force_detach;
    detach |= (g_PlaybackState == PLAYBACK_PAUSED || g_PlaybackState == PLAYBACK_STOPPED);

    return detach;
}

void UpdateDrawFrame(void) {
    float dt = GetFrameTime();
    int w = GetScreenWidth();
    int h = GetScreenHeight();

    if (g_PlaybackState == PLAYBACK_PLAYING && g_MediaDuration > 0.0f) {
        g_MediaPosition += dt;
        if (g_MediaPosition > g_MediaDuration) {
            g_MediaPosition = g_MediaDuration;
        }
    }
    size_t m = we_analyzer_analyze(&g_Analyzer, dt);
    float beat = we_analyzer_beat(&g_Analyzer);
    bool detach = compute_detach_state(&g_Analyzer, dt, false);

    ProcessTextureUpdates();

    BeginDrawing();
        ClearBackground(BLACK);

        hud_background(&marking, w, h, dt, 50, 200);

        if (g_PlaybackState != PLAYBACK_STOPPED) {
            if (IsTextureValid(g_AlbumArtTexture)) {
                hud_background(&g_AlbumArtTexture, w, h, dt / 2., 50, 200);
            } else {
                hud_background(&marking, w, h, dt / 2., 50, 200);
            }
            visualizer_render(
                hud_visualizer_area(w, h),
                we_analyzer_smooth(&g_Analyzer),
                we_analyzer_smear(&g_Analyzer),
                m,
                detach,
                beat,
                dt
            );
            visualizer_draw_corner_glow(beat, w, h);
            if (g_MediaDuration == 0 || g_MediaPosition == 0) {
                g_MediaDuration = 200;
                g_MediaPosition = 50;
            }
            hud_draw_track_info(&track, w, h, 5, 50);
        } else {
            hud_background(&marking, w, h, dt / 2., 50, 200);
        }

        DrawTexturePro(
            marking, 
            RECT(0, 0, marking.width, marking.height), 
            RECT(w-8-32, h-16-32, 32, 32),
            VEC2_ZERO, 
            0.f, 
            WHITE
        );
    EndDrawing();
}

int main(void) {
    InitWindow(1280, 720, "Raylib WE Visualizer");

    Image rori = LoadImageFromMemory(".png", __resources_rori_png, __resources_rori_png_len);
    marking = LoadTextureFromImage(rori);
    UnloadImage(rori);

    hud_background_init();
    visualizer_init();
    text_init();

    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
    return 0;
}
