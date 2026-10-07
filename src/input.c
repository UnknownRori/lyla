#include "input.h"
#include "platform/wallpaper.h"

#define MOUSE_BUTTONS 3
#define KEY_COUNT     350

#define KEY_REPEAT_DELAY    0.40f
#define KEY_REPEAT_INTERVAL 0.04f
#define CHAR_QUEUE_SIZE     32

typedef struct {
    Vector2 mouse;
    bool    button[MOUSE_BUTTONS];
    bool    key[KEY_COUNT];
} InputState;

#define KEYBOARD_TOGGLE_KEY KEY_F3

static InputState s_cur, s_prev;
static bool       s_was_attached = false;
static bool       s_kb_enabled      = false;
static bool       s_toggle_was_down = false;

static float s_hold[KEY_COUNT];
static float s_next_repeat[KEY_COUNT];
static bool  s_repeat_fired[KEY_COUNT];

static int s_chars[CHAR_QUEUE_SIZE];
static int s_char_head  = 0;
static int s_char_count = 0;

enum {
    VK_MB_LEFT = 0x01, VK_MB_RIGHT = 0x02, VK_MB_MIDDLE = 0x04,
    VK_K_BACK = 0x08, VK_K_TAB = 0x09, VK_K_RETURN = 0x0D,
    VK_K_CAPS = 0x14,
    VK_K_ESCAPE = 0x1B, VK_K_SPACE = 0x20,
    VK_K_PRIOR = 0x21, VK_K_NEXT = 0x22, VK_K_END = 0x23, VK_K_HOME = 0x24,
    VK_K_LEFT = 0x25, VK_K_UP = 0x26, VK_K_RIGHT = 0x27, VK_K_DOWN = 0x28,
    VK_K_INSERT = 0x2D, VK_K_DELETE = 0x2E,
    VK_K_LWIN = 0x5B, VK_K_RWIN = 0x5C,
    VK_K_NUMPAD0 = 0x60,
    VK_K_MULTIPLY = 0x6A, VK_K_ADD = 0x6B, VK_K_SUBTRACT = 0x6D,
    VK_K_DECIMAL = 0x6E, VK_K_DIVIDE = 0x6F,
    VK_K_F1 = 0x70,
    VK_K_LSHIFT = 0xA0, VK_K_RSHIFT = 0xA1,
    VK_K_LCTRL = 0xA2,  VK_K_RCTRL = 0xA3,
    VK_K_LALT = 0xA4,   VK_K_RALT = 0xA5,
    VK_K_OEM_1 = 0xBA,      // ; :
    VK_K_OEM_PLUS = 0xBB,   // = +
    VK_K_OEM_COMMA = 0xBC,  // , <
    VK_K_OEM_MINUS = 0xBD,  // - _
    VK_K_OEM_PERIOD = 0xBE, // . >
    VK_K_OEM_2 = 0xBF,      // / ?
    VK_K_GRAVE = 0xC0,      // ` ~
    VK_K_OEM_4 = 0xDB,      // [ {
    VK_K_OEM_5 = 0xDC,      // \ |
    VK_K_OEM_6 = 0xDD,      // ] }
    VK_K_OEM_7 = 0xDE,      // ' "
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
    if (key >= KEY_KP_0 && key <= KEY_KP_9)
        return VK_K_NUMPAD0 + (key - KEY_KP_0);

    switch (key) {
        case KEY_GRAVE:         return VK_K_GRAVE;
        case KEY_ENTER:         return VK_K_RETURN;
        case KEY_KP_ENTER:      return VK_K_RETURN;
        case KEY_SPACE:         return VK_K_SPACE;
        case KEY_ESCAPE:        return VK_K_ESCAPE;
        case KEY_TAB:           return VK_K_TAB;
        case KEY_BACKSPACE:     return VK_K_BACK;
        case KEY_CAPS_LOCK:     return VK_K_CAPS;
        case KEY_LEFT:          return VK_K_LEFT;
        case KEY_RIGHT:         return VK_K_RIGHT;
        case KEY_UP:            return VK_K_UP;
        case KEY_DOWN:          return VK_K_DOWN;
        case KEY_INSERT:        return VK_K_INSERT;
        case KEY_DELETE:        return VK_K_DELETE;
        case KEY_HOME:          return VK_K_HOME;
        case KEY_END:           return VK_K_END;
        case KEY_PAGE_UP:       return VK_K_PRIOR;
        case KEY_PAGE_DOWN:     return VK_K_NEXT;
        case KEY_LEFT_SHIFT:    return VK_K_LSHIFT;
        case KEY_RIGHT_SHIFT:   return VK_K_RSHIFT;
        case KEY_LEFT_CONTROL:  return VK_K_LCTRL;
        case KEY_RIGHT_CONTROL: return VK_K_RCTRL;
        case KEY_LEFT_ALT:      return VK_K_LALT;
        case KEY_RIGHT_ALT:     return VK_K_RALT;
        case KEY_LEFT_SUPER:    return VK_K_LWIN;
        case KEY_RIGHT_SUPER:   return VK_K_RWIN;

        // punctuation (these were the "keys that don't register", e.g. '/')
        case KEY_SEMICOLON:     return VK_K_OEM_1;
        case KEY_EQUAL:         return VK_K_OEM_PLUS;
        case KEY_COMMA:         return VK_K_OEM_COMMA;
        case KEY_MINUS:         return VK_K_OEM_MINUS;
        case KEY_PERIOD:        return VK_K_OEM_PERIOD;
        case KEY_SLASH:         return VK_K_OEM_2;
        case KEY_LEFT_BRACKET:  return VK_K_OEM_4;
        case KEY_BACKSLASH:     return VK_K_OEM_5;
        case KEY_RIGHT_BRACKET: return VK_K_OEM_6;
        case KEY_APOSTROPHE:    return VK_K_OEM_7;

        case KEY_KP_MULTIPLY:   return VK_K_MULTIPLY;
        case KEY_KP_ADD:        return VK_K_ADD;
        case KEY_KP_SUBTRACT:   return VK_K_SUBTRACT;
        case KEY_KP_DECIMAL:    return VK_K_DECIMAL;
        case KEY_KP_DIVIDE:     return VK_K_DIVIDE;
        default:                return 0;
    }
}

