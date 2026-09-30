// Thank you claude for making broken sit
#if defined(_WIN32)
#else
#error "Dummy dumb dumb, not supported yet"
#endif

#include <windows.h>
#include "wallpaper.h"

extern void *GetWindowHandle(void);

static HWND g_workerw = NULL;

static int       g_attached = 0;
static LONG_PTR  g_oldStyle, g_oldExStyle;
static HWND      g_oldParent;
static RECT      g_oldRect;

static BOOL CALLBACK EnumProc(HWND hwnd, LPARAM lParam)
{
    (void)lParam;
    if (FindWindowExA(hwnd, NULL, "SHELLDLL_DefView", NULL))
        g_workerw = FindWindowExA(NULL, hwnd, "WorkerW", NULL);
    return TRUE;
}

static HWND FindWallpaperParent(void)
{
    HWND progman = FindWindowA("Progman", NULL);
    SendMessageTimeoutA(progman, 0x052C, 0xD, 0x1, SMTO_NORMAL, 1000, NULL);

    g_workerw = NULL;
    EnumWindows(EnumProc, 0);

    if (!g_workerw)
        g_workerw = FindWindowExA(progman, NULL, "WorkerW", NULL);

    return g_workerw ? g_workerw : progman;
}

void AttachAsWallpaper(void)
{
    HWND hwnd = (HWND)GetWindowHandle();
    if (!hwnd || g_attached) return;

    g_oldStyle   = GetWindowLongPtrA(hwnd, GWL_STYLE);
    g_oldExStyle = GetWindowLongPtrA(hwnd, GWL_EXSTYLE);
    g_oldParent  = GetParent(hwnd);
    GetWindowRect(hwnd, &g_oldRect);

    HWND parent = FindWallpaperParent();
    int w = GetSystemMetrics(SM_CXSCREEN);
    int h = GetSystemMetrics(SM_CYSCREEN);

    SetWindowLongPtrA(hwnd, GWL_STYLE,
        (g_oldStyle & ~(WS_CAPTION | WS_THICKFRAME)) | WS_POPUP);
    SetWindowLongPtrA(hwnd, GWL_EXSTYLE,
        g_oldExStyle | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE);

    SetParent(hwnd, parent);
    SetWindowPos(hwnd, HWND_BOTTOM, 0, 0, w, h,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_FRAMECHANGED);
    g_attached = 1;
}

void DetachWallpaper(void)
{
    HWND hwnd = (HWND)GetWindowHandle();
    if (!hwnd || !g_attached) return;

    SetParent(hwnd, g_oldParent);
    SetWindowLongPtrA(hwnd, GWL_STYLE,   g_oldStyle);
    SetWindowLongPtrA(hwnd, GWL_EXSTYLE, g_oldExStyle);

    SetWindowPos(hwnd, HWND_TOP,
                 g_oldRect.left, g_oldRect.top,
                 g_oldRect.right - g_oldRect.left,
                 g_oldRect.bottom - g_oldRect.top,
                 SWP_SHOWWINDOW | SWP_FRAMECHANGED);
    SetForegroundWindow(hwnd);

    SystemParametersInfoA(SPI_SETDESKWALLPAPER, 0, NULL, 0);
    g_attached = 0;
}

int IsWallpaperAttached(void) { return g_attached; }
