#pragma once
#include <stdbool.h>
#include "music/track.h"

void discord_init(const char* client_id);

void discord_update_presence(Track* track, bool paused);

void discord_update(void);

void discord_shutdown(void);
