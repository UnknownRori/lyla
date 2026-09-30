#include <raylib.h>
#include <string.h>
#include <rstb_common.h>

#include "thirdparty/qrcodegen.h"
#include "types.h"
#include "resources.h"

typedef struct TextureStorage {
    const char* key;
    Texture texture;
} TextureStorage;

static TextureStorage _texture[MAX_RESOURCE_TEXTURE] = {0};
static usize _index = 0;

Texture* resource_generate_qr_code(const char* link)
{
    RORI_ASSERT(link != NULL && "Dummy dumb dumb");
    Texture* qr = resource_get_texture(link);
    if (qr) return qr;

    uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
    uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];
    
    // Take the payload data and encode it into a QR code array
    bool ok = qrcodegen_encodeText(link, tempBuffer, qrcode, qrcodegen_Ecc_LOW, 
                                   qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, 
                                   qrcodegen_Mask_AUTO, true);
    if (!ok) return false;
    
    int size = qrcodegen_getSize(qrcode);
    int scale = 4;
    int padding = 4;
    int img_size = (size + padding * 2) * scale;
    
    Image qr_img = GenImageColor(img_size, img_size, WHITE);
    
    // Read the encoded modules and draw them onto the Raylib image
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            if (qrcodegen_getModule(qrcode, x, y)) {
                ImageDrawRectangle(&qr_img, (x + padding) * scale, (y + padding) * scale, scale, scale, BLACK);
            }
        }
    }
    
    Texture texture = LoadTextureFromImage(qr_img);
    UnloadImage(qr_img);
    
    return resource_add_texture(link, texture);
}

Texture* resource_fetch_texture(const char* path)
{
    Texture* found = resource_get_texture(path);
    if (found) return found;
    resource_add_texture(path, LoadTexture(path));
}
Texture* resource_add_texture(const char* key, Texture texture)
{
    RORI_ASSERT(key != NULL && "Dummy dumb dumb");
    for (usize i = 0; i < _index; i++) {
        if (strcmp(key, _texture[i].key) == 0) return &_texture[i].texture;
    }

    _texture[_index].key     = strdup(key);
    _texture[_index].texture = texture;
    Texture* ptr = &_texture[_index].texture;
    _index += 1;
    return ptr;
}

Texture* resource_get_texture(const char* key)
{
    RORI_ASSERT(key != NULL && "Dummy dumb dumb");
    for (usize i = 0; i < _index; i++) {
        if (strcmp(key, _texture[i].key) == 0) return &_texture[i].texture;
    }
    return NULL;
}
void     resource_reset()
{
    for (usize i = 0; i < _index; i++) {
        if (IsTextureValid(_texture[i].texture)) {
            UnloadTexture(_texture[i].texture);
        }
    }
    _index = 0;
}
