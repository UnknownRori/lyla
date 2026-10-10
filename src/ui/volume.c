#include <math.h>
#include <stdio.h>
#include <raylib.h>
#include "types.h"
#include <rstb_common.h>
#include "ui.h"
#include "text.h"
#include "utils.h"
#include "assets/volume.c"

#define VOLUME_BLOCKS         16
#define VOLUME_BLOCK_W        9.0f
#define VOLUME_BLOCK_H        20.0f
#define VOLUME_BLOCK_GAP      4.0f
#define VOLUME_ICON_SIZE      28.0f
#define VOLUME_ICON_TEX       32
#define VOLUME_ICON_CHECKS    4
#define VOLUME_SIGN_SIZE      12.0f
#define VOLUME_SIGN_THICK     3.0f
#define VOLUME_PAD            16.0f
#define VOLUME_GAP            12.0f
#define VOLUME_TEXT_SIZE      14.0f
#define VOLUME_TEXT_W         44.0f
#define VOLUME_TOP_MARGIN     24.0f
#define VOLUME_HOLD_TIME      1.2f
#define VOLUME_SLIDE_SPEED    7.0f
#define VOLUME_EPSILON        0.0005f
#define VOLUME_FLASH_DECAY    3.0f

typedef struct {
    f32 x, y, w, h;
    f32 scale;
    Rectangle icon, minus, bar, plus, text;
} VolumeLayout;

static f32 volume_slide = 0.0f;
static f32 volume_timer = 0.0f;
static f32 volume_last = 0.0f;
static f32 volume_flash = 0.0f;
static i32 volume_dir = 0;
static bool volume_ready = false;
static Texture2D volume_icon = {0};

void hud_volume_init(void)
{
    // TODO : Texture atlas handler
    Image img = LoadImageFromMemory(".png", __resources_volume_png, __resources_volume_png_len);
    volume_icon = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureFilter(volume_icon, TEXTURE_FILTER_BILINEAR);

    volume_slide = 0.0f;
    volume_timer = 0.0f;
    volume_flash = 0.0f;
    volume_dir = 0;
    volume_ready = false;
}

void hud_volume_shutdown(void)
{
    if (volume_icon.id != 0) UnloadTexture(volume_icon);
    volume_icon = (Texture2D){0};
}

static Color vol_color(u8 r, u8 g, u8 b, f32 a)
{
    f32 k = fmaxf(0.0f, fminf(1.0f, a));
    return (Color){ r, g, b, (u8)(255.0f*k) };
}

static Color vol_mix(Color a, Color b, f32 t, f32 alpha)
{
    t = fmaxf(0.0f, fminf(1.0f, t));
    return vol_color(
        (u8)(a.r + (b.r - a.r)*t),
        (u8)(a.g + (b.g - a.g)*t),
        (u8)(a.b + (b.b - a.b)*t),
        alpha
    );
}

static f32 hud_volume_base_width(void)
{
    f32 bar_w = VOLUME_BLOCKS*VOLUME_BLOCK_W + (VOLUME_BLOCKS - 1)*VOLUME_BLOCK_GAP;
    return VOLUME_PAD*2.0f
         + VOLUME_ICON_SIZE + VOLUME_GAP
         + VOLUME_SIGN_SIZE + VOLUME_GAP
         + bar_w + VOLUME_GAP
         + VOLUME_SIGN_SIZE + VOLUME_GAP
         + VOLUME_TEXT_W;
}

static VolumeLayout hud_volume_layout(i32 w, f32 progress)
{
    VolumeLayout l;

    f32 base_w = hud_volume_base_width();
    f32 base_h = VOLUME_PAD*2.0f + VOLUME_ICON_SIZE;
    f32 s = fminf(1.0f, (w*0.92f)/base_w);

    l.scale = s;
    l.w = base_w*s;
    l.h = base_h*s;
    l.x = (w - l.w)*0.5f;

    f32 target_y = VOLUME_TOP_MARGIN*s;
    f32 inv = 1.0f - progress;
    f32 eased = 1.0f - inv*inv*inv;
    l.y = -l.h + (target_y + l.h)*eased;

    f32 cy = l.y + l.h*0.5f;
    f32 x = l.x + VOLUME_PAD*s;

    l.icon = RECT(x, cy - VOLUME_ICON_SIZE*s*0.5f, VOLUME_ICON_SIZE*s, VOLUME_ICON_SIZE*s);
    x += (VOLUME_ICON_SIZE + VOLUME_GAP)*s;

    l.minus = RECT(x, cy - VOLUME_SIGN_SIZE*s*0.5f, VOLUME_SIGN_SIZE*s, VOLUME_SIGN_SIZE*s);
    x += (VOLUME_SIGN_SIZE + VOLUME_GAP)*s;

    f32 bar_w = (VOLUME_BLOCKS*VOLUME_BLOCK_W + (VOLUME_BLOCKS - 1)*VOLUME_BLOCK_GAP)*s;
    l.bar = RECT(x, cy - VOLUME_BLOCK_H*s*0.5f, bar_w, VOLUME_BLOCK_H*s);
    x += bar_w + VOLUME_GAP*s;

    l.plus = RECT(x, cy - VOLUME_SIGN_SIZE*s*0.5f, VOLUME_SIGN_SIZE*s, VOLUME_SIGN_SIZE*s);
    x += (VOLUME_SIGN_SIZE + VOLUME_GAP)*s;

    l.text = RECT(x, cy - VOLUME_TEXT_SIZE*s*0.5f, VOLUME_TEXT_W*s, VOLUME_TEXT_SIZE*s);
    return l;
}

