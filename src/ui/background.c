#include <raylib.h>
#include <raymath.h>
#include "ui.h"
#include "input.h"
#include "window_drag.h"
#include "assets/blur330_frag.c"
#include <stdlib.h>

#define PAN_TOGGLE_KEY KEY_F1

static Camera2D       camera = {0};
static RenderTexture2D pass1;
static RenderTexture2D pass2;
static RenderTexture2D cache;
static Shader         blurShader;
static Vector2        shift = {0};
static bool           panning = true;
static Texture* old_thumb = NULL;
static bool dirty = true;

void hud_background_set_panning(bool on) { panning = on; }
void hud_background_toggle_panning(void) { panning = !panning; }
bool hud_background_panning(void)        { return panning; }

void hud_background_init()
{
    pass1 = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
    pass2 = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
    cache = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
    blurShader = LoadShaderFromMemory(NULL, (char*) __resources_blur330_frag_glsl);
    camera.zoom = 1.2;
}

void hud_background_shutdown()
{
    UnloadRenderTexture(pass1);
    UnloadRenderTexture(pass2);
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
    f32 scale = scaleX > scaleY ? scaleX : scaleY;

    Rectangle source = { 0, 0, thumb.width, thumb.height };
    Rectangle dest = {
        (w - thumb.width * scale) / 2.0f,
        (h - thumb.height * scale) / 2.0f,
        thumb.width * scale,
        thumb.height * scale
    };

    Vector2 center = { w / 2.0f, h / 2.0f };
    Vector2 want   = { 0.0f, 0.0f };

    if (panning) {
        Vector2 mouse = input_mouse_position();

        float maxX = w * (camera.zoom - 1.0f) / 2.0f;
        float maxY = h * (camera.zoom - 1.0f) / 2.0f;

        want.x = Clamp((center.x - mouse.x) / 6.0f, -maxX, maxX);
        want.y = Clamp((center.y - mouse.y) / 6.0f, -maxY, maxY);
    }

    float t = Clamp(10.0f * dt, 0.0f, 1.0f);
    shift.x += (want.x - shift.x) * t;
    shift.y += (want.y - shift.y) * t;

    camera.target = center;
    camera.offset = (Vector2){ center.x + shift.x, center.y + shift.y };

    if (!window_is_resizing() && (pass1.texture.width != w || pass1.texture.height != h)) {
        UnloadRenderTexture(pass1);
        UnloadRenderTexture(pass2);
        UnloadRenderTexture(cache);
        pass1 = LoadRenderTexture(w, h);
        pass2 = LoadRenderTexture(w, h);
        cache = LoadRenderTexture(w, h);
        dirty = true;
    }

    if (dirty && IsRenderTextureValid(pass1) && IsRenderTextureValid(pass2)&& IsRenderTextureValid(cache)) {
        dirty = false;
        BeginTextureMode(pass1);
            ClearBackground(BLACK);
            DrawTexturePro(thumb, source, dest, (Vector2){0, 0}, 0.0f, WHITE);
        EndTextureMode();

        int resolutionLoc = GetShaderLocation(blurShader, "resolution");
        int directionLoc = GetShaderLocation(blurShader, "direction");
        int blurRadiusLoc = GetShaderLocation(blurShader, "blurRadius");

        f32 res[2] = { (f32)w, (f32)h };
        f32 radius = 8.0f;
        SetShaderValue(blurShader, resolutionLoc, res, SHADER_UNIFORM_VEC2);
        SetShaderValue(blurShader, blurRadiusLoc, &radius, SHADER_UNIFORM_FLOAT);

        BeginTextureMode(pass2);
            ClearBackground(BLACK);
            BeginShaderMode(blurShader);
                f32 dirX[2] = { 1.0f, 0.0f };
                SetShaderValue(blurShader, directionLoc, dirX, SHADER_UNIFORM_VEC2);

                DrawTexturePro(
                    pass1.texture,
                    (Rectangle){ 0, 0, w, -h },
                    (Rectangle){ 0, 0, w, h },
                    (Vector2){0, 0},
                    0.0f,
                    WHITE
                );
            EndShaderMode();
        EndTextureMode();

        BeginTextureMode(cache);
            ClearBackground(BLACK);
            BeginShaderMode(blurShader);
                f32 dirY[2] = { 0.0f, 1.0f };
                SetShaderValue(blurShader, directionLoc, dirY, SHADER_UNIFORM_VEC2);

            
                DrawTexturePro(
                    pass2.texture,
                    (Rectangle){ 0, 0, w, -h },
                    (Rectangle){ -69, -69, w + 69, h + 69 },
                    (Vector2){0, 0},
                    0.0f,
                    WHITE
                );
            EndShaderMode();
        EndTextureMode();
    }

    BeginMode2D(camera);
        DrawTexturePro(
            cache.texture,
            (Rectangle){ 0, 0, w, -h },
            (Rectangle){ 0, 0, w + 69, h + 69 },
            (Vector2){0, 0},
            0.0f,
            ColorAlpha(WHITE, 0.3f)
        );
        DrawRectangle(
            0,
            0,
            w,
            h,
            player_paused()
            ? ColorAlpha(BLACK, 0.65) :
            ColorAlpha(BLACK, 0.25)
        );
    EndMode2D();
}
