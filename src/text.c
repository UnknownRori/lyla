#include "text.h"
#include "assets/font.c"
#include <stdbool.h>
#include <stddef.h>

#define TEXT_FONT_SIZE 32

static Font font;

static bool font_is_custom(void)
{
    return font.texture.id != 0 && font.texture.id != GetFontDefault().texture.id;
}

void text_init(void)
{
    text_prepare("");
}

void text_shutdown(void)
{
    if (font_is_custom()) UnloadFont(font);
    font = (Font){0};
}

void text_prepare(const char *utf8)
{
    if (font_is_custom()) UnloadFont(font);
    font = GetFontDefault();

    int text_count = 0;
    int *text_cps = LoadCodepoints(utf8, &text_count);

    int *cps = MemAlloc((95 + text_count) * sizeof(int));
    int n = 0;
    for (int c = 32; c < 127; ++c) cps[n++] = c;
    for (int i = 0; i < text_count; ++i) {
        bool dup = false;
        for (int j = 0; j < n; ++j) {
            if (cps[j] == text_cps[i]) { dup = true; break; }
        }
        if (!dup) cps[n++] = text_cps[i];
    }
    UnloadCodepoints(text_cps);

    // Load font from memory instead of file
    Font f = LoadFontFromMemory(".ttf", __resources_FOT_Yuruka_Std_ttf, __resources_FOT_Yuruka_Std_ttf_len, TEXT_FONT_SIZE, cps, n);
    MemFree(cps);

    if (f.texture.id != 0 && f.glyphCount > 0) {
        font = f;
        SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
    }
}

void text_draw(const char *utf8, float x, float y, float size, Color color)
{
    DrawTextEx(font, utf8, (Vector2){ x, y }, size, 1, color);
}

float text_width(const char *utf8, float size)
{
    return MeasureTextEx(font, utf8, size, 1).x;
}
