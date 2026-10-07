#include "text.h"
#include "assets/font.c"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define TEXT_FONT_SIZE 32
#define TEXT_CP_LIMIT  0x110000

static Font font;
static bool text_ready = false;

static uint8_t cp_have[TEXT_CP_LIMIT / 8];
static uint8_t cp_want[TEXT_CP_LIMIT / 8];
static bool cp_dirty = false;

static inline bool bit_get(const uint8_t* b, int cp) { return (b[cp >> 3] >> (cp & 7)) & 1u; }
static inline void bit_set(uint8_t* b, int cp)       { b[cp >> 3] |= (uint8_t)(1u << (cp & 7)); }

static bool font_is_custom(void)
{
    return font.texture.id != 0 && font.texture.id != GetFontDefault().texture.id;
}

static void want_cp(int cp)
{
    if (cp < 32 || cp == 127 || cp >= TEXT_CP_LIMIT) return;
    if (!bit_get(cp_want, cp)) {
        bit_set(cp_want, cp);
        cp_dirty = true;
    }
}

static void text_setup_once(void)
{
    if (text_ready) return;
    text_ready = true;
    font = GetFontDefault();

    for (int c = 32; c < 127; ++c)     want_cp(c);
    for (int c = 0xA0; c <= 0x17F; ++c) want_cp(c);
}

void text_init(void)
{
    text_setup_once();
    text_flush();
}

void text_shutdown(void)
{
    if (font_is_custom()) UnloadFont(font);
    font = (Font){0};
    text_ready = false;
    cp_dirty = false;
    memset(cp_have, 0, sizeof(cp_have));
    memset(cp_want, 0, sizeof(cp_want));
}

void text_collect(const char *utf8)
{
    text_setup_once();
    if (!utf8) return;

    while (*utf8) {
        int size = 0;
        int cp = GetCodepointNext(utf8, &size);
        if (size <= 0) break;
        want_cp(cp);
        utf8 += size;
    }
}

void text_flush(void)
{
    text_setup_once();
    if (!cp_dirty) return;
    cp_dirty = false;

    int count = 0;
    for (int i = 0; i < TEXT_CP_LIMIT / 8; ++i) {
        if (cp_want[i] == 0) continue;
        for (int b = 0; b < 8; ++b) if ((cp_want[i] >> b) & 1u) count++;
    }

    int *cps = malloc((unsigned int)count * sizeof(int));
    int n = 0;
    for (int i = 0; i < TEXT_CP_LIMIT / 8; ++i) {
        if (cp_want[i] == 0) continue;
        for (int b = 0; b < 8; ++b) if ((cp_want[i] >> b) & 1u) cps[n++] = i * 8 + b;
    }

    Font f = LoadFontFromMemory(".ttf", __resources_FOT_Yuruka_Std_ttf, __resources_FOT_Yuruka_Std_ttf_len, TEXT_FONT_SIZE, cps, n);
    free(cps);

    if (f.texture.id != 0 && f.glyphCount > 0) {
        if (font_is_custom()) UnloadFont(font);
        font = f;
        SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
        memcpy(cp_have, cp_want, sizeof(cp_have));
    } else {
        if (f.texture.id != 0) UnloadFont(f);
        memcpy(cp_want, cp_have, sizeof(cp_want));
    }
}

void text_prepare(const char *utf8)
{
    text_collect(utf8);
    text_flush();
}

void text_draw(const char *utf8, float x, float y, float size, Color color)
{
    DrawTextEx(font, utf8, (Vector2){ x, y }, size, 1, color);
}

float text_width(const char *utf8, float size)
{
    return MeasureTextEx(font, utf8, size, 1).x;
}
