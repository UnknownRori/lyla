#include <raylib.h>
#include <stdlib.h>
#include <string.h>
#include <rstb_common.h>

#include "track.h"
#include "resources.h"

bool track_load(Track* track, const char* path)
{
    RORI_ASSERT(track != NULL && "dummy dumb dumb");
    memset(track, 0, sizeof(*track));

    track->music = LoadMusicStream(path);
    if (!IsMusicValid(track->music)) return false;

    track->path = strdup(GetFileName(path));
    track->thumbnail = NULL;
    return true;
}

void track_unload(Track* track)
{
    RORI_ASSERT(track != NULL && "dummy dumb dumb");
    if (IsMusicValid(track->music)) {
        UnloadMusicStream(track->music);
    }

    #define UNLOAD_STRING(X) do { if (X != NULL) free(X);  } while (0)

    UNLOAD_STRING(track->path);
    UNLOAD_STRING(track->title);
    UNLOAD_STRING(track->artist);
    UNLOAD_STRING(track->album);
}

bool track_set_thumbnail(Track* track, const char* path)
{
    RORI_ASSERT(track != NULL && "dummy dumb dumb");
    track->thumbnail = resource_fetch_texture(path);
    if (track->thumbnail == NULL) return false;
    return true;
}
bool track_set_qrcode(Track* track, const char* link)
{
    RORI_ASSERT(track != NULL && "dummy dumb dumb");
    track->qrcode = resource_generate_qr_code(link);
    if (track->qrcode == NULL) return false;
    return true;
}