static void hud_draw_volume_icon(Rectangle r, f32 alpha)
{
    if (!IsTextureValid(volume_icon)) return;
    DrawTexturePro(
        volume_icon,
        RECT( 0, 0, (f32)volume_icon.width, (f32)volume_icon.height),
        r,
        (Vector2){ 0, 0 },
        0.0f,
        ColorAlpha(WHITE, alpha)
    );
}

static void hud_draw_volume_sign(Rectangle r, bool plus, f32 glow, f32 alpha, f32 scale)
{
    f32 grow = 1.0f + 0.3f*glow;
    f32 size = r.width*grow;
    f32 thick = VOLUME_SIGN_THICK*scale*grow;
    f32 cx = r.x + r.width*0.5f;
    f32 cy = r.y + r.height*0.5f;

    Color c = vol_mix((Color){ 110, 110, 125, 255 }, WHITE, glow, alpha);

    DrawRectangleRec(RECT(cx - size*0.5f, cy - thick*0.5f, size, thick), c);
    if (plus) {
        DrawRectangleRec(RECT(cx - thick*0.5f, cy - size*0.5f, thick, size ), c);
    }
}

static void hud_draw_volume_bar(Rectangle r, f32 volume, f32 alpha, f32 scale)
{
    f32 bw = VOLUME_BLOCK_W*scale;
    f32 gap = VOLUME_BLOCK_GAP*scale;
    Color on = vol_mix((Color){ 100, 200, 255, 255 }, WHITE, 0.25f*volume_flash, alpha);
    Color off = vol_color(48, 48, 58, alpha);

    for (i32 i = 0; i < VOLUME_BLOCKS; i++) {
        f32 x = r.x + i*(bw + gap);
        DrawRectangleRec((Rectangle){ x, r.y, bw, r.height }, off);

        f32 fill = fmaxf(0.0f, fminf(1.0f, volume*VOLUME_BLOCKS - i));
        if (fill <= 0.0f) continue;

        f32 fh = r.height*fill;
        DrawRectangleRec((Rectangle){ x, r.y + r.height - fh, bw, fh }, on);
    }
}

static void hud_draw_volume_percent(Rectangle r, f32 volume, f32 alpha, f32 scale)
{
    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", (i32)(volume*100.0f + 0.5f));

    f32 size = VOLUME_TEXT_SIZE*scale;
    f32 tw = text_width(buf, size);
    text_draw(buf, r.x + r.width - tw, r.y, size, vol_color(200, 200, 210, alpha));
}

void hud_volume_render(f32 volume, int w, int h, f32 dt)
{
    UNUSED(h);
    volume = fmaxf(0.0f, fminf(1.0f, volume));

    if (!volume_ready) {
        volume_ready = true;
        volume_last = volume;
    }

    f32 diff = volume - volume_last;
    if (fabsf(diff) > VOLUME_EPSILON) {
        volume_dir = diff > 0.0f ? 1 : -1;
        volume_flash = 1.0f;
        volume_timer = VOLUME_HOLD_TIME;
        volume_last = volume;
    }

    if (volume_timer > 0.0f) volume_timer -= dt;
    if (volume_flash > 0.0f) volume_flash = fmaxf(0.0f, volume_flash - VOLUME_FLASH_DECAY*dt);

    f32 target = volume_timer > 0.0f ? 1.0f : 0.0f;
    f32 step = VOLUME_SLIDE_SPEED*dt;
    if (volume_slide < target) {
        volume_slide += step;
        if (volume_slide > target) volume_slide = target;
    } else if (volume_slide > target) {
        volume_slide -= step;
        if (volume_slide < target) volume_slide = target;
    }

    if (volume_slide <= 0.0f) return;

    VolumeLayout l = hud_volume_layout(w, volume_slide);
    f32 a = volume_slide;

    Rectangle box = { l.x, l.y, l.w, l.h };
    DrawRectangleRounded(box, 0.35f, 8, vol_color(22, 22, 22, a*0.96f));
    DrawRectangleRoundedLines(box, 0.35f, 8, vol_color(60, 60, 60, a));

    hud_draw_volume_icon(l.icon, a);
    hud_draw_volume_sign(l.minus, false, volume_dir < 0 ? volume_flash : 0.0f, a, l.scale);
    hud_draw_volume_bar(l.bar, volume, a, l.scale);
    hud_draw_volume_sign(l.plus, true, volume_dir > 0 ? volume_flash : 0.0f, a, l.scale);
    hud_draw_volume_percent(l.text, volume, a, l.scale);
}
