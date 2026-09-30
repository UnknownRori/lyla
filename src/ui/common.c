#include "ui.h"
#include "text.h"
#include "./common.h"
void hud_draw_text_with_bg(
    const char *text, 
    f32 x, f32 y, f32 font_size, 
    Color text_color, 
    Color bg_color, 
    float pad_x, f32 pad_y, f32 roundness
)
{
    float tw = text_width(text, font_size);
    Rectangle rect = {
        x - pad_x,
        y - pad_y,
        tw + (pad_x * 2.0f),
        font_size + (pad_y * 2.0f)
    };
    DrawRectangleRounded(rect, roundness, 4, bg_color);
    text_draw(text, x, y, font_size, text_color);
}
