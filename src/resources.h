#pragma once

#include <raylib.h>

#define MAX_RESOURCE_TEXTURE 64
#define MAX_RESOURCE_STRING 128

Texture* resource_generate_qr_code(const char* link);

Texture* resource_fetch_texture(const char* path);
Texture* resource_add_texture(const char* key, Texture texture);
Texture* resource_get_texture(const char* key);

void     resource_reset();
