#include "ui.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "raylib.h"
#include "input.h"
#include "text.h"
#include "types.h"

#define CONFIRM_ANIM_SPEED     8.0f
#define CONFIRM_BACKDROP_ALPHA 170.0f
#define CONFIRM_MAX_W          460.0f
#define CONFIRM_H              180.0f
#define CONFIRM_PAD            20.0f
#define CONFIRM_BTN_W          124.0f
#define CONFIRM_BTN_H          36.0f
#define CONFIRM_BTN_GAP        12.0f
#define CONFIRM_BUTTONS        3

typedef struct {
    f32 x, y, w, h;
    Rectangle btn[CONFIRM_BUTTONS];
} ConfirmLayout;

static bool open_state = false;
static bool armed = false;
static f32  anim = 0.0f;
static i32  focus = 0;
static Vector2 last_mouse = {0};

static char title_buf[96];
static char message_buf[256];
static char label_buf[CONFIRM_BUTTONS][32];

static void copy_str(char* dst, usize cap, const char* src)
{
    snprintf(dst, cap, "%s", src ? src : "");
}

static Color fade(Color c)
{
    return ColorAlpha(c, anim);
}

static ConfirmLayout confirm_layout(i32 w, i32 h, f32 progress)
{
    ConfirmLayout l;
    f32 inv = 1.0f - progress;
    f32 eased = 1.0f - inv * inv * inv;

    l.w = fminf(CONFIRM_MAX_W, w * 0.92f);
    l.h = CONFIRM_H;
    l.x = (w - l.w) * 0.5f;
    l.y = (h - l.h) * 0.5f + (1.0f - eased) * 24.0f;

    f32 bw = (l.w - 2.0f * CONFIRM_PAD - (CONFIRM_BUTTONS - 1) * CONFIRM_BTN_GAP) / CONFIRM_BUTTONS;
    if (bw > CONFIRM_BTN_W) bw = CONFIRM_BTN_W;

    f32 total = bw * CONFIRM_BUTTONS + CONFIRM_BTN_GAP * (CONFIRM_BUTTONS - 1);
    f32 bx = l.x + (l.w - total) * 0.5f;
    f32 by = l.y + l.h - CONFIRM_PAD - CONFIRM_BTN_H;

    for (i32 i = 0; i < CONFIRM_BUTTONS; i++) {
        l.btn[i] = (Rectangle){ bx + i * (bw + CONFIRM_BTN_GAP), by, bw, CONFIRM_BTN_H };
    }
    return l;
}

void hud_confirm_open(const char* title, const char* message,
                      const char* save_label,
                      const char* discard_label,
                      const char* cancel_label)
{
    copy_str(title_buf, sizeof(title_buf), title);
    copy_str(message_buf, sizeof(message_buf), message);
    copy_str(label_buf[0], sizeof(label_buf[0]), save_label);
    copy_str(label_buf[1], sizeof(label_buf[1]), discard_label);
    copy_str(label_buf[2], sizeof(label_buf[2]), cancel_label);

    open_state = true;
    armed = false;
    focus = 0;
    last_mouse = input_mouse_position();
}

bool hud_confirm_is_open(void)
{
    return open_state;
}

static ConfirmResult result_for(i32 index)
{
    switch (index) {
        case 0:  return CONFIRM_SAVE;
        case 1:  return CONFIRM_DISCARD;
        default: return CONFIRM_CANCEL;
    }
}

static ConfirmResult finish(ConfirmResult r)
{
    open_state = false;
    return r;
}

