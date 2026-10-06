#include "music/playlist.h"
#include <stdio.h>
#include <stdlib.h>

#include "raylib.h"
#include "ui.h"
#include "input.h"
#include "text.h"
#include "common.h"
#include "types.h"

#define PLAYLIST_ANCHOR_X 0.5f
#define PLAYLIST_ANCHOR_Y 0.5f
#define PLAYLIST_WIDTH_RATIO 0.65f
#define PLAYLIST_HEIGHT_RATIO 0.65f
#define PLAYLIST_OFFSET_X 0.0f
#define PLAYLIST_OFFSET_Y 0.0f
#define PLAYLIST_SLIDE_SPEED 6.0f
#define PLAYLIST_BACKDROP_ALPHA 210.0f
#define PLAYLIST_WHEEL_STEP 2

typedef struct {
    f32 box_x;
    f32 box_y;
    f32 box_w;
    f32 box_h;
    f32 start_y;
    f32 item_height;
    i32 max_visible;
} PlaylistLayout;

static f32 playlist_slide = 0.0f;
static bool show_playlist = false;
static usize playlist_selected_index = 0;
static usize playlist_scroll_offset = 0;

static PlaylistLayout hud_playlist_layout(i32 w, i32 h, f32 progress)
{
    PlaylistLayout l;
    l.box_w = w * PLAYLIST_WIDTH_RATIO;
    l.box_h = h * PLAYLIST_HEIGHT_RATIO;
    l.box_x = (w - l.box_w) * PLAYLIST_ANCHOR_X + PLAYLIST_OFFSET_X;

    f32 target_y = (h - l.box_h) * PLAYLIST_ANCHOR_Y + PLAYLIST_OFFSET_Y;
    f32 inv = 1.0f - progress;
    f32 eased = 1.0f - inv * inv * inv;
    l.box_y = h + (target_y - h) * eased;

    l.start_y = l.box_y + 64.0f;
    l.item_height = 42.0f;
    l.max_visible = (i32)((l.box_h - 90.0f) / l.item_height);
    if (l.max_visible < 1) l.max_visible = 1;
    return l;
}

static usize hud_playlist_max_scroll(usize count, i32 max_visible)
{
    return count > (usize)max_visible ? count - (usize)max_visible : 0;
}

static void hud_playlist_clamp_scroll(usize count, i32 max_visible)
{
    usize max_scroll = hud_playlist_max_scroll(count, max_visible);
    if (playlist_scroll_offset > max_scroll) playlist_scroll_offset = max_scroll;
}

static void hud_playlist_ensure_visible(usize count, i32 max_visible)
{
    if (playlist_selected_index < playlist_scroll_offset) {
        playlist_scroll_offset = playlist_selected_index;
    } else if (playlist_selected_index >= playlist_scroll_offset + (usize)max_visible) {
        playlist_scroll_offset = playlist_selected_index - (usize)max_visible + 1;
    }
    hud_playlist_clamp_scroll(count, max_visible);
}

static Rectangle hud_playlist_item_rect(const PlaylistLayout* l, usize count, i32 slot)
{
    f32 item_y = l->start_y + (slot * l->item_height);
    f32 item_w = count > (usize)l->max_visible ? l->box_w - 52.0f : l->box_w - 40.0f;
    return (Rectangle){l->box_x + 20, item_y, item_w, l->item_height - 6};
}

static i32 hud_playlist_slot_at(const PlaylistLayout* l, usize count, Vector2 mouse)
{
    for (i32 i = 0; i < l->max_visible && (playlist_scroll_offset + i) < count; ++i) {
        Rectangle r = hud_playlist_item_rect(l, count, i);
        if (CheckCollisionPointRec(mouse, r)) return i;
    }
    return -1;
}

static void hud_draw_backdrop(i32 w, i32 h, f32 progress)
{
    DrawRectangle(0, 0, w, h, (Color){0, 0, 0, (u8)(PLAYLIST_BACKDROP_ALPHA * progress)});
}

static void hud_draw_modal_window(f32 box_x, f32 box_y, f32 box_w, f32 box_h)
{
    DrawRectangleRounded((Rectangle){box_x, box_y, box_w, box_h}, 0.05f, 4, (Color){22, 22, 22, 255});
    DrawRectangleRoundedLines((Rectangle){box_x, box_y, box_w, box_h}, 0.05f, 4, (Color){60, 60, 60, 255});
}

static void hud_draw_playlist_header(f32 box_x, f32 box_y)
{
    const char* header = "Playlist";
    text_draw(header, box_x + 24, box_y + 20, 16.0f, LIGHTGRAY);
}

static void hud_draw_playlist_items(Playlist* playlist, usize selected_index, const PlaylistLayout* l, usize start_idx)
{
    for (i32 i = 0; i < l->max_visible && (start_idx + i) < playlist->count; ++i) {
        usize idx = start_idx + i;
        Track* t = &playlist->items[idx];

        Rectangle item_rect = hud_playlist_item_rect(l, playlist->count, i);

        if (idx == selected_index) {
            DrawRectangleRounded(item_rect, 0.2f, 4, (Color){50, 50, 70, 255});
        }
        if (idx == playlist->current) {
            DrawRectangleRoundedLines(item_rect, 0.2f, 4, (Color){100, 200, 255, 255});
        }

        const char* display_name = t->title ? t->title : GetFileNameWithoutExt(t->path);
        char line_buf[512];
        snprintf(line_buf, sizeof(line_buf), "%zu. %s", idx + 1, display_name);

        Color text_col = (idx == playlist->current) ? (Color){100, 200, 255, 255} : WHITE;
        text_draw(line_buf, item_rect.x + 16, item_rect.y + 10, 16.0f, text_col);
    }
}

