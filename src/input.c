#include "input.h"
#include "platform/wallpaper.h"

#define MOUSE_BUTTONS 3
#define KEY_COUNT     350

typedef struct {
    Vector2 mouse;
    bool    button[MOUSE_BUTTONS];
    bool    key[KEY_COUNT];
} InputState;

#define KEYBOARD_TOGGLE_KEY KEY_F2

static InputState s_cur, s_prev;
static bool       s_was_attached = false;
static bool       s_kb_enabled      = false;
static bool       s_toggle_was_down = false;

enum {
    VK_MB_LEFT = 0x01, VK_MB_RIGHT = 0x02, VK_MB_MIDDLE = 0x04,
    VK_K_BACK = 0x08, VK_K_TAB = 0x09, VK_K_RETURN = 0x0D,
    VK_K_ESCAPE = 0x1B, VK_K_SPACE = 0x20,
    VK_K_LEFT = 0x25, VK_K_UP = 0x26, VK_K_RIGHT = 0x27, VK_K_DOWN = 0x28,
    VK_K_F1 = 0x70,
    VK_K_LSHIFT = 0xA0, VK_K_RSHIFT = 0xA1,
    VK_K_LCTRL = 0xA2,  VK_K_RCTRL = 0xA3,
    VK_K_LALT = 0xA4,   VK_K_RALT = 0xA5,
    VK_K_GRAVE = 0xC0,
};

bool input_keyboard_active(void)
{
    return !IsWallpaperAttached() || s_kb_enabled;
}

static int mouse_button_to_vk(int b)
{
    switch (b) {
        case MOUSE_BUTTON_LEFT:   return VK_MB_LEFT;
        case MOUSE_BUTTON_RIGHT:  return VK_MB_RIGHT;
        case MOUSE_BUTTON_MIDDLE: return VK_MB_MIDDLE;
        default:                  return 0;
    }
}

static int key_to_vk(int key)
{
    if ((key >= KEY_A && key <= KEY_Z) || (key >= KEY_ZERO && key <= KEY_NINE))
        return key;
    if (key >= KEY_F1 && key <= KEY_F12)
        return VK_K_F1 + (key - KEY_F1);

    switch (key) {
        case KEY_GRAVE:         return VK_K_GRAVE;
        case KEY_ENTER:         return VK_K_RETURN;
        case KEY_KP_ENTER:      return VK_K_RETURN;
        case KEY_SPACE:         return VK_K_SPACE;
        case KEY_ESCAPE:        return VK_K_ESCAPE;
        case KEY_TAB:           return VK_K_TAB;
        case KEY_BACKSPACE:     return VK_K_BACK;
        case KEY_LEFT:          return VK_K_LEFT;
        case KEY_RIGHT:         return VK_K_RIGHT;
        case KEY_UP:            return VK_K_UP;
        case KEY_DOWN:          return VK_K_DOWN;
        case KEY_LEFT_SHIFT:    return VK_K_LSHIFT;
        case KEY_RIGHT_SHIFT:   return VK_K_RSHIFT;
        case KEY_LEFT_CONTROL:  return VK_K_LCTRL;
        case KEY_RIGHT_CONTROL: return VK_K_RCTRL;
        case KEY_LEFT_ALT:      return VK_K_LALT;
        case KEY_RIGHT_ALT:     return VK_K_RALT;
        default:                return 0;
    }
}

static void sample_wallpaper(InputState *s, bool keyboard)
{
    int x, y, left;
    WallpaperMouse(&x, &y, &left);
    s->mouse = (Vector2){ (float)x, (float)y };

    for (int b = 0; b < MOUSE_BUTTONS; b++) {
        int vk = mouse_button_to_vk(b);
        s->button[b] = vk && WallpaperKeyDown(vk);
    }

    for (int k = 0; k < KEY_COUNT; k++) {
        if (!keyboard || k == KEYBOARD_TOGGLE_KEY) {
            s->key[k] = false;
            continue;
        }
        int vk = key_to_vk(k);
        s->key[k] = vk && WallpaperKeyDown(vk);
    }
}

static void sample_raylib(InputState *s)
{
    s->mouse = GetMousePosition();
    for (int b = 0; b < MOUSE_BUTTONS; b++) s->button[b] = IsMouseButtonDown(b);
    for (int k = 0; k < KEY_COUNT; k++)     s->key[k]    = IsKeyDown(k);
}


static void update_keyboard_toggle(void)
{
    bool down = WallpaperKeyDown(key_to_vk(KEYBOARD_TOGGLE_KEY));
    if (down && !s_toggle_was_down)
        s_kb_enabled = !s_kb_enabled;
    s_toggle_was_down = down;
}

void input_update(void)
{
    s_prev = s_cur;
    bool attached = IsWallpaperAttached();
    bool switched = (attached != s_was_attached);

    if (switched) {
        s_kb_enabled = false;
        s_toggle_was_down = attached && WallpaperKeyDown(key_to_vk(KEYBOARD_TOGGLE_KEY));
    }

    if (attached) {
        update_keyboard_toggle();
        sample_wallpaper(&s_cur, s_kb_enabled);
    } else {
        sample_raylib(&s_cur);
    }

    if (switched) {
        s_prev = s_cur;
        s_was_attached = attached;
    }
}


void input_init(void)
{
    s_cur = (InputState){ 0 };
    s_prev = s_cur;
    s_was_attached = IsWallpaperAttached();
}

Vector2 input_mouse_position(void) { return s_cur.mouse; }

static bool valid_button(int b) { return b >= 0 && b < MOUSE_BUTTONS; }
static bool valid_key(int k)    { return k >= 0 && k < KEY_COUNT; }

bool input_mouse_down(int b)     { return valid_button(b) && s_cur.button[b]; }
bool input_mouse_pressed(int b)  { return valid_button(b) &&  s_cur.button[b] && !s_prev.button[b]; }
bool input_mouse_released(int b) { return valid_button(b) && !s_cur.button[b] &&  s_prev.button[b]; }

bool input_key_down(int k)       { return valid_key(k) && s_cur.key[k]; }
bool input_key_pressed(int k)    { return valid_key(k) &&  s_cur.key[k] && !s_prev.key[k]; }
bool input_key_released(int k)   { return valid_key(k) && !s_cur.key[k] &&  s_prev.key[k]; }
