#include <raylib.h>
#include <raymath.h>
#include "types.h"
#include "ui.h"
#include "input.h"
#include "window_drag.h"
#if defined(PLATFORM_WEB)
#include "assets/blur100_frag.c"
#else
#include "assets/blur330_frag.c"
#endif
#include <stdlib.h>

#define PAN_TOGGLE_KEY KEY_F1

#define BG_ZOOM 1.15f
#define ZOOM_MIN 1.08f
#define ZOOM_MAX 1.35f
#define ZOOM_STEP 0.12f
#define MAX_PAN_SPEED 2.f
#define PAN_FOLLOW_SPEED 1.5f
#define SHAKE_AMOUNT 0.01f
#define SHAKE_SPEED 0.2f
#define FADE_IN_TIME 3.0f
#define FADE_OUT_TIME 3.0f

#define AUTO_PAN_PAUSE_MIN 4.0f
#define AUTO_PAN_PAUSE_MAX 8.0f
#define EASE_COUNT 5
#define AUTO_PAN_DUR_SCALE 1.6f

#define JUMP_RATE 0.015f
#define JUMP_COOLDOWN 30.0f
#define JUMP_ARRIVAL_GUARD 3.0f
#define JUMP_FADE_OUT_MIN 0.7f
#define JUMP_FADE_OUT_MAX 1.4f
#define JUMP_FADE_IN_MIN 0.9f
#define JUMP_FADE_IN_MAX 1.8f
#define JUMP_MIN_DIST 1.2f
#define JUMP_SETTLE_MIN 0.5f
#define JUMP_SETTLE_MAX 2.0f

typedef enum {
    PAN_HOLD,
    PAN_MOVE,
    PAN_FADE_OUT,
    PAN_FADE_IN,
} PanState;

static RenderTexture2D pass1;
static RenderTexture2D cache;
static Shader blurShader;
static i32 resolutionLoc;
static i32 directionLoc;
static i32 blurRadiusLoc;

static Vector2 shift = {0};
static bool panning = false;
static bool was_panning = true;
static Vector2 auto_from = {0};
static Vector2 auto_to = {0};
static Vector2 auto_pos = {0};
static Vector2 auto_perp = {0};
static f32 auto_t = 1.0f;
static f32 auto_dur = 8.0f;
static f32 auto_wait = 0.0f;
static i32 auto_ease = 0;
static f32 zoom_cur = BG_ZOOM;
static f32 zoom_from = BG_ZOOM;
static f32 zoom_to = BG_ZOOM;
static f32 shake_amp = SHAKE_AMOUNT;
static f32 jump_cooldown = JUMP_COOLDOWN;
static f32 arrive_guard = 0.0f;
static PanState pan_state = PAN_HOLD;
static f32 pan_fade = 1.0f;
static f32 jump_out_dur = 1.0f;
static f32 jump_in_dur = 1.2f;
static f32 shake_time = 0.0f;
static Texture* old_thumb = NULL;
static bool dirty = true;

void hud_background_set_panning(bool on) { panning = on; }
void hud_background_toggle_panning(void) { panning = !panning; }
bool hud_background_panning(void)        { return panning; }

void hud_background_init()
{
    pass1 = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
    cache = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
#if defined(PLATFORM_WEB)
    blurShader = LoadShaderFromMemory(NULL, (char*) __resources_blur100_frag_glsl);
#else
    blurShader = LoadShaderFromMemory(NULL, (char*) __resources_blur330_frag_glsl);
#endif
    resolutionLoc = GetShaderLocation(blurShader, "resolution");
    directionLoc  = GetShaderLocation(blurShader, "direction");
    blurRadiusLoc = GetShaderLocation(blurShader, "blurRadius");
}

void hud_background_shutdown()
{
    UnloadRenderTexture(pass1);
    UnloadRenderTexture(cache);
    UnloadShader(blurShader);
}

static f32 rand01(void)
{
    return GetRandomValue(0, 10000) / 10000.0f;
}

static f32 rand_range(f32 a, f32 b)
{
    return a + (b - a) * rand01();
}

static Vector2 pick_target(Vector2 from, f32 min_d, f32 max_d)
{
    Vector2 best = from;
    f32 best_miss = 1e9f;

    for (i32 i = 0; i < 24; i++) {
        Vector2 c = { rand_range(-1.0f, 1.0f), rand_range(-1.0f, 1.0f) };
        f32 d = Vector2Distance(c, from);
        if (d >= min_d && d <= max_d) return c;

        f32 miss = d < min_d ? min_d - d : d - max_d;
        if (miss < best_miss) { best_miss = miss; best = c; }
    }
    return best;
}

