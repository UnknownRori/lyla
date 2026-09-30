#if defined(_WIN32)

#define WIN32_MEAN_AND_LEAN
#define NOGDI

#include <windows.h>
#include <stdbool.h>

extern void* GetWindowHandle(void);

void set_window_ontop(bool enable)
{
    HWND hwnd = (HWND)GetWindowHandle();
    if (!hwnd) return;

    HWND insert_after = enable ? HWND_TOPMOST : HWND_NOTOPMOST;

    SetWindowPos(
        hwnd, 
        insert_after, 
        0, 0,
        0, 0, 
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE
    );
}
#else
void set_window_ontop(bool enable) {}
#endif
