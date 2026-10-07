#include "./playlist_search.h"
#include "utf8/fold.h"
#include <stdlib.h>
#include <string.h>

#define SEARCH_MAX_TOKENS 8

#define W_TITLE  8
#define W_ARTIST 6
#define W_ALBUM  4
#define W_FILE   2
#define BOUNDARY_BONUS 3

typedef struct {
    size_t idx;
    int    score;
} Scored;

static int score_field(const char* hay, const char* tok, int weight)
{
    if (!hay || !*hay) return 0;
    const char* f = strstr(hay, tok);
    if (!f) return 0;

    for (const char* q = f; q; q = strstr(q + 1, tok)) {
        if (q == hay || q[-1] == ' ' || q[-1] == '-' || q[-1] == '_' || q[-1] == '.')
            return weight + BOUNDARY_BONUS;
    }
    return weight;
}

static char* folded_file_stem(const char* path)
{
    if (!path) return NULL;
    const char* base = path;
    for (const char* p = path; *p; p++)
        if (*p == '/' || *p == '\\') base = p + 1;

    size_t len = strlen(base);
    const char* dot = strrchr(base, '.');
    if (dot && dot != base) len = (size_t)(dot - base);

    char* tmp = (char*)malloc(len + 1);
    if (!tmp) return NULL;
    memcpy(tmp, base, len);
    tmp[len] = '\0';

    char* folded = text_fold_dup(tmp);
    free(tmp);
    return folded;
}

static int cmp_scored(const void* a, const void* b)
{
    const Scored* x = (const Scored*)a;
    const Scored* y = (const Scored*)b;
    if (x->score != y->score) return y->score - x->score;
    return (x->idx > y->idx) - (x->idx < y->idx);
}

void playlist_search(const Playlist* playlist, const char* query, PlaylistSearchResult* out)
{
    out->indices = NULL;
    out->count = 0;
    if (!playlist || playlist->count == 0) return;

    out->indices = (size_t*)malloc(playlist->count * sizeof(size_t));
    if (!out->indices) return;

    char* q = query ? text_fold_dup(query) : NULL;
    char* tokens[SEARCH_MAX_TOKENS];
    size_t ntok = 0;
    if (q) {
        char* p = q;
        while (*p && ntok < SEARCH_MAX_TOKENS) {
            while (*p == ' ' || *p == '\t') p++;
            if (!*p) break;
            tokens[ntok++] = p;
            while (*p && *p != ' ' && *p != '\t') p++;
            if (*p) *p++ = '\0';
        }
    }

    if (ntok == 0) {
        for (size_t i = 0; i < playlist->count; i++) out->indices[i] = i;
        out->count = playlist->count;
        free(q);
        return;
    }

    Scored* scored = (Scored*)malloc(playlist->count * sizeof(Scored));
    if (!scored) { free(q); free(out->indices); out->indices = NULL; return; }
    size_t nscored = 0;

    for (size_t i = 0; i < playlist->count; i++) {
        const Track* t = &playlist->items[i];

        char* f_title  = text_fold_dup(t->title);
        char* f_artist = text_fold_dup(t->artist);
        char* f_album  = text_fold_dup(t->album);
        char* f_file   = folded_file_stem(t->path);

        int total = 0;
        bool all = true;
        for (size_t k = 0; k < ntok && all; k++) {
            int best = 0, s;
            if ((s = score_field(f_title,  tokens[k], W_TITLE))  > best) best = s;
            if ((s = score_field(f_artist, tokens[k], W_ARTIST)) > best) best = s;
            if ((s = score_field(f_album,  tokens[k], W_ALBUM))  > best) best = s;
            if ((s = score_field(f_file,   tokens[k], W_FILE))   > best) best = s;
            if (best == 0) all = false;
            total += best;
        }

        if (all) {
            scored[nscored].idx = i;
            scored[nscored].score = total;
            nscored++;
        }

        free(f_title); free(f_artist); free(f_album); free(f_file);
    }

    qsort(scored, nscored, sizeof(Scored), cmp_scored);
    for (size_t i = 0; i < nscored; i++) out->indices[i] = scored[i].idx;
    out->count = nscored;

    free(scored);
    free(q);
}

void playlist_search_result_free(PlaylistSearchResult* result)
{
    if (!result) return;
    free(result->indices);
    result->indices = NULL;
    result->count = 0;
}