static Vector2 pick_axis_target(Vector2 from, bool horizontal)
{
    Vector2 to = from;
    f32 side_drift = rand_range(-0.15f, 0.15f);
    f32 start = horizontal ? from.x : from.y;
    f32 end = start;

    for (i32 i = 0; i < 12; i++) {
        end = rand_range(-1.0f, 1.0f);
        if (fabsf(end - start) >= 0.8f) break;
    }
    if (fabsf(end - start) < 0.8f) {
        end = start > 0.0f ? -rand_range(0.6f, 1.0f) : rand_range(0.6f, 1.0f);
    }

    if (horizontal) {
        to.x = end;
        to.y = Clamp(from.y + side_drift, -1.0f, 1.0f);
    } else {
        to.y = end;
        to.x = Clamp(from.x + side_drift, -1.0f, 1.0f);
    }
    return to;
}

static f32 ease_pan(f32 k, i32 kind)
{
    switch (kind) {
        case 0:  return k * k * (3.0f - 2.0f * k);
        case 1:  return k * k * k * (k * (k * 6.0f - 15.0f) + 10.0f);
        case 2:  return 0.5f - 0.5f * cosf(PI * k);
        case 3:  return 1.0f - (1.0f - k) * (1.0f - k);
        default: {
            if (k < 0.5f) return 2.0f * k * k;
            f32 m = -2.0f * k + 2.0f;
            return 1.0f - m * m * 0.5f;
        }
    }
}

static f32 pick_pause(void)
{
    f32 r = rand01();
    if (r < 0.2f)  return rand_range(0.3f, 1.0f);
    if (r < 0.85f) return rand_range(AUTO_PAN_PAUSE_MIN, AUTO_PAN_PAUSE_MAX);
    return rand_range(6.0f, 10.0f);
}

static void pan_enter_hold(f32 wait)
{
    pan_state = PAN_HOLD;
    auto_wait = wait;
    shake_amp = rand_range(0.004f, SHAKE_AMOUNT * 1.5f);
}

static void pan_start_move(void)
{
    f32 r = rand01();
    f32 arc_chance = 0.5f;

    auto_from = auto_to;

    if (r < 0.25f) {
        auto_to  = pick_target(auto_from, 0.15f, 0.55f);
        auto_dur = rand_range(10.0f, 16.0f);
    } else if (r < 0.60f) {
        auto_to  = pick_target(auto_from, 0.7f, 1.4f);
        auto_dur = rand_range(7.0f, 11.0f);
    } else if (r < 0.75f) {
        auto_to  = pick_target(auto_from, 1.5f, 2.4f);
        auto_dur = rand_range(5.0f, 8.0f);
    } else {
        auto_to  = pick_axis_target(auto_from, r < 0.875f);
        auto_dur = rand_range(6.0f, 10.0f);
        arc_chance = 0.0f;
    }

    auto_dur *= AUTO_PAN_DUR_SCALE;
    auto_ease = GetRandomValue(0, EASE_COUNT - 1);
    auto_t = 0.0f;

    Vector2 d = Vector2Subtract(auto_to, auto_from);
    f32 len = Vector2Length(d);
    auto_perp = (Vector2){ 0.0f, 0.0f };
    if (len > 0.001f && rand01() < arc_chance) {
        f32 bend = rand_range(-0.35f, 0.35f) * len;
        auto_perp = (Vector2){ -d.y / len * bend, d.x / len * bend };
    }

    zoom_from = zoom_cur;
    zoom_to = Clamp(zoom_cur + rand_range(-ZOOM_STEP, ZOOM_STEP), ZOOM_MIN, ZOOM_MAX);

    pan_state = PAN_MOVE;
}

static void pan_start_jump(void)
{
    jump_out_dur = rand_range(JUMP_FADE_OUT_MIN, JUMP_FADE_OUT_MAX);
    jump_in_dur  = rand_range(JUMP_FADE_IN_MIN, JUMP_FADE_IN_MAX);
    jump_cooldown = JUMP_COOLDOWN;
    pan_state = PAN_FADE_OUT;
}

static void pan_teleport(void)
{
    Vector2 p = pick_target(shift, JUMP_MIN_DIST, 3.0f);
    shift = auto_from = auto_to = auto_pos = p;
    auto_perp = (Vector2){ 0.0f, 0.0f };
    auto_t = 1.0f;
    zoom_cur = zoom_from = zoom_to = rand_range(ZOOM_MIN, ZOOM_MAX);
}

