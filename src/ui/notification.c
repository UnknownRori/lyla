#include "ui.h"
#include "text.h"

#define ERROR_POPUP_SECS 2.0

static double error_until = 0;

void hud_draw_idle(int w, int h)
{
    const char *label = "Drag & Drop a music file";
    float size = 40;
    text_draw(label, w/2 - text_width(label, size)/2, h/2 - size/2, size, WHITE);
}

void hud_notify_load_error(void)
{
    error_until = GetTime() + ERROR_POPUP_SECS;
}

void hud_draw_notifications(int w, int h)
{
    if (GetTime() >= error_until) return;

    const char *msg = "Could not load file";
    Rectangle box = { w - 270, h - 100, 250, 60 };
    DrawRectangleRounded(box, 0.3f, 20, ColorFromHSV(0, 0.75f, 0.8f));
    text_draw(msg, box.x + box.width/2 - text_width(msg, 14)/2, box.y + 19, 14, WHITE);
}