static int key_to_char(int key, bool shift)
{
    if (key >= KEY_A && key <= KEY_Z)       return shift ? key : key + 32;
    if (key >= KEY_KP_0 && key <= KEY_KP_9) return '0' + (key - KEY_KP_0);
    if (key >= KEY_ZERO && key <= KEY_NINE) {
        static const char shifted[] = ")!@#$%^&*(";
        return shift ? shifted[key - KEY_ZERO] : key;
    }

    switch (key) {
        case KEY_SPACE:         return ' ';
        case KEY_COMMA:         return shift ? '<' : ',';
        case KEY_MINUS:         return shift ? '_' : '-';
        case KEY_PERIOD:        return shift ? '>' : '.';
        case KEY_SLASH:         return shift ? '?' : '/';
        case KEY_SEMICOLON:     return shift ? ':' : ';';
        case KEY_EQUAL:         return shift ? '+' : '=';
        case KEY_LEFT_BRACKET:  return shift ? '{' : '[';
        case KEY_BACKSLASH:     return shift ? '|' : '\\';
        case KEY_RIGHT_BRACKET: return shift ? '}' : ']';
        case KEY_APOSTROPHE:    return shift ? '"' : '\'';
        default:                return 0;
    }
}

static void push_char(int c)
{
    if (s_char_count >= CHAR_QUEUE_SIZE) return;
    s_chars[(s_char_head + s_char_count) % CHAR_QUEUE_SIZE] = c;
    s_char_count++;
}

static void synthesize_chars(void)
{
    bool ctrl  = s_cur.key[KEY_LEFT_CONTROL] || s_cur.key[KEY_RIGHT_CONTROL];
    bool alt   = s_cur.key[KEY_LEFT_ALT]     || s_cur.key[KEY_RIGHT_ALT];
    bool shift = s_cur.key[KEY_LEFT_SHIFT]   || s_cur.key[KEY_RIGHT_SHIFT];
    if (ctrl || alt) return;

    for (int k = 0; k < KEY_COUNT; k++) {
        if (!(s_cur.key[k] && !s_prev.key[k])) continue;
        int c = key_to_char(k, shift);
        if (c) push_char(c);
    }
}

static void update_repeat(float dt)
{
    for (int k = 0; k < KEY_COUNT; k++) {
        s_repeat_fired[k] = false;
        if (!s_cur.key[k]) { s_hold[k] = 0.0f; continue; }

        if (!s_prev.key[k]) {
            s_hold[k] = 0.0f;
            s_next_repeat[k] = KEY_REPEAT_DELAY;
        } else {
            s_hold[k] += dt;
            if (s_hold[k] >= s_next_repeat[k]) {
                s_repeat_fired[k] = true;
                s_next_repeat[k] += KEY_REPEAT_INTERVAL;
            }
        }
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
    s_char_head = 0;
    s_char_count = 0;

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

    update_repeat(GetFrameTime());
    if (attached) synthesize_chars();
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

bool input_key_repeat(int k)     { return valid_key(k) && (input_key_pressed(k) || s_repeat_fired[k]); }

int input_char_pressed(void)
{
    if (!IsWallpaperAttached()) return GetCharPressed();
    if (s_char_count == 0) return 0;

    int c = s_chars[s_char_head];
    s_char_head = (s_char_head + 1) % CHAR_QUEUE_SIZE;
    s_char_count--;
    return c;
}