static bool jump_due(f32 dt)
{
    if (jump_cooldown > 0.0f) jump_cooldown -= dt;
    if (arrive_guard > 0.0f) arrive_guard -= dt;

    if (pan_state != PAN_HOLD && pan_state != PAN_MOVE) return false;
    if (jump_cooldown > 0.0f || arrive_guard > 0.0f) return false;
    return rand01() < JUMP_RATE * dt;
}

static void update_auto_pan(f32 dt)
{
    if (jump_due(dt)) pan_start_jump();

    switch (pan_state) {
    case PAN_HOLD:
        auto_wait -= dt;
        if (auto_wait <= 0.0f) pan_start_move();
        break;

    case PAN_MOVE:
        auto_t += dt / auto_dur;
        if (auto_t >= 1.0f) {
            auto_t = 1.0f;
            auto_from = auto_to;
            arrive_guard = JUMP_ARRIVAL_GUARD;
            pan_enter_hold(pick_pause());
        }
        break;

    case PAN_FADE_OUT:
        if (auto_t < 1.0f) auto_t = fminf(1.0f, auto_t + dt / auto_dur);
        pan_fade -= dt / jump_out_dur;
        if (pan_fade <= 0.0f) {
            pan_fade = 0.0f;
            pan_teleport();
            pan_state = PAN_FADE_IN;
        }
        break;

    case PAN_FADE_IN:
        pan_fade += dt / jump_in_dur;
        if (pan_fade >= 1.0f) {
            pan_fade = 1.0f;
            pan_enter_hold(rand_range(JUMP_SETTLE_MIN, JUMP_SETTLE_MAX));
        }
        break;
    }
}

