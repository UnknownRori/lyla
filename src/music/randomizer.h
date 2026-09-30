#pragma once

#include "types.h"

typedef struct {
    u32 n, p, a, b, x;
} PlaylistRandomizer;

void playlist_randomizer_init(PlaylistRandomizer* rand, u32 n);
u32  playlist_randomizer_next(PlaylistRandomizer* rand);
