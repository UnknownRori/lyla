#include <raylib.h>
#include "music/player.h"
#include "ui.h"
#include "utils.h"

void hud_overlay_pause(int w, int h)
{
    if (!player_paused()) return;
    DrawRectangleLinesEx(RECT(0, 0, w, h), 2.5, BLUE);
}

void hud_overlay_mode_on(int w, int h)
{
    DrawRectangleLinesEx(RECT(0, 0, w, h), 2.5, RED);
}
