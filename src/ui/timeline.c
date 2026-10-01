#include "platform/wallpaper.h"
#include "ui.h"
#include "input.h"

void hud_draw_timeline(int w, int h)
{
    Rectangle bar = { 0, h - HUD_TIMELINE_HEIGHT, w, HUD_TIMELINE_HEIGHT };

    if (input_mouse_pressed(MOUSE_BUTTON_LEFT) &&
        CheckCollisionPointRec(input_mouse_position(), bar) && !IsWallpaperAttached()) {
        player_seek_fraction(GetMousePosition().x/w);
    }

    DrawRectangleRec(bar, ColorBrightness(GetColor(HUD_BACKGROUND_COLOR), -0.3f));
    DrawRectangle(0, bar.y, (int)(player_progress()*w), HUD_TIMELINE_HEIGHT, ColorFromHSV(225, 0.75f, 0.8f));
}