static void hud_draw_playlist_scrollbar(Playlist* playlist, const PlaylistLayout* l, usize start_idx)
{
    f32 sb_x = l->box_x + l->box_w - 24.0f;
    f32 sb_y = l->start_y;
    f32 sb_w = 6.0f;
    f32 sb_h = l->max_visible * l->item_height;

    DrawRectangleRounded((Rectangle){sb_x, sb_y, sb_w, sb_h}, 1.0f, 4, (Color){40, 40, 40, 255});

    f32 thumb_h = sb_h * ((f32)l->max_visible / playlist->count);
    if (thumb_h < 24.0f) thumb_h = 24.0f;

    f32 scroll_progress = (f32)start_idx / (playlist->count - l->max_visible);
    f32 thumb_y = sb_y + scroll_progress * (sb_h - thumb_h);

    DrawRectangleRounded((Rectangle){sb_x, thumb_y, sb_w, thumb_h}, 1.0f, 4, (Color){120, 120, 140, 255});
}

static void hud_draw_playlist(Playlist* playlist, usize selected_index, i32 w, i32 h, f32 progress)
{
    if (!playlist || playlist->count == 0) return;

    hud_draw_backdrop(w, h, progress);

    PlaylistLayout l = hud_playlist_layout(w, h, progress);

    hud_draw_modal_window(l.box_x, l.box_y, l.box_w, l.box_h);
    hud_draw_playlist_header(l.box_x, l.box_y);

    hud_playlist_clamp_scroll(playlist->count, l.max_visible);
    usize start_idx = playlist_scroll_offset;

    hud_draw_playlist_items(playlist, selected_index, &l, start_idx);

    if (playlist->count > (usize)l.max_visible) {
        hud_draw_playlist_scrollbar(playlist, &l, start_idx);
    }
}

bool hud_update_playlist(Playlist* playlist, void (*play_next_song)(void))
{
    PlaylistLayout l = hud_playlist_layout(GetScreenWidth(), GetScreenHeight(), 1.0f);

    if (input_key_pressed(KEY_GRAVE)) {
        show_playlist = !show_playlist;
        if (show_playlist) {
            playlist_selected_index = playlist->current;
            hud_playlist_ensure_visible(playlist->count, l.max_visible);
        }
    }

    if (show_playlist) {
        if (input_key_pressed(KEY_DOWN) || input_key_pressed(KEY_J)) {
            if (playlist->count > 0) {
                playlist_selected_index = (playlist_selected_index + 1) % playlist->count;
                hud_playlist_ensure_visible(playlist->count, l.max_visible);
            }
        }
        if (input_key_pressed(KEY_UP) || input_key_pressed(KEY_K)) {
            if (playlist->count > 0) {
                playlist_selected_index = (playlist_selected_index + playlist->count - 1) % playlist->count;
                hud_playlist_ensure_visible(playlist->count, l.max_visible);
            }
        }
        if (input_key_pressed(KEY_ENTER)) {
            playlist_set(playlist, playlist_selected_index);
            play_next_song();
            show_playlist = false;
        }
        if (input_key_pressed(KEY_ESCAPE)) {
            show_playlist = false;
        }

        f32 wheel = GetMouseWheelMove();
        if (wheel != 0 && playlist->count > 0) {
            usize max_scroll = hud_playlist_max_scroll(playlist->count, l.max_visible);
            if (wheel < 0) {
                playlist_scroll_offset += PLAYLIST_WHEEL_STEP;
                if (playlist_scroll_offset > max_scroll) playlist_scroll_offset = max_scroll;
            } else {
                playlist_scroll_offset = playlist_scroll_offset > PLAYLIST_WHEEL_STEP ? playlist_scroll_offset - PLAYLIST_WHEEL_STEP : 0;
            }
        }

        if (show_playlist && playlist_slide >= 1.0f && playlist->count > 0) {
            Vector2 mouse = GetMousePosition();
            Vector2 delta = GetMouseDelta();
            i32 slot = hud_playlist_slot_at(&l, playlist->count, mouse);

            if (slot >= 0) {
                usize hovered = playlist_scroll_offset + (usize)slot;

                if (delta.x != 0.0f || delta.y != 0.0f || wheel != 0) {
                    playlist_selected_index = hovered;
                }

                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    playlist_selected_index = hovered;
                    playlist_set(playlist, hovered);
                    play_next_song();
                    show_playlist = false;
                }
            }
        }
        return true;
    }
    return false;
}

void hud_render_playlist(Playlist* playlist, i32 w, i32 h)
{
    f32 target = show_playlist ? 1.0f : 0.0f;
    f32 step = PLAYLIST_SLIDE_SPEED * GetFrameTime();

    if (playlist_slide < target) {
        playlist_slide += step;
        if (playlist_slide > target) playlist_slide = target;
    } else if (playlist_slide > target) {
        playlist_slide -= step;
        if (playlist_slide < target) playlist_slide = target;
    }

    if (playlist_slide > 0.0f) {
        hud_draw_playlist(playlist, playlist_selected_index, w, h, playlist_slide);
    }
}