void hud_background(Texture* thumbptr, i32 w, i32 h, f32 dt, f32 music_elapsed, f32 music_duration)
{
    if (thumbptr != old_thumb) {
        old_thumb = thumbptr;
        dirty = true;
    }

    Texture thumb = *thumbptr;
    if (input_key_pressed(PAN_TOGGLE_KEY)) panning = !panning;

    f32 scaleX = (f32)w / thumb.width;
    f32 scaleY = (f32)h / thumb.height;
    f32 scale  = scaleX > scaleY ? scaleX : scaleY;
    i32 iw = (i32)(thumb.width  * scale + 0.5f);
    i32 ih = (i32)(thumb.height * scale + 0.5f);

    if (!window_is_resizing() && (pass1.texture.width != iw || pass1.texture.height != ih)) {
        UnloadRenderTexture(pass1);
        UnloadRenderTexture(cache);
        pass1 = LoadRenderTexture(iw, ih);
        cache = LoadRenderTexture(iw, ih);
        dirty = true;
    }

    i32 cw = cache.texture.width;
    i32 ch = cache.texture.height;

    Vector2 want = { 0.0f, 0.0f };

    if (!panning && was_panning) {
        auto_from = shift;
        auto_to   = shift;
        auto_pos  = shift;
        auto_perp = (Vector2){ 0.0f, 0.0f };
        auto_t    = 1.0f;
        zoom_from = zoom_cur;
        zoom_to   = zoom_cur;
        arrive_guard = JUMP_ARRIVAL_GUARD;
        pan_enter_hold(1.0f);
    }
    was_panning = panning;

    if (panning) {
        Vector2 mouse = input_mouse_position();
        want.x = Clamp((mouse.x - w / 2.0f) / (w / 2.0f), -1.0f, 1.0f);
        want.y = Clamp((mouse.y - h / 2.0f) / (h / 2.0f), -1.0f, 1.0f);

        pan_fade = fminf(pan_fade + dt / jump_in_dur, 1.0f);
        zoom_cur += (BG_ZOOM - zoom_cur) * Clamp(PAN_FOLLOW_SPEED * dt, 0.0f, 1.0f);
    } else {
        update_auto_pan(dt);

        f32 k = Clamp(auto_t, 0.0f, 1.0f);
        f32 e = ease_pan(k, auto_ease);
        f32 bow = sinf(PI * k);
        Vector2 base = Vector2Lerp(auto_from, auto_to, e);
        auto_pos = (Vector2){ base.x + auto_perp.x * bow, base.y + auto_perp.y * bow };
        zoom_cur = Lerp(zoom_from, zoom_to, e);

        f32 amp = (pan_state == PAN_HOLD) ? shake_amp : 0.0f;
        shake_time += dt * SHAKE_SPEED;
        want.x = auto_pos.x + sinf(shake_time * 6.0f) * amp;
        want.y = auto_pos.y + sinf(shake_time * 7.3f + 1.7f) * amp;
    }
    want.x = Clamp(want.x, -MAX_PAN_SPEED, MAX_PAN_SPEED);
    want.y = Clamp(want.y, -MAX_PAN_SPEED, MAX_PAN_SPEED);

    f32 t = Clamp(PAN_FOLLOW_SPEED * dt, 0.0f, 1.0f);
    shift.x += (want.x - shift.x) * t;
    shift.y += (want.y - shift.y) * t;

    f32 win_w = fminf((f32)w / zoom_cur, (f32)cw);
    f32 win_h = fminf((f32)h / zoom_cur, (f32)ch);
    f32 max_x = fmaxf((cw - win_w) / 2.0f, 0.0f);
    f32 max_y = fmaxf((ch - win_h) / 2.0f, 0.0f);

    if (dirty && IsRenderTextureValid(pass1) && IsRenderTextureValid(cache)) {
        dirty = false;

        Rectangle source = { 0, 0, (f32)thumb.width, (f32)thumb.height };
        Rectangle full   = { 0, 0, (f32)cw, (f32)ch };

        BeginTextureMode(pass1);
            ClearBackground(BLACK);
            DrawTexturePro(thumb, source, full, (Vector2){0, 0}, 0.0f, WHITE);
        EndTextureMode();

        f32 res[2] = { (f32)cw, (f32)ch };
        f32 radius = 8.0f;
        SetShaderValue(blurShader, resolutionLoc, res, SHADER_UNIFORM_VEC2);
        SetShaderValue(blurShader, blurRadiusLoc, &radius, SHADER_UNIFORM_FLOAT);

        BeginTextureMode(pass1);
            ClearBackground(BLACK);
            BeginShaderMode(blurShader);
                f32 dirX[2] = { 1.0f, 0.0f };
                SetShaderValue(blurShader, directionLoc, dirX, SHADER_UNIFORM_VEC2);
                DrawTexturePro(thumb, source, full, (Vector2){0, 0}, 0.0f, WHITE);
            EndShaderMode();
        EndTextureMode();

        BeginTextureMode(cache);
            ClearBackground(BLACK);
            BeginShaderMode(blurShader);
                f32 dirY[2] = { 0.0f, 1.0f };
                SetShaderValue(blurShader, directionLoc, dirY, SHADER_UNIFORM_VEC2);
                DrawTexturePro(
                    pass1.texture,
                    (Rectangle){ 0, 0, (f32)cw, -(f32)ch },
                    full,
                    (Vector2){0, 0},
                    0.0f,
                    WHITE
                );
            EndShaderMode();
        EndTextureMode();
    }

    f32 left = (cw - win_w) / 2.0f + shift.x * max_x;
    f32 top  = (ch - win_h) / 2.0f + shift.y * max_y;
    left = Clamp(left, 0.0f, (f32)cw - win_w);
    top  = Clamp(top,  0.0f, (f32)ch - win_h);
    f32 mem_top = ch - (top + win_h);

    f32 fade = 1.0f;
    f32 in_t  = Clamp(FADE_IN_TIME,  0.0f, music_duration * 0.5f);
    f32 out_t = Clamp(FADE_OUT_TIME, 0.0f, music_duration * 0.5f);
    if (in_t > 0.0f && music_elapsed < in_t) {
        fade = music_elapsed / in_t;
    } else if (out_t > 0.0f && music_elapsed > music_duration - out_t) {
        fade = (music_duration - music_elapsed) / out_t;
    }
    fade = Clamp(fade, 0.0f, 1.0f);
    fade = fade * fade * (3.0f - 2.0f * fade);

    f32 jump_fade = Clamp(pan_fade, 0.0f, 1.0f);
    jump_fade = jump_fade * jump_fade * (3.0f - 2.0f * jump_fade);

    DrawTexturePro(
        cache.texture,
        (Rectangle){ left, mem_top, win_w, -win_h },
        (Rectangle){ 0, 0, (f32)w, (f32)h },
        (Vector2){0, 0},
        0.0f,
        ColorAlpha(WHITE, 0.3f * fade * jump_fade)
    );
    DrawRectangle(
        0, 0, w, h,
        player_paused() ? ColorAlpha(BLACK, 0.65f) : ColorAlpha(BLACK, 0.25f)
    );
}
