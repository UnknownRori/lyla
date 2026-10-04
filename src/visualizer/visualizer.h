#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <raylib.h>

#include "types.h"

bool visualizer_init(void);
void visualizer_shutdown(void);

void visualizer_render(
    Rectangle boundary, 
    const f32* smooth, 
    const f32* smear,
    usize m, 
    bool detached, 
    f32 beat, 
    f32 dt
);

void visualizer_draw_corner_glow(f32 beat, int w, int h);