ConfirmResult hud_confirm_update(void)
{
    if (!open_state) return CONFIRM_NONE;

    if (!armed) {
        armed = true;
        last_mouse = input_mouse_position();
        return CONFIRM_NONE;
    }

    ConfirmLayout l = confirm_layout(GetScreenWidth(), GetScreenHeight(), 1.0f);

    if (input_key_repeat(KEY_RIGHT) || input_key_repeat(KEY_L) || input_key_pressed(KEY_TAB)) {
        focus = (focus + 1) % CONFIRM_BUTTONS;
    }
    if (input_key_repeat(KEY_LEFT) || input_key_repeat(KEY_H)) {
        focus = (focus + CONFIRM_BUTTONS - 1) % CONFIRM_BUTTONS;
    }

    if (input_key_pressed(KEY_S)) return finish(CONFIRM_SAVE);
    if (input_key_pressed(KEY_D)) return finish(CONFIRM_DISCARD);
    if (input_key_pressed(KEY_ESCAPE)) return finish(CONFIRM_CANCEL);
    if (input_key_pressed(KEY_ENTER) || input_key_pressed(KEY_SPACE)) {
        return finish(result_for(focus));
    }

    Vector2 mouse = input_mouse_position();
    bool moved = mouse.x != last_mouse.x || mouse.y != last_mouse.y;
    last_mouse = mouse;

    i32 hovered = -1;
    for (i32 i = 0; i < CONFIRM_BUTTONS; i++) {
        if (CheckCollisionPointRec(mouse, l.btn[i])) { hovered = i; break; }
    }
    if (hovered >= 0 && moved) focus = hovered;
    if (hovered >= 0 && input_mouse_pressed(MOUSE_BUTTON_LEFT)) {
        return finish(result_for(hovered));
    }

    return CONFIRM_NONE;
}

static void draw_message_lines(const ConfirmLayout* l)
{
    const f32 size = 15.0f;
    char line[128];
    const char* p = message_buf;
    f32 y = l->y + 64.0f;

    while (*p) {
        const char* nl = strchr(p, '\n');
        usize n = nl ? (usize)(nl - p) : strlen(p);
        if (n >= sizeof(line)) n = sizeof(line) - 1;
        memcpy(line, p, n);
        line[n] = '\0';

        f32 tw = text_width(line, size);
        text_draw(line, l->x + (l->w - tw) * 0.5f, y, size, fade(LIGHTGRAY));
        y += 22.0f;

        p += n;
        if (*p == '\n') p++;
    }
}

void hud_confirm_render(int w, int h)
{
    f32 target = open_state ? 1.0f : 0.0f;
    f32 step = CONFIRM_ANIM_SPEED * GetFrameTime();

    if (anim < target) {
        anim += step;
        if (anim > target) anim = target;
    } else if (anim > target) {
        anim -= step;
        if (anim < target) anim = target;
    }

    if (anim <= 0.0f) return;

    ConfirmLayout l = confirm_layout(w, h, anim);

    // backdrop + window
    DrawRectangle(0, 0, w, h, (Color){0, 0, 0, (u8)(CONFIRM_BACKDROP_ALPHA * anim)});
    DrawRectangleRounded((Rectangle){l.x, l.y, l.w, l.h}, 0.06f, 6, fade((Color){22, 22, 22, 255}));
    DrawRectangleRoundedLines((Rectangle){l.x, l.y, l.w, l.h}, 0.06f, 6, fade((Color){60, 60, 60, 255}));

    // title + message
    f32 title_size = 18.0f;
    f32 tw = text_width(title_buf, title_size);
    text_draw(title_buf, l.x + (l.w - tw) * 0.5f, l.y + 24.0f, title_size, fade(WHITE));
    draw_message_lines(&l);

    const Color label_colors[CONFIRM_BUTTONS] = {
        {100, 200, 255, 255},
        {235, 110, 110, 255},
        {200, 200, 200, 255},
    };

    for (i32 i = 0; i < CONFIRM_BUTTONS; i++) {
        Rectangle r = l.btn[i];
        bool focused = (i == focus) && open_state;

        DrawRectangleRounded(r, 0.25f, 6, fade(focused ? (Color){50, 50, 70, 255} : (Color){34, 34, 34, 255}));
        DrawRectangleRoundedLines(r, 0.25f, 6, fade(focused ? label_colors[i] : (Color){70, 70, 70, 255}));

        f32 lw = text_width(label_buf[i], 16.0f);
        text_draw(label_buf[i], r.x + (r.width - lw) * 0.5f, r.y + (r.height - 16.0f) * 0.5f, 16.0f, fade(label_colors[i]));
    }
}
