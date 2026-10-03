#include <string.h>
#include <stdlib.h>
#include "tag.h"
#include <tag_c.h>
#include "resources.h"

typedef struct {
    unsigned char *data;
    int size;
    char ext[8];
} CoverArt;

static int taglib_load_cover(TagLib_File *f, CoverArt *art) {
    memset(art, 0, sizeof *art);

    TagLib_Complex_Property_Attribute ***props =
        taglib_complex_property_get(f, "PICTURE");
    if (!props) return 0;

    int best_is_front = 0;

    for (int i = 0; props[i]; i++) {
        const char *mime = NULL, *ptype = NULL;
        const char *bytes = NULL;
        unsigned int size = 0;

        for (TagLib_Complex_Property_Attribute **a = props[i]; *a; a++) {
            const char *key = (*a)->key;
            TagLib_Variant *v = &(*a)->value;

            if (!strcmp(key, "data") && v->type == TagLib_Variant_ByteVector) {
                bytes = v->value.byteVectorValue;
                size  = v->size;
            } else if (!strcmp(key, "mimeType") && v->type == TagLib_Variant_String) {
                mime = v->value.stringValue;
            } else if (!strcmp(key, "pictureType") && v->type == TagLib_Variant_String) {
                ptype = v->value.stringValue;
            }
        }
        if (!bytes || size == 0) continue;

        int is_front = ptype && !strcmp(ptype, "Front Cover");
        if (art->data && !(is_front && !best_is_front)) continue;

        free(art->data);
        art->data = malloc(size);
        if (!art->data) break;
        memcpy(art->data, bytes, size);
        art->size = (int)size;
        best_is_front = is_front;

        int png = (mime && strstr(mime, "png")) || (art->data[0] == 0x89);
        strcpy(art->ext, png ? ".png" : ".jpg");
    }

    taglib_complex_property_free(props);
    return art->data != NULL;
}

static Texture2D cover_to_texture(const CoverArt *art, int max_dim) {
    Texture2D tex = {0};
    if (!art->data) return tex;

    Image img = LoadImageFromMemory(art->ext, art->data, art->size);
    if (!IsImageValid(img)) return tex;

    if (img.width > max_dim || img.height > max_dim) {
        float s = (float)max_dim / (img.width > img.height ? img.width : img.height);
        ImageResize(&img, (int)(img.width * s), (int)(img.height * s));
    }

    tex = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    return tex;
}

void tag_meta_load(Track* track, const char* path)
{
    TagLib_File* f = taglib_file_new(path);
    if (!f || !taglib_file_is_valid(f)) { if (f) taglib_file_free(f); return; }
    TagLib_Tag *t = taglib_file_tag(f);
    #define SET_WHEN_NEEDED(X, Y) do { if (strcmp(Y(t), "") != 0) track->X = strdup(Y(t)); } while (0)
    SET_WHEN_NEEDED(title, taglib_tag_title);
    SET_WHEN_NEEDED(artist, taglib_tag_artist);
    SET_WHEN_NEEDED(album, taglib_tag_album);

    CoverArt tmp = {0};
    if (taglib_load_cover(f, &tmp)) {
        Texture2D cover = cover_to_texture(&tmp, 256);
        track->thumbnail = resource_add_texture(path, cover);
    }

    taglib_tag_free_strings();
    taglib_file_free(f);
}
