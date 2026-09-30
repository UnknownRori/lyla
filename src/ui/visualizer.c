#include "ui.h"

Rectangle hud_visualizer_area(int w, int h)
{
    return (Rectangle){ 0, 0, w, h - HUD_TIMELINE_HEIGHT };
}
