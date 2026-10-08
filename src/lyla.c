#include <raylib.h>
#include <stdlib.h>
#include <math.h>

#include "./assets/rori.c"

#include "lyla.h"
#include "utils.h"
#include "music/randomizer.h"
#include "music/tag.h"
#include "platform/force.h"
#include "platform/tray.h"
#include "resources.h"
#include "thirdparty/discord.h"
#include "thirdparty/profiler.h"
#include "env.h"
#include "music/player.h"
#include "music/playlist.h"
#include "ui.h"
#include "input.h"
#include "text.h"
#include "visualizer/visualizer.h"
#include "platform/wallpaper.h"
#include "visualizer/fft.h"
#include "window_drag.h"

PROFILER_DEF(music_visual);
PROFILER_DEF(idle_visual);
PROFILER_DEF(fft_analyze);

static Playlist playlist = {0};
static FFT_Analyzer* analyzer = NULL;
static bool should_close = false;
static bool shuffle = false;
static bool show_track_info = true;
static bool force_track_show = false;
static bool force_on_top = false;
static bool force_detach = false;
static PlaylistRandomizer randomizer;
static Texture marking = {0};
static bool radial = false;
static bool idle_first = true;
static bool playlist_dirty = false;

#ifdef WITH_MICROPHONE
#define MIC_RATE 44100
#define MIC_CHANNELS 2
#define MIC_GAIN 4.0f
#define MIC_QUIET_THRESHOLD 0.01f

static FFT_Analyzer* mic_analyzer = NULL;
static AudioStream mic_stream = {0};
static bool mic_enabled = false;

static void mic_callback(void *buffer, unsigned int frames)
{
    f32 (*fs)[2] = buffer;
    ReadMicrophone((float *)buffer, frames);
    if (frames == 0) return;

    for (unsigned int i = 0; i < frames; ++i) {
        f32 sample = 0.5f*(fs[i][0] + fs[i][1])*MIC_GAIN;
        fft_analyzer_push(mic_analyzer, sample);
    }
}

static void mic_init(void)
{
    mic_analyzer = fft_analyzer_create();
    if (mic_analyzer == NULL) {
        TraceLog(LOG_ERROR, "Failed to allocate mic FFT Analyzer");
        return;
    }
    InitMicrophoneDevice(MIC_RATE, MIC_CHANNELS);
    mic_stream = LoadAudioStream(MIC_RATE, 32, MIC_CHANNELS);
    SetAudioStreamCallback(mic_stream, mic_callback);
    SetAudioStreamVolume(mic_stream, 0.0f);
    PlayAudioStream(mic_stream);
}

static void mic_shutdown(void)
{
    if (mic_analyzer == NULL) return;
    UnloadAudioStream(mic_stream);
    CloseMicrophoneDevice();
    fft_analyzer_destroy(mic_analyzer);
    mic_analyzer = NULL;
}

static void draw_mic_visualizer(int w, int h, f32 dt)
{
    PROFILER_REG(fft_analyze)
    size_t m = fft_analyzer_analyze(mic_analyzer, dt);
    PROFILER_END
    bool detach = force_detach;
    f32 beat = fft_analyzer_beat(mic_analyzer);

    if (radial) {
        visualizer_render_radial(hud_visualizer_area(w, h),
            fft_analyzer_smooth(mic_analyzer), fft_analyzer_smear(mic_analyzer),
            m, detach, beat, dt);
    } else {
        visualizer_render(hud_visualizer_area(w, h),
            fft_analyzer_smooth(mic_analyzer), fft_analyzer_smear(mic_analyzer),
            m, detach, beat, dt);
    }
    visualizer_draw_corner_glow(beat, w, h);
}
#endif

static void prepare_playlist_glyphs(void)
{
    for (size_t i = 0; i < playlist.count; i++) {
        Track* t = &playlist.items[i];
        text_collect(t->title);
        text_collect(t->artist);
        text_collect(t->album);
        text_collect(GetFileNameWithoutExt(t->path));
    }
    text_flush();
}

void next_song()
{
    if (shuffle) {
        u32 idx = playlist_randomizer_next(&randomizer);
        playlist_set(&playlist, idx % playlist.count);
        return;
    }
    if (playlist.count > 0) {
        playlist_next(&playlist);
    }
}


void play_song()
{
    if (playlist.count <= 0) return;
    Track* track = playlist_get_current_ref(&playlist);
    if (!player_play_track(track)) {
        next_song();
        play_song();
        return;
    }
    text_prepare(TextFormat("%s %s %s", player_name(), player_artist(),  player_album()));
}

static bool save_playlist(void)
{
    TraceLog(LOG_INFO, "saving");
    if (!playlist_save_ini(&playlist, "playlists.ini")) {
        TraceLog(LOG_INFO, "saving failed");
        return false;
    }
    playlist_dirty = false;
    return true;
}

