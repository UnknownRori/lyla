#pragma once

#include "track.h"

typedef enum {
    TAG_META_TITLE  = 1 << 0,
    TAG_META_ARTIST = 1 << 1,
    TAG_META_ALBUM  = 1 << 2,
    TAG_META_COVER  = 1 << 3,
    TAG_META_TEXT   = TAG_META_TITLE | TAG_META_ARTIST | TAG_META_ALBUM,
    TAG_META_ALL    = TAG_META_TEXT  | TAG_META_COVER
} TagMetaFlags;

void tag_meta_load(Track* track, const char* path, unsigned flags);
