#include "types.h"
#include <raylib.h>

void hud_draw_text_with_bg(
    const char *text, 
    f32 x, f32 y, f32 font_size, 
    Color text_color, 
    Color bg_color, 
    float pad_x, f32 pad_y, f32 roundness
);