static void request_exit(void)
{
    if (hud_confirm_is_open()) return;
    if (!playlist_dirty) { should_close = true; return; }
    hud_confirm_open(
        "Unsaved changes",
        "Your playlist has unsaved changes.\nSave them before exiting?",
        "Save", "Don't Save", "Cancel"
    );
}

static void handle_confirm_result(ConfirmResult r)
{
    switch (r) {
        case CONFIRM_SAVE:
            if (save_playlist()) should_close = true;
            else hud_confirm_open("Saving failed", "Could not write playlists.ini.",
                                  "Retry", "Exit anyway", "Cancel");
            break;
        case CONFIRM_DISCARD: should_close = true; break;
        default: break; // cancel / none
    }
}

static void handle_dropped_files(void)
{
    if (!IsFileDropped()) return;

    bool added = false;
    FilePathList files = LoadDroppedFiles();
    for (unsigned int i = 0; i < files.count; ++i) {
        Track track = {0};
        if (track_load(&track, files.paths[i])) {
            tag_meta_load(&track, files.paths[i], TAG_META_ALL);
            playlist_append(&playlist, track);
            added = true;
        }
    }
    prepare_playlist_glyphs();
    playlist_randomizer_init(&randomizer, playlist.count);
    playlist_set(&playlist, playlist.count - 1);
    play_song();
    UnloadDroppedFiles(files);

    if (added) playlist_dirty = true;
}

static void handle_keyboard(f32 dt)
{
    bool typing = hud_playlist_typing();

    if (hud_confirm_is_open()) {
        handle_confirm_result(hud_confirm_update());
        return;
    }

    if (!typing && input_key_pressed(KEY_F)) {
        ToggleBorderlessWindowed();
        SetWindowState(FLAG_WINDOW_UNDECORATED);
    }

#ifdef WITH_MICROPHONE
    if (input_key_pressed(KEY_M) && input_key_down(KEY_LEFT_CONTROL)) {
        mic_enabled = !mic_enabled;
    }
#endif

    if (hud_update_playlist(&playlist, play_song)) {
        return;
    }

    if (input_key_pressed(KEY_M)) {
#ifdef WITH_MICROPHONE
        mic_enabled = !mic_enabled;
#endif
        player_toggle_mute();
    }
    if (input_key_pressed(KEY_R)) player_reset_progress();
    if (input_key_pressed(KEY_C)) {
        if (playlist.count > 0) playlist_dirty = true;
        player_shutdown();
        playlist_clear(&playlist);
        idle_first = true;
        resource_reset();
    }
    if (input_key_pressed(KEY_S) && !input_key_down(KEY_LEFT_CONTROL)) shuffle = !shuffle;
    if (input_key_pressed(KEY_S) && input_key_down(KEY_LEFT_CONTROL)) save_playlist();
    if (input_key_pressed(KEY_D)) force_detach = !force_detach;
    if (input_key_pressed(KEY_F2)) radial = !radial;
    if (input_key_pressed(KEY_F1)) {
        force_on_top = !force_on_top;
        set_window_ontop(force_on_top);
    }
    if (input_key_down(KEY_UP))   player_change_volume(+0.5f*dt);
    if (input_key_down(KEY_DOWN)) player_change_volume(-0.5f*dt);
    if (input_key_pressed(KEY_T)) {
        show_track_info = !show_track_info;
        force_track_show = true;
    }
    if (input_key_pressed(KEY_ENTER)) {
        next_song();
        play_song();
    }

    if (player_has_track()) {
        if (input_key_pressed(KEY_SPACE)) player_toggle_pause();
        if (input_key_pressed(KEY_RIGHT)) player_seek_by(+5);
        if (input_key_pressed(KEY_LEFT))  player_seek_by(-5);
    }
}

