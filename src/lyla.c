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

void next_song()
{
    if (shuffle) {
        u32 idx = playlist_randomizer_next(&randomizer);
        playlist_set(&playlist, idx % playlist.count);
        return;
    }
    playlist_next(&playlist);
}


void play_song()
{
    if (playlist.count <= 0) return;
    Track track = {0};
    playlist_get_current(&playlist, &track);
    if (!player_play_track(track)) {
        next_song();
        play_song();
        return;
    }
    text_prepare(TextFormat("%s %s %s", player_name(), player_artist(),  player_album()));
}

static void handle_dropped_files(void)
{
    if (!IsFileDropped()) return;

    FilePathList files = LoadDroppedFiles();
    for (unsigned int i = 0; i < files.count; ++i) {
        Track track = {0};
        if (track_load(&track, files.paths[i])) {
            tag_meta_load(&track, files.paths[i], TAG_META_ALL);
            playlist_append(&playlist, track);
        }
    }
    playlist_randomizer_init(&randomizer, playlist.count);
    playlist_set(&playlist, playlist.count - 1);
    play_song();
    UnloadDroppedFiles(files);
}

static void handle_keyboard(f32 dt)
{
    if (input_key_pressed(KEY_F)) {
        ToggleBorderlessWindowed();
        SetWindowState(FLAG_WINDOW_UNDECORATED);
    }

    if (hud_update_playlist(&playlist, play_song)) {
        return;
    }

    if (input_key_pressed(KEY_M)) player_toggle_mute();
    if (input_key_pressed(KEY_R)) player_reset_progress();
    if (input_key_pressed(KEY_C)) {
        playlist.count = 0; // TODO : De-allocate cached resource
        resource_reset();
        player_shutdown();
    }
    if (input_key_pressed(KEY_S)) shuffle = !shuffle;
    if (input_key_pressed(KEY_D)) force_detach = !force_detach;
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

    if (IsKeyPressed(KEY_S) && IsKeyDown(KEY_LEFT_CONTROL)) {
        TraceLog(LOG_INFO, "saving");
        if (!playlist_save_ini(&playlist, "playlists.ini")) {
            // TODO : Send error notification
            TraceLog(LOG_INFO, "saving failed");
        }
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
    ClearBackground(ColorAlpha(GetColor(HUD_BACKGROUND_COLOR), 0.75f));

    if (player_has_track()) {
        size_t m = fft_analyzer_analyze(analyzer, GetFrameTime());
        Track* current = player_get_track();

        if (current->thumbnail != NULL) {
            hud_background(current->thumbnail, w, h, dt);
        }


        bool detach = player_paused();
        static f32 quiet_time = 0.0f;

        f32 fe = player_get_fast_energy(), se = player_get_slow_energy();
        f32 t = player_time();
        f32 prog = player_progress();

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
        detach = quiet_time > 0.4f;
        detach |= force_detach;
        f32 beat = fft_analyzer_beat(analyzer);
        visualizer_render(hud_visualizer_area(w, h),
            fft_analyzer_smooth(analyzer), fft_analyzer_smear(analyzer),
            m, detach, beat, dt);

        hud_draw_timeline(w, h);
        if (show_track_info) hud_draw_track_info(current, w, h, player_time(), player_length());
        visualizer_draw_corner_glow(beat, w, h);
    } else {
        hud_background(&marking, w, h, dt);
        hud_draw_idle(w, h);
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

    // DrawFPS(0, 0);

    EndDrawing();
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
    playlist_init(&playlist);
    input_init();
    hud_background_init();
    if (!visualizer_init()) {
        TraceLog(LOG_WARNING, "Glow shader failed to compile, drawing bars only");
    }

    playlist_load_ini(&playlist, "playlists.ini");
    if (playlist.count > 0 ) {
        play_song();
        player_toggle_pause();
    }

    discord_init(DISCORD_APP_ID);
    playlist_randomizer_init(&randomizer, playlist.count);
    platform_tray_init();
    window_drag_init();
}

bool lyla_should_close()
{
    return should_close || platform_tray_exit_signal();
}

void lyla_update(void) 
{
    int w = GetScreenWidth();
    int h = GetScreenHeight();
    f32 dt = GetFrameTime();
    dt = fminf(dt, 1.0f/20.0f)/2;

    window_drag_update(w, h);
    platform_tray_update();
    input_update();
    if (input_mouse_pressed(MOUSE_BUTTON_RIGHT) && !IsWallpaperAttached()) {
        should_close = true;
    }
    if (player_progress() > 0.998) {
        next_song();
        play_song();
    }
    handle_dropped_files();
    handle_keyboard(dt);
    player_update();
    discord_update_presence(player_get_track(), player_paused());
    discord_update();
    draw_frame(w, h, dt);
}
void lyla_shutdown(void) 
{
    platform_tray_shutdown();
    hud_background_shutdown();
    player_shutdown();
    text_shutdown();
    visualizer_shutdown();
    fft_analyzer_destroy(analyzer);
    discord_shutdown();
}

