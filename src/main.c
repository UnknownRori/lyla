#include <raylib.h>
#include "lyla.h"
#include "./assets/lyla.c"

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
    SetTargetFPS(60);

    lyla_init();
    Image img = LoadImageFromMemory(".png", icon_png, icon_png_len);
    SetWindowIcon(img);

    while (!WindowShouldClose()) {
        if (lyla_should_close()) {
            break;
        }

        lyla_update();
    }

    lyla_shutdown();

#ifdef WITH_MICROPHONE
    CloseMicrophoneDevice();
#endif
    CloseAudioDevice();
    CloseWindow();
}