static void draw_frame(int w, int h, f32 dt)
{

    if ((w < 256 || h < 256) && !force_track_show) {
        show_track_info = false;
    } else if (!force_track_show) {
        show_track_info = true;
    }


    BeginDrawing();
    PROFILER_BEGIN(frame)
    ClearBackground(ColorAlpha(GetColor(HUD_BACKGROUND_COLOR), 0.75f));

    if (player_has_track()) {
        PROFILER_REG(music_visual)
    
        Track* current = player_get_track();
        f32 fe = player_get_fast_energy(), se = player_get_slow_energy();
        f32 t = player_time();
        f32 du = player_length();
        f32 prog = player_progress();


        if (current->thumbnail != NULL) {
            hud_background(current->thumbnail, w, h, dt, t, du);
        }

#ifdef WITH_MICROPHONE
        if (mic_analyzer != NULL && mic_enabled && player_paused()) draw_mic_visualizer(w, h, dt);
        else
#endif
        {
            PROFILER_REG(fft_analyze)
            size_t m = fft_analyzer_analyze(analyzer, dt);
            static f32 quiet_time = 0.0f;
            PROFILER_END

            bool quiet;
            if (t < 15.0f || prog < 0.05f) {
                quiet = fe < 0.095f;
            } else if (se < 0.005f) {
                quiet = fe < 0.01f;
            } else if (prog > 0.98f) {
                quiet = fe < se*0.7f;
            } else {
                quiet = fe < se*0.15f;
            }

            quiet_time = quiet ? quiet_time + dt : 0.0f;
            bool detach = quiet_time > 0.4f;
            detach |= force_detach;
            detach |= player_paused();
            f32 beat = fft_analyzer_beat(analyzer);
            if (radial) {
                visualizer_render_radial(hud_visualizer_area(w, h),
                    fft_analyzer_smooth(analyzer), fft_analyzer_smear(analyzer),
                    m, detach, beat, dt);
            } else {
                visualizer_render(hud_visualizer_area(w, h),
                    fft_analyzer_smooth(analyzer), fft_analyzer_smear(analyzer),
                    m, detach, beat, dt);
            }
            visualizer_draw_corner_glow(beat, w, h);

            hud_draw_timeline(w, h);
            if (show_track_info) hud_draw_track_info(current, w, h, t, du);
            PROFILER_END
        }
    } else {
        PROFILER_REG(idle_visual)
        if (idle_first) {
            text_prepare("Drag & Drop a music file");
            idle_first = false;
        }
        hud_background(&marking, w, h, dt, 100., 200.); //:p
#ifdef WITH_MICROPHONE
        if (mic_analyzer != NULL && mic_enabled) draw_mic_visualizer(w, h, dt);
#endif
        hud_draw_idle(w, h);
        PROFILER_END
    }

    hud_overlay_pause(w, h);
    if (IsWallpaperAttached() && input_keyboard_active()) hud_overlay_mode_on(w, h);
    hud_render_playlist(&playlist, w, h);
    hud_draw_notifications(w, h);

    DrawTexturePro(
        marking, 
        RECT(0, 0, marking.width, marking.height), 
        RECT(w-8-64, h-16-64, 64, 64),
        VEC2_ZERO, 
        0.f, 
        WHITE
    );
    PROFILER_END;
    hud_confirm_render(w, h);
    profiler_draw(w, h);
    EndDrawing();
    profiler_update();
}

void lyla_init(void)
{
    analyzer = fft_analyzer_create();
    if (analyzer == NULL) {
        TraceLog(LOG_ERROR, "Failed to allocate FFT Analyzer");
        exit(69);
    }

    Image rori = LoadImageFromMemory(".png", __resources_rori_png, __resources_rori_png_len);
    marking = LoadTextureFromImage(rori);
    UnloadImage(rori);

    player_init(analyzer);
#ifdef WITH_MICROPHONE
    mic_init();
#endif
    playlist_init(&playlist);
    input_init();
    hud_background_init();
    if (!visualizer_init()) {
        TraceLog(LOG_WARNING, "Glow shader failed to compile, drawing bars only");
    }

    playlist_load_ini(&playlist, "playlists.ini");
    prepare_playlist_glyphs();
    discord_init(DISCORD_APP_ID);
    playlist_randomizer_init(&randomizer, playlist.count);
    if (playlist.count > 0 ) {
        shuffle = true;
        playlist_next(&playlist);
        play_song();
        player_toggle_pause();
        shuffle = false;
    }

    platform_tray_init();
    window_drag_init();
    profiler_init();
}

bool lyla_should_close()
{
    return should_close;
}

void lyla_update(void) 
{
    int w = GetScreenWidth();
    int h = GetScreenHeight();
    f32 dt = GetFrameTime();
    // dt = fminf(dt, 1.0f/60.0f)/2;

    platform_tray_update();
    input_update();

    static bool tray_was_signalled = false;
    bool tray = platform_tray_exit_signal();
    if (tray && !tray_was_signalled) request_exit();
    tray_was_signalled = tray;

    if (input_mouse_pressed(MOUSE_BUTTON_RIGHT) && !IsWallpaperAttached()) {
        request_exit();
    }
    if (player_time() > player_length() - 0.1) {
        next_song();
        play_song();
    }
    if (!hud_confirm_is_open()) {
        window_drag_update(w, h);
        handle_dropped_files();
    }
    handle_keyboard(dt);
    player_update();
    discord_update_presence(player_get_track(), player_paused());
    discord_update();
    draw_frame(w, h, dt);
}
void lyla_shutdown(void) 
{
    hud_playlist_free_search();
    platform_tray_shutdown();
    hud_background_shutdown();
    player_shutdown();
#ifdef WITH_MICROPHONE
    mic_shutdown();
#endif
    text_shutdown();
    visualizer_shutdown();
    fft_analyzer_destroy(analyzer);
    discord_shutdown();
}
