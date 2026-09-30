#include <raylib.h>
#include <stdlib.h>

#include "lyla.h"
#include "music/randomizer.h"
#include "platform/force.h"
#include "platform/tray.h"
#include "thirdparty/discord.h"
#include "env.h"
#include "music/player.h"
#include "music/playlist.h"
#include "ui.h"
#include "input.h"
#include "text.h"
#include "visualizer/visualizer.h"
#include "visualizer/fft.h"

static Playlist playlist = {0};
static FFT_Analyzer* analyzer = NULL;
static bool should_close = false;
static bool shuffle = false;
static bool show_track_info = true;
static bool force_track_show = false;
static bool force_on_top = false;
static PlaylistRandomizer randomizer;

void play_song()
{
    if (playlist.count <= 0) return;
    Track track = {0};
    playlist_get_current(&playlist, &track);
    player_play_track(track);
    text_prepare(TextFormat("%s %s %s", player_name(), player_artist(),  player_album()));
}

void next_song()
{
    if (shuffle) {
        u32 idx = playlist_randomizer_next(&randomizer);
        playlist_set(&playlist, idx % playlist.count);
        return;
    }
    playlist_next(&playlist);
}

static void handle_dropped_files(void)
{
    if (!IsFileDropped()) return;

    FilePathList files = LoadDroppedFiles();
    for (unsigned int i = 0; i < files.count; ++i) {
        Track track = {0};
        if (track_load(&track, files.paths[i])) {
            playlist_append(&playlist, track);
        }
    }
    playlist_randomizer_init(&randomizer, playlist.count);
    if (player_paused()) play_song();
    UnloadDroppedFiles(files);
}

static void handle_keyboard(void)
{
    if (IsKeyPressed(KEY_F)) {
        ToggleBorderlessWindowed();
        SetWindowState(FLAG_WINDOW_UNDECORATED);
    }

    if (hud_update_playlist(&playlist, play_song)) {
        return;
    }

    if (IsKeyPressed(KEY_M)) player_toggle_mute();
    if (IsKeyPressed(KEY_R)) player_reset_progress(0.0);
    if (IsKeyPressed(KEY_S)) shuffle = !shuffle;
    if (IsKeyPressed(KEY_F1)) {
        force_on_top = !force_on_top;
        set_window_ontop(force_on_top);
    }
    if (IsKeyDown(KEY_UP))   player_change_volume(+0.5f*GetFrameTime());
    if (IsKeyDown(KEY_DOWN)) player_change_volume(-0.5f*GetFrameTime());
    if (IsKeyPressed(KEY_T)) {
        show_track_info = !show_track_info;
        force_track_show = true;
    }
    if (IsKeyPressed(KEY_ENTER)) {
        next_song();
        play_song();
    }

    if (player_has_track()) {
        if (IsKeyPressed(KEY_SPACE)) player_toggle_pause();
        if (IsKeyPressed(KEY_RIGHT)) player_seek_by(+5);
        if (IsKeyPressed(KEY_LEFT))  player_seek_by(-5);
    }
}

static void draw_frame()
{
    int w = GetScreenWidth();
    int h = GetScreenHeight();

    if ((w < 256 || h < 256) && !force_track_show) {
        show_track_info = false;
    } else if (!force_track_show) {
        show_track_info = true;
    }

    size_t m = fft_analyzer_analyze(analyzer, GetFrameTime());

    BeginDrawing();
    ClearBackground(ColorAlpha(GetColor(HUD_BACKGROUND_COLOR), 0.75f));

    if (player_has_track()) {
        Track* current = player_get_track();

        if (current->thumbnail != NULL) {
            hud_background(*current->thumbnail, w, h);
        }

        bool detach = player_paused();
        if (player_progress() < 0.050 && player_get_fast_energy() < 0.11) {
            detach = true;
        } else if (player_progress() > 0.98 && player_get_fast_energy() < 0.15) {
            detach = true;
        } else if (player_get_fast_energy() < 0.06) {
            detach = true;
        }
        visualizer_render(hud_visualizer_area(w, h),
                          fft_analyzer_smooth(analyzer),
                          fft_analyzer_smear(analyzer),
                          m,
                          detach);
                          
        hud_draw_timeline(w, h);
        if (show_track_info) hud_draw_track_info(current, w, h);
    } else {
        hud_draw_idle(w, h);
    }

    hud_overlay_pause(w, h);
    hud_render_playlist(&playlist, w, h);
    hud_draw_notifications(w, h);

    EndDrawing();
}

void lyla_init(void)
{
    analyzer = fft_analyzer_create();
    if (analyzer == NULL) {
        TraceLog(LOG_ERROR, "Failed to allocate FFT Analyzer");
        exit(69);
    }

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
    }

    discord_init(DISCORD_APP_ID);
    playlist_randomizer_init(&randomizer, playlist.count);
    platform_tray_init();
}

bool lyla_should_close()
{
    return should_close || platform_tray_exit_signal();
}

void lyla_update(void) 
{
    platform_tray_update();
    input_default_update();
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        should_close = true;
    }
    if (player_progress() > 0.995) {
        next_song();
        play_song();
    }
    handle_dropped_files();
    handle_keyboard();
    player_update();
    discord_update_presence(player_get_track(), player_paused());
    discord_update();
    draw_frame();
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

