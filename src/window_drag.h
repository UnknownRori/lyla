#pragma once

#include <stdbool.h>

void window_drag_init(void);
void window_drag_update(int w, int h);

bool window_is_resizing();
