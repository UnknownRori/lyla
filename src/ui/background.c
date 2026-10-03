#include <raylib.h>
#include <raymath.h>
#include "ui.h"
#include "input.h"
#include "window_drag.h"
#include "assets/blur330_frag.c"
#include <stdlib.h>

#define PAN_TOGGLE_KEY KEY_F1

#define BG_ZOOM 1.15f
#define MAX_PAN_SPEED 10.f
#define AUTO_PAN_DUR       8.0f
#define AUTO_PAN_DUR_RANGE 1.0f
#define AUTO_PAN_PAUSE     3.0f
#define PAN_FOLLOW_SPEED   2.0f
#define SHAKE_AMOUNT 0.02f
#define SHAKE_SPEED  0.4f

static RenderTexture2D pass1;
static RenderTexture2D cache;
static Shader         blurShader;
static int resolutionLoc;
static int directionLoc;
static int blurRadiusLoc;


static Vector2        shift = {0};
static bool           panning = true;
static bool    was_panning = true;
static Vector2 auto_from = {0};
static Vector2 auto_to   = {0};
static Vector2 auto_pos  = {0};
static float   auto_t    = 1.0f;
static float   auto_dur  = 4.0f;
static float   auto_wait = 0.0f;
static float   shake_time = 0.0f;
static Texture* old_thumb = NULL;
static bool dirty = true;

void hud_background_set_panning(bool on) { panning = on; }
void hud_background_toggle_panning(void) { panning = !panning; }
bool hud_background_panning(void)        { return panning; }

void hud_background_init()
{
    pass1 = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
    cache = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
    blurShader = LoadShaderFromMemory(NULL, (char*) __resources_blur330_frag_glsl);
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

void hud_background(Texture* thumbptr, int w, int h, f32 dt)
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
    int iw = (int)(thumb.width  * scale + 0.5f);
    int ih = (int)(thumb.height * scale + 0.5f);

    if (!window_is_resizing() && (pass1.texture.width != iw || pass1.texture.height != ih)) {
        UnloadRenderTexture(pass1);
        UnloadRenderTexture(cache);
        pass1 = LoadRenderTexture(iw, ih);
        cache = LoadRenderTexture(iw, ih);
        dirty = true;
    }

    int cw = cache.texture.width;
    int ch = cache.texture.height;

    float win_w = fminf((float)w / BG_ZOOM, (float)cw);
    float win_h = fminf((float)h / BG_ZOOM, (float)ch);
    float max_x = fmaxf((cw - win_w) / 2.0f, 0.0f);
    float max_y = fmaxf((ch - win_h) / 2.0f, 0.0f);

    Vector2 want = { 0.0f, 0.0f };

    if (!panning && was_panning) {
        auto_from = shift;
        auto_to   = shift;
        auto_pos  = shift;
        auto_t    = 1.0f;
        auto_wait = 0.0f;
    }
    was_panning = panning;
    if (panning) {
        Vector2 mouse = input_mouse_position();
        want.x = Clamp((mouse.x - w / 2.0f) / (w / 2.0f), -1.0f, 1.0f);
        want.y = Clamp((mouse.y - h / 2.0f) / (h / 2.0f), -1.0f, 1.0f);
    } else {
        if (auto_wait > 0.0f) {
            auto_wait -= dt;
        } else {
            auto_t += dt / auto_dur;
            if (auto_t >= 1.0f) {
                auto_from = auto_to;

                Vector2 next = auto_from;
                for (int i = 0; i < 10; i++) {
                    next.x = GetRandomValue(-1000, 1000) / 1000.0f;
                    next.y = GetRandomValue(-1000, 1000) / 1000.0f;
                    f32 dist = Vector2Distance(next, auto_from);
                    if (dist >= 1.0f && dist <= 1.5f) break;
                }

                auto_to   = next;
                auto_dur  = AUTO_PAN_DUR + AUTO_PAN_DUR_RANGE * (GetRandomValue(0, 1000) / 1000.0f);
                auto_t    = 0.0f;
                auto_wait = AUTO_PAN_PAUSE;
            }
        }

        float k = Clamp(auto_t, 0.0f, 1.0f);
        float e = k * k * (3.0f - 2.0f * k);
        auto_pos = Vector2Lerp(auto_from, auto_to, e);

        float amp = (auto_wait > 0.0f) ? SHAKE_AMOUNT : 0.0f;
        shake_time += dt * SHAKE_SPEED;
        want.x = auto_pos.x + sinf(shake_time * 6.0f) * amp;
        want.y = auto_pos.y + sinf(shake_time * 7.3f + 1.7f) * amp;
    }
    want.x = Clamp(want.x, -MAX_PAN_SPEED, MAX_PAN_SPEED);
    want.y = Clamp(want.y, -MAX_PAN_SPEED, MAX_PAN_SPEED);

    float t = Clamp(PAN_FOLLOW_SPEED * dt, 0.0f, 1.0f);
    shift.x += (want.x - shift.x) * t;
    shift.y += (want.y - shift.y) * t;

    if (dirty && IsRenderTextureValid(pass1) && IsRenderTextureValid(cache)) {
        dirty = false;

        Rectangle source = { 0, 0, (float)thumb.width, (float)thumb.height };
        Rectangle full   = { 0, 0, (float)cw, (float)ch };

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
                    (Rectangle){ 0, 0, (float)cw, -(float)ch },
                    full,
                    (Vector2){0, 0},
                    0.0f,
                    WHITE
                );
            EndShaderMode();
        EndTextureMode();
    }

    float left = (cw - win_w) / 2.0f + shift.x * max_x;
    float top  = (ch - win_h) / 2.0f + shift.y * max_y;
    left = Clamp(left, 0.0f, (float)cw - win_w);
    top  = Clamp(top,  0.0f, (float)ch - win_h);
    float mem_top = ch - (top + win_h);

    DrawTexturePro(
        cache.texture,
        (Rectangle){ left, mem_top, win_w, -win_h },
        (Rectangle){ 0, 0, (float)w, (float)h },
        (Vector2){0, 0},
        0.0f,
        ColorAlpha(WHITE, 0.3f)
    );
    DrawRectangle(
        0, 0, w, h,
        player_paused() ? ColorAlpha(BLACK, 0.65f) : ColorAlpha(BLACK, 0.25f)
    );
}
