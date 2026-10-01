#pragma once

void AttachAsWallpaper(void);
void DetachWallpaper(void);
int  IsWallpaperAttached(void);

void WallpaperMouse(int* x, int* y, int *leftDown);
int WallpaperKeyDown(int vk);
