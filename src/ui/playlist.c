#include "music/playlist.h"
#include <stdio.h>
#include <stdlib.h>

#include "ui.h"
#include "text.h"
#include "common.h"
#include "utils.h"

static void hud_draw_backdrop(int w, int h)
{
    DrawRectangle(0, 0, w, h, (Color){0, 0, 0, 210});
}

static void hud_draw_modal_window(float box_x, float box_y, float box_w, float box_h)
{
    DrawRectangleRounded((Rectangle){box_x, box_y, box_w, box_h}, 0.05f, 4, (Color){22, 22, 22, 255});
    DrawRectangleRoundedLines((Rectangle){box_x, box_y, box_w, box_h}, 0.05f, 4, (Color){60, 60, 60, 255});
}

static void hud_draw_playlist_header(float box_x, float box_y)
{
    const char* header = "Playlist";
    text_draw(header, box_x + 24, box_y + 20, 16.0f, LIGHTGRAY);
}

static void hud_draw_playlist_items(Playlist* playlist, size_t selected_index, float box_x, float box_w, float start_y, float item_height, int max_visible, size_t start_idx)
{
    for (int i = 0; i < max_visible && (start_idx + i) < playlist->count; ++i) {
        size_t idx = start_idx + i;
        Track* t = &playlist->items[idx];

        float item_y = start_y + (i * item_height);
        float item_w = playlist->count > (size_t)max_visible ? box_w - 52.0f : box_w - 40.0f;
        Rectangle item_rect = {box_x + 20, item_y, item_w, item_height - 6};

        if (idx == selected_index) {
            DrawRectangleRounded(item_rect, 0.2f, 4, (Color){50, 50, 70, 255});
        }
        if (idx == playlist->current) {
            DrawRectangleRoundedLines(item_rect, 0.2f, 4, (Color){100, 200, 255, 255});
        }

        const char* display_name = t->title ? t->title : t->path;
        char line_buf[512];
        snprintf(line_buf, sizeof(line_buf), "%zu. %s", idx + 1, display_name);

        Color text_col = (idx == playlist->current) ? (Color){100, 200, 255, 255} : WHITE;
        text_draw(line_buf, item_rect.x + 16, item_rect.y + 10, 16.0f, text_col);
    }
}

static void hud_draw_playlist_scrollbar(Playlist* playlist, float box_x, float box_y, float box_w, float start_y, float item_height, int max_visible, size_t start_idx)
{
    float sb_x = box_x + box_w - 24.0f;
    float sb_y = start_y;
    float sb_w = 6.0f;
    float sb_h = max_visible * item_height;

    DrawRectangleRounded((Rectangle){sb_x, sb_y, sb_w, sb_h}, 1.0f, 4, (Color){40, 40, 40, 255});

    float thumb_h = sb_h * ((float)max_visible / playlist->count);
    if (thumb_h < 24.0f) thumb_h = 24.0f; // Minimum thumb height limit

    float scroll_progress = (float)start_idx / (playlist->count - max_visible);
    float thumb_y = sb_y + scroll_progress * (sb_h - thumb_h);

    DrawRectangleRounded((Rectangle){sb_x, thumb_y, sb_w, thumb_h}, 1.0f, 4, (Color){120, 120, 140, 255});
}

static void hud_draw_playlist(Playlist* playlist, size_t selected_index, int w, int h)
{
    if (!playlist || playlist->count == 0) return;

    hud_draw_backdrop(w, h);

    float box_w = w * 0.65f;
    float box_h = h * 0.65f;
    float box_x = (w - box_w) / 2.0f;
    float box_y = (h - box_h) / 2.0f;

    hud_draw_modal_window(box_x, box_y, box_w, box_h);
    hud_draw_playlist_header(box_x, box_y);

    float start_y = box_y + 64.0f;
    float item_height = 42.0f;
    int max_visible = (int)((box_h - 90.0f) / item_height);

    size_t start_idx = 0;
    if (selected_index >= (size_t)max_visible) {
        start_idx = selected_index - max_visible + 1;
    }

    hud_draw_playlist_items(playlist, selected_index, box_x, box_w, start_y, item_height, max_visible, start_idx);

    if (playlist->count > (size_t)max_visible) {
        hud_draw_playlist_scrollbar(playlist, box_x, box_y, box_w, start_y, item_height, max_visible, start_idx);
    }
}


static bool show_playlist = false;
static size_t playlist_selected_index = 0;

bool hud_update_playlist(Playlist* playlist, void (*play_next_song)(void))
{
    if (IsKeyPressed(KEY_TAB)) {
        show_playlist = !show_playlist;
        if (show_playlist) {
            playlist_selected_index = playlist->current;
        }
    }

    if (show_playlist) {
        if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_J)) {
            if (playlist->count > 0) playlist_selected_index = (playlist_selected_index + 1) % playlist->count;
        }
        if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_K)) {
            if (playlist->count > 0) playlist_selected_index = (playlist_selected_index + playlist->count - 1) % playlist->count;
        }
        if (IsKeyPressed(KEY_ENTER)) {
            playlist_set(playlist, playlist_selected_index);
            play_next_song();
            show_playlist = false;
        }
        if (IsKeyPressed(KEY_ESCAPE)) {
            show_playlist = false;
        }

        float wheel = GetMouseWheelMove();
        if (wheel != 0 && playlist->count > 0) {
            if (wheel < 0) {
                playlist_selected_index = (playlist_selected_index + 1) % playlist->count;
            } else {
                playlist_selected_index = (playlist_selected_index + playlist->count - 1) % playlist->count;
            }
        }
        return true;
    }
    return false;
}

void hud_render_playlist(Playlist* playlist, int w, int h)
{
    if (show_playlist) {
        hud_draw_playlist(playlist, playlist_selected_index, w, h);
    }
}

