#pragma once

#include "./playlist.h"
#include "types.h"

typedef struct {
    usize* indices;
    usize  count;
} PlaylistSearchResult;

void playlist_search(const Playlist* playlist, const char* query, PlaylistSearchResult* out);

void playlist_search_result_free(PlaylistSearchResult* result);
