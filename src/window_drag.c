#include <raylib.h>
#include "platform/wallpaper.h"
#include "types.h"

static bool s_dragged = false;
static bool s_resizing = false;
static Vector2 s_pan_offset = { 0 };
static Vector2 s_window_pos;
static i32 s_initial_width = 0;
static i32 s_initial_height = 0;

void window_drag_init()
{
    s_window_pos = GetWindowPosition();
    s_initial_width = GetScreenHeight();
    s_initial_height = GetScreenHeight();
}

bool window_is_resizing()
{
    return s_resizing;
}

void window_drag_update(int w, int h)
{
    if (IsWallpaperAttached()) return;

    Vector2 curr_mos = GetMousePosition();

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        s_dragged = true;
        s_pan_offset = curr_mos;
        s_window_pos = GetWindowPosition();

        if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
            s_resizing = true;
            s_initial_width = w;
            s_initial_height = h;
        } else {
            s_resizing = false;
        }
    }

    if (s_dragged) {
        if (s_resizing) {
            f32 dx = curr_mos.x - s_pan_offset.x;
            f32 dy = curr_mos.y - s_pan_offset.y;

            i32 new_width = s_initial_width + (i32)dx;
            i32 new_height = s_initial_height + (i32)dy;

            if (new_width < 64) new_width = 64;
            if (new_height < 64) new_height = 64;

            SetWindowSize(new_width, new_height);
        } 
        else {
            s_window_pos.x += curr_mos.x - s_pan_offset.x;
            s_window_pos.y += curr_mos.y - s_pan_offset.y;
            
            SetWindowPosition((i32)s_window_pos.x, (i32)s_window_pos.y);
        }

        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            s_dragged = false;
            s_resizing = false;
        }
    }
}
