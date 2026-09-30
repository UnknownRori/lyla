#include <raylib.h>
#include "types.h"

static bool s_dragged = false;
static bool s_resizing = false;
static Vector2 s_pan_offset = { 0 };
static Vector2 s_window_pos;
static int s_initial_width = 0;
static int s_initial_height = 0;

void input_init()
{
    s_window_pos = GetWindowPosition();
}

void input_default_update()
{
    Vector2 curr_mos = GetMousePosition();

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        s_dragged = true;
        s_pan_offset = curr_mos;
        s_window_pos = GetWindowPosition();

        if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
            s_resizing = true;
            s_initial_width = GetScreenWidth();
            s_initial_height = GetScreenHeight();
        } else {
            s_resizing = false;
        }
    }

    if (s_dragged) {
        if (s_resizing) {
            float dx = curr_mos.x - s_pan_offset.x;
            float dy = curr_mos.y - s_pan_offset.y;

            int new_width = s_initial_width + (int)dx;
            int new_height = s_initial_height + (int)dy;

            if (new_width < 64) new_width = 64;
            if (new_height < 64) new_height = 64;

            SetWindowSize(new_width, new_height);
        } 
        else {
            s_window_pos.x += curr_mos.x - s_pan_offset.x;
            s_window_pos.y += curr_mos.y - s_pan_offset.y;
            
            SetWindowPosition((int)s_window_pos.x, (int)s_window_pos.y);
        }

        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            s_dragged = false;
            s_resizing = false;
        }
    }
}
