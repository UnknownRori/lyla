#if !defined(WALLPAPER_ENGINE)
#include <raylib.h>
#include <time.h>
#include "lyla.h"
#include "./assets/lyla.c"

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>

static void UpdateGame()
{
    lyla_update();
}
#endif

#if defined(_WIN32)
#   define CP_UTF8 65001
#   ifdef __cplusplus
extern "C" {
#   endif

__declspec(dllimport) int __stdcall SetConsoleOutputCP(unsigned int wCodePageID);

#   ifdef __cplusplus
}
#   endif
#endif

int main()
{
#if defined(_WIN32)
    SetConsoleOutputCP(CP_UTF8);
#endif

    int raylib_cfg_flag = 
            FLAG_WINDOW_UNDECORATED  | 
            FLAG_WINDOW_TRANSPARENT  | 
            FLAG_WINDOW_TOPMOST | FLAG_VSYNC_HINT;
    SetConfigFlags(raylib_cfg_flag);
    InitWindow(1280, 720, "UnknownRori's Lyla Music Player");
    InitAudioDevice();
#ifdef WITH_MICROPHONE
    InitMicrophoneDevice(44100, 2);
#endif
    // SetTargetFPS(60);
    SetExitKey(KEY_NULL);
    SetRandomSeed((unsigned int)time(NULL));

    lyla_init();
    Image img = LoadImageFromMemory(".png", icon_png, icon_png_len);
    SetWindowIcon(img);

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateGame, 0, 1);
#else
    while (!WindowShouldClose()) {
        if (lyla_should_close()) {
            break;
        }

        lyla_update();
    }
#endif

    lyla_shutdown();

#ifdef WITH_MICROPHONE
    CloseMicrophoneDevice();
#endif
    CloseAudioDevice();
    CloseWindow();
}
#else
#   include "wallpaper_engine.c"
#endif
