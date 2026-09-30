#include "raylib.h"
#include "ui.h"
#include "assets/blur330_frag.c"
#include <stdlib.h>

RenderTexture2D pass1;
RenderTexture2D pass2;
Shader blurShader;

void hud_background_init()
{
    pass1 = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
    pass2 = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
    blurShader = LoadShaderFromMemory(NULL, (char*) __resources_blur330_frag_glsl);
}

void hud_background_shutdown()
{
    UnloadRenderTexture(pass1);
    UnloadRenderTexture(pass2);
    UnloadShader(blurShader);
}

void hud_background(Texture thumb, int w, int h)
{
    if (pass1.texture.width != w || pass1.texture.height != h) {
        UnloadRenderTexture(pass1);
        UnloadRenderTexture(pass2);
        pass1 = LoadRenderTexture(w, h);
        pass2 = LoadRenderTexture(w, h);
    }

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

    BeginShaderMode(blurShader);
    f32 dirY[2] = { 0.0f, 1.0f };
    SetShaderValue(blurShader, directionLoc, dirY, SHADER_UNIFORM_VEC2);

    DrawTexturePro(
        pass2.texture, 
        (Rectangle){ 0, 0, w, -h }, 
        (Rectangle){ 0, 0, w, h }, 
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
    EndShaderMode();
}

