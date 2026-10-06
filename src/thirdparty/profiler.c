#include <raylib.h>
#include "profiler.h"
#include "input.h"
#include "prof.h"

#ifdef WITH_PROFILER
// it expect that this thing exist from text.c
extern unsigned char __resources_FOT_Yuruka_Std_ttf[];
extern int __resources_FOT_Yuruka_Std_ttf_len;

#define FONT_SIZE 16
static Font _font;
static bool display = false;

static void print_text(float x, float y, char *str, float r, float g, float b, float a)
{
    // TraceLog(LOG_INFO, "print_text(%f, %f, %s, ...)", x, y, str);
    DrawTextEx(_font, str, (Vector2){x, y}, FONT_SIZE, 2, ColorFromNormalized((Vector4){r, g, b, a}));
}

static float text_width(char *str)
{
    return MeasureTextEx(_font, str, FONT_SIZE, 2).x;
}

static void draw_rectangle(float x0, float y0, float x1, float y1, float r, float g, float b, float a)
{
    DrawRectangle(x0, y0, x1 - x0, y1 - y0, ColorFromNormalized((Vector4){r, g, b, a}));
}

static void draw_line(float x0, float y0, float x1, float y1, float r, float g, float b, float a)
{
    DrawLine(x0, y0, x1, y1, ColorFromNormalized((Vector4){r, g, b, a}));
}

void profiler_init(void)
{
    _font = LoadFontFromMemory(".ttf", __resources_FOT_Yuruka_Std_ttf, __resources_FOT_Yuruka_Std_ttf_len, 32, NULL, 0);
    TraceLog(LOG_INFO, "font profiler loaded");
}

void profiler_draw(int w, int h)
{
    if (display) {
        Prof_draw(0, 0, w, h, 16, 2, print_text, text_width, draw_rectangle);
        Prof_draw_graph(0, 0, 2.0, 8.0, draw_line);
    }
}

void profiler_update()
{
    Prof_update(display);
    if (input_key_pressed(KEY_F11)) display = !display;
    if (IsKeyPressed(KEY_Q) && IsKeyDown(KEY_LEFT_CONTROL)) {
        Prof_move_cursor(1);
    }
    if (IsKeyPressed(KEY_A) && IsKeyDown(KEY_LEFT_CONTROL)) {
        Prof_move_cursor(-1);
    }
    if (IsKeyPressed(KEY_Z)) {
        Prof_select();
    }
    if (IsKeyPressed(KEY_X)) {
        Prof_select_parent();
    }
}
#else
void profiler_init(void) { }
void profiler_draw(int w, int h) { }
void profiler_update(int) { }
#endif // WITH_PROFILER

