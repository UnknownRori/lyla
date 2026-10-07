#include "./fold.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define UTF8_INVALID 0xFFFFFFFFu

typedef struct { uint16_t lo, hi; char base; } FoldRange;

static const FoldRange fold_ranges[] = {
    // Latin-1 Supplement
    {0xC0, 0xC5, 'a'}, {0xC7, 0xC7, 'c'}, {0xC8, 0xCB, 'e'}, {0xCC, 0xCF, 'i'},
    {0xD0, 0xD0, 'd'}, {0xD1, 0xD1, 'n'}, {0xD2, 0xD6, 'o'}, {0xD8, 0xD8, 'o'},
    {0xD9, 0xDC, 'u'}, {0xDD, 0xDD, 'y'},
    {0xE0, 0xE5, 'a'}, {0xE7, 0xE7, 'c'}, {0xE8, 0xEB, 'e'}, {0xEC, 0xEF, 'i'},
    {0xF0, 0xF0, 'd'}, {0xF1, 0xF1, 'n'}, {0xF2, 0xF6, 'o'}, {0xF8, 0xF8, 'o'},
    {0xF9, 0xFC, 'u'}, {0xFD, 0xFD, 'y'}, {0xFF, 0xFF, 'y'},
    // Latin Extended-A
    {0x100, 0x105, 'a'}, {0x106, 0x10D, 'c'}, {0x10E, 0x111, 'd'},
    {0x112, 0x11B, 'e'}, {0x11C, 0x123, 'g'}, {0x124, 0x127, 'h'},
    {0x128, 0x131, 'i'}, {0x134, 0x135, 'j'}, {0x136, 0x138, 'k'},
    {0x139, 0x142, 'l'}, {0x143, 0x14B, 'n'}, {0x14C, 0x151, 'o'},
    {0x154, 0x159, 'r'}, {0x15A, 0x161, 's'}, {0x162, 0x167, 't'},
    {0x168, 0x173, 'u'}, {0x174, 0x175, 'w'}, {0x176, 0x178, 'y'},
    {0x179, 0x17E, 'z'}, {0x17F, 0x17F, 's'},
};

static uint32_t utf8_next(const unsigned char** pp)
{
    const unsigned char* p = *pp;
    uint32_t c = p[0], cp;
    int n;

    if (c < 0x80) { *pp = p + 1; return c; }
    else if ((c & 0xE0) == 0xC0) { n = 1; cp = c & 0x1F; }
    else if ((c & 0xF0) == 0xE0) { n = 2; cp = c & 0x0F; }
    else if ((c & 0xF8) == 0xF0) { n = 3; cp = c & 0x07; }
    else { *pp = p + 1; return UTF8_INVALID; }

    for (int i = 1; i <= n; i++) {
        if ((p[i] & 0xC0) != 0x80) { *pp = p + 1; return UTF8_INVALID; }
        cp = (cp << 6) | (p[i] & 0x3F);
    }
    *pp = p + n + 1;
    return cp > 0x10FFFF ? UTF8_INVALID : cp;
}

static size_t utf8_put(uint32_t cp, char* out)
{
    if (cp < 0x80)    { out[0] = (char)cp; return 1; }
    if (cp < 0x800)   { out[0] = (char)(0xC0 | (cp >> 6));
                        out[1] = (char)(0x80 | (cp & 0x3F)); return 2; }
    if (cp < 0x10000) { out[0] = (char)(0xE0 | (cp >> 12));
                        out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
                        out[2] = (char)(0x80 | (cp & 0x3F)); return 3; }
    out[0] = (char)(0xF0 | (cp >> 18));
    out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
    out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
    out[3] = (char)(0x80 | (cp & 0x3F));
    return 4;
}

static size_t fold_cp(uint32_t cp, char rep[2])
{
    if (cp < 0x80) {
        rep[0] = (cp >= 'A' && cp <= 'Z') ? (char)(cp + 32) : (char)cp;
        return 1;
    }

    switch (cp) {
        case 0xC6: case 0xE6:   rep[0] = 'a'; rep[1] = 'e'; return 2; // Æ æ
        case 0xDE: case 0xFE:   rep[0] = 't'; rep[1] = 'h'; return 2; // Þ þ
        case 0xDF:              rep[0] = 's'; rep[1] = 's'; return 2; // ß
        case 0x132: case 0x133: rep[0] = 'i'; rep[1] = 'j'; return 2; // Ĳ ĳ
        case 0x152: case 0x153: rep[0] = 'o'; rep[1] = 'e'; return 2; // Œ œ
        default: break;
    }

    for (size_t i = 0; i < sizeof(fold_ranges) / sizeof(fold_ranges[0]); i++) {
        if (cp >= fold_ranges[i].lo && cp <= fold_ranges[i].hi) {
            rep[0] = fold_ranges[i].base;
            return 1;
        }
    }

    uint32_t lower = 0;
    if (cp >= 0x410 && cp <= 0x42F)      lower = cp + 0x20;  // А-Я
    else if (cp >= 0x400 && cp <= 0x40F) lower = cp + 0x50;  // Ѐ-Џ
    else if (cp >= 0x391 && cp <= 0x3A9 && cp != 0x3A2) lower = cp + 0x20; // Α-Ω
    if (lower) {
        char tmp[4];
        size_t n = utf8_put(lower, tmp);
        rep[0] = tmp[0]; rep[1] = tmp[1];
        return n;
    }

    return 0;
}

char* text_fold_dup(const char* s)
{
    if (!s) return NULL;

    size_t n = strlen(s);
    char* out = (char*)malloc(n + 1);
    if (!out) return NULL;

    size_t o = 0;
    const unsigned char* p = (const unsigned char*)s;
    while (*p) {
        uint32_t cp = utf8_next(&p);
        if (cp == UTF8_INVALID) continue;
        if (cp >= 0x300 && cp <= 0x36F) continue;

        char rep[2];
        size_t k = fold_cp(cp, rep);
        if (k) { memcpy(out + o, rep, k); o += k; }
        else   { o += utf8_put(cp, out + o); }
    }
    out[o] = '\0';
    return out;
}
