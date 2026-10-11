#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define COBJMACROS
#include <windows.h>
#include <roapi.h>
#include <winstring.h>
#include <inspectable.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "media.h"

extern void *GetWindowHandle(void);

#define INSPECTABLE_METHODS(T)                                                 \
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(T *, REFIID, void **);         \
    ULONG   (STDMETHODCALLTYPE *AddRef)(T *);                                  \
    ULONG   (STDMETHODCALLTYPE *Release)(T *);                                 \
    HRESULT (STDMETHODCALLTYPE *GetIids)(T *, ULONG *, IID **);                \
    HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(T *, HSTRING *);          \
    HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(T *, TrustLevel *);

#ifndef E_NOT_VALID_STATE
#define E_NOT_VALID_STATE ((HRESULT)0x8007139FL)
#endif

typedef BYTE wrt_bool;

// Windows.Media.MediaPlaybackStatus / MediaPlaybackType 
enum { PLAYBACK_CLOSED = 0, PLAYBACK_CHANGING = 1, PLAYBACK_STOPPED = 2,
       PLAYBACK_PLAYING = 3, PLAYBACK_PAUSED = 4 };
enum { PLAYBACKTYPE_UNKNOWN = 0, PLAYBACKTYPE_MUSIC = 1 };

typedef struct MyStreamRef MyStreamRef;
typedef struct MyUri MyUri;
typedef struct MyMusicProps MyMusicProps;
typedef struct MyMusicProps2 MyMusicProps2;
typedef struct MyUpdater MyUpdater;
typedef struct MySMTC MySMTC;
typedef struct MyInterop MyInterop;
typedef struct MyUriFactory MyUriFactory;
typedef struct MyStreamRefStatics MyStreamRefStatics;

// Windows.Storage.Streams.IRandomAccessStreamReference (only Release used)
typedef struct { INSPECTABLE_METHODS(MyStreamRef) } MyStreamRefVtbl;
struct MyStreamRef { const MyStreamRefVtbl *lpVtbl; };

// Windows.Foundation.IUriRuntimeClass (only Release used)
typedef struct { INSPECTABLE_METHODS(MyUri) } MyUriVtbl;
struct MyUri { const MyUriVtbl *lpVtbl; };

// Windows.Foundation.IUriRuntimeClassFactory
typedef struct {
    INSPECTABLE_METHODS(MyUriFactory)
    HRESULT (STDMETHODCALLTYPE *CreateUri)(MyUriFactory *, HSTRING, MyUri **);
    HRESULT (STDMETHODCALLTYPE *CreateWithRelativeUri)(MyUriFactory *, HSTRING, HSTRING, MyUri **);
} MyUriFactoryVtbl;
struct MyUriFactory { const MyUriFactoryVtbl *lpVtbl; };

// Windows.Storage.Streams.IRandomAccessStreamReferenceStatics
typedef struct {
    INSPECTABLE_METHODS(MyStreamRefStatics)
    HRESULT (STDMETHODCALLTYPE *CreateFromFile)(MyStreamRefStatics *, void *, MyStreamRef **);
    HRESULT (STDMETHODCALLTYPE *CreateFromUri)(MyStreamRefStatics *, MyUri *, MyStreamRef **);
    HRESULT (STDMETHODCALLTYPE *CreateFromStream)(MyStreamRefStatics *, void *, MyStreamRef **);
} MyStreamRefStaticsVtbl;
struct MyStreamRefStatics { const MyStreamRefStaticsVtbl *lpVtbl; };

// Windows.Media.IMusicDisplayProperties
typedef struct {
    INSPECTABLE_METHODS(MyMusicProps)
    HRESULT (STDMETHODCALLTYPE *get_Title)(MyMusicProps *, HSTRING *);
    HRESULT (STDMETHODCALLTYPE *put_Title)(MyMusicProps *, HSTRING);
    HRESULT (STDMETHODCALLTYPE *get_AlbumArtist)(MyMusicProps *, HSTRING *);
    HRESULT (STDMETHODCALLTYPE *put_AlbumArtist)(MyMusicProps *, HSTRING);
    HRESULT (STDMETHODCALLTYPE *get_Artist)(MyMusicProps *, HSTRING *);
    HRESULT (STDMETHODCALLTYPE *put_Artist)(MyMusicProps *, HSTRING);
} MyMusicPropsVtbl;
struct MyMusicProps { const MyMusicPropsVtbl *lpVtbl; };

// Windows.Media.IMusicDisplayProperties2 (album title lives here)
typedef struct {
    INSPECTABLE_METHODS(MyMusicProps2)
    HRESULT (STDMETHODCALLTYPE *get_AlbumTitle)(MyMusicProps2 *, HSTRING *);
    HRESULT (STDMETHODCALLTYPE *put_AlbumTitle)(MyMusicProps2 *, HSTRING);
    HRESULT (STDMETHODCALLTYPE *get_TrackNumber)(MyMusicProps2 *, UINT32 *);
    HRESULT (STDMETHODCALLTYPE *put_TrackNumber)(MyMusicProps2 *, UINT32);
    HRESULT (STDMETHODCALLTYPE *get_Genres)(MyMusicProps2 *, void **);
} MyMusicProps2Vtbl;
struct MyMusicProps2 { const MyMusicProps2Vtbl *lpVtbl; };

// Windows.Media.ISystemMediaTransportControlsDisplayUpdater
typedef struct {
    INSPECTABLE_METHODS(MyUpdater)
    HRESULT (STDMETHODCALLTYPE *get_Type)(MyUpdater *, int *);
    HRESULT (STDMETHODCALLTYPE *put_Type)(MyUpdater *, int);
    HRESULT (STDMETHODCALLTYPE *get_AppMediaId)(MyUpdater *, HSTRING *);
    HRESULT (STDMETHODCALLTYPE *put_AppMediaId)(MyUpdater *, HSTRING);
    HRESULT (STDMETHODCALLTYPE *get_Thumbnail)(MyUpdater *, MyStreamRef **);
    HRESULT (STDMETHODCALLTYPE *put_Thumbnail)(MyUpdater *, MyStreamRef *);
    HRESULT (STDMETHODCALLTYPE *get_MusicProperties)(MyUpdater *, MyMusicProps **);
    HRESULT (STDMETHODCALLTYPE *get_VideoProperties)(MyUpdater *, void **);
    HRESULT (STDMETHODCALLTYPE *get_ImageProperties)(MyUpdater *, void **);
    HRESULT (STDMETHODCALLTYPE *CopyFromFileAsync)(MyUpdater *, int, void *, void **);
    HRESULT (STDMETHODCALLTYPE *ClearAll)(MyUpdater *);
    HRESULT (STDMETHODCALLTYPE *Update)(MyUpdater *);
} MyUpdaterVtbl;
struct MyUpdater { const MyUpdaterVtbl *lpVtbl; };

// Windows.Media.ISystemMediaTransportControls
typedef struct {
    INSPECTABLE_METHODS(MySMTC)
    HRESULT (STDMETHODCALLTYPE *get_PlaybackStatus)(MySMTC *, int *);
    HRESULT (STDMETHODCALLTYPE *put_PlaybackStatus)(MySMTC *, int);
    HRESULT (STDMETHODCALLTYPE *get_DisplayUpdater)(MySMTC *, MyUpdater **);
    HRESULT (STDMETHODCALLTYPE *get_SoundLevel)(MySMTC *, int *);
    HRESULT (STDMETHODCALLTYPE *get_IsEnabled)(MySMTC *, wrt_bool *);
    HRESULT (STDMETHODCALLTYPE *put_IsEnabled)(MySMTC *, wrt_bool);
    HRESULT (STDMETHODCALLTYPE *get_IsPlayEnabled)(MySMTC *, wrt_bool *);
    HRESULT (STDMETHODCALLTYPE *put_IsPlayEnabled)(MySMTC *, wrt_bool);
    HRESULT (STDMETHODCALLTYPE *get_IsStopEnabled)(MySMTC *, wrt_bool *);
    HRESULT (STDMETHODCALLTYPE *put_IsStopEnabled)(MySMTC *, wrt_bool);
    HRESULT (STDMETHODCALLTYPE *get_IsPauseEnabled)(MySMTC *, wrt_bool *);
    HRESULT (STDMETHODCALLTYPE *put_IsPauseEnabled)(MySMTC *, wrt_bool);
    HRESULT (STDMETHODCALLTYPE *get_IsRecordEnabled)(MySMTC *, wrt_bool *);
    HRESULT (STDMETHODCALLTYPE *put_IsRecordEnabled)(MySMTC *, wrt_bool);
    HRESULT (STDMETHODCALLTYPE *get_IsFastForwardEnabled)(MySMTC *, wrt_bool *);
    HRESULT (STDMETHODCALLTYPE *put_IsFastForwardEnabled)(MySMTC *, wrt_bool);
    HRESULT (STDMETHODCALLTYPE *get_IsRewindEnabled)(MySMTC *, wrt_bool *);
    HRESULT (STDMETHODCALLTYPE *put_IsRewindEnabled)(MySMTC *, wrt_bool);
    HRESULT (STDMETHODCALLTYPE *get_IsPreviousEnabled)(MySMTC *, wrt_bool *);
    HRESULT (STDMETHODCALLTYPE *put_IsPreviousEnabled)(MySMTC *, wrt_bool);
    HRESULT (STDMETHODCALLTYPE *get_IsNextEnabled)(MySMTC *, wrt_bool *);
    HRESULT (STDMETHODCALLTYPE *put_IsNextEnabled)(MySMTC *, wrt_bool);
} MySMTCVtbl;
struct MySMTC { const MySMTCVtbl *lpVtbl; };

// ISystemMediaTransportControlsInterop (activation factory of SMTC class)
typedef struct {
    INSPECTABLE_METHODS(MyInterop)
    HRESULT (STDMETHODCALLTYPE *GetForWindow)(MyInterop *, HWND, REFIID, void **);
} MyInteropVtbl;
struct MyInterop { const MyInteropVtbl *lpVtbl; };

static const GUID IID_My_Interop =
    {0xDDB0472D, 0xC911, 0x4A1F, {0x86, 0xD9, 0xDC, 0x3D, 0x71, 0xA9, 0x5F, 0x5A}};
static const GUID IID_My_SMTC =
    {0x99FA3FF4, 0x1742, 0x42A6, {0x90, 0x2E, 0x08, 0x7D, 0x41, 0xF9, 0x65, 0xEC}};
static const GUID IID_My_MusicProps2 =
    {0x00368462, 0x97D3, 0x44B9, {0xB0, 0x0F, 0x00, 0x8A, 0xFC, 0xEF, 0xAF, 0x18}};
static const GUID IID_My_UriFactory =
    {0x44A9796F, 0x723E, 0x4FDF, {0xA2, 0x18, 0x03, 0x3E, 0x75, 0xB0, 0xC0, 0x84}};
static const GUID IID_My_StreamRefStatics =
    {0x857309DC, 0x3FBF, 0x4E7D, {0x98, 0x6F, 0xEF, 0x3B, 0x1A, 0x07, 0xA9, 0x64}};

static MySMTC *g_smtc = NULL;
static int     g_ro_inited = 0;

static HSTRING hs_new(const wchar_t *s)
{
    HSTRING h = NULL;
    if (!s) s = L"";
    WindowsCreateString(s, (UINT32)wcslen(s), &h);
    return h;
}

static HRESULT get_factory(const wchar_t *class_name, REFIID iid, void **out)
{
    HSTRING cls = hs_new(class_name);
    HRESULT hr = RoGetActivationFactory(cls, iid, out);
    WindowsDeleteString(cls);
    return hr;
}

static int build_file_uri(const wchar_t *path, wchar_t *out, size_t out_cap)
{
    static const char keep[] = "/:.-_~";
    wchar_t full[1024];
    char    utf8[4096];
    size_t  n = 0;

    DWORD len = GetFullPathNameW(path, 1024, full, NULL);
    if (len == 0 || len >= 1024) return 0;
    if (!WideCharToMultiByte(CP_UTF8, 0, full, -1, utf8, sizeof utf8, NULL, NULL))
        return 0;

    const wchar_t *prefix = L"file:///";
    size_t plen = wcslen(prefix);
    if (out_cap < plen + 1) return 0;
    wcscpy(out, prefix);
    n = plen;

    for (const unsigned char *p = (const unsigned char *)utf8; *p; p++) {
        int plain = (*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'z') ||
                    (*p >= 'A' && *p <= 'Z') || (*p && strchr(keep, *p));
        if (*p == '\\') { if (n + 1 >= out_cap) return 0; out[n++] = L'/'; }
        else if (plain) { if (n + 1 >= out_cap) return 0; out[n++] = (wchar_t)*p; }
        else {
            static const wchar_t hex[] = L"0123456789ABCDEF";
            if (n + 3 >= out_cap) return 0;
            out[n++] = L'%';
            out[n++] = hex[*p >> 4];
            out[n++] = hex[*p & 15];
        }
    }
    out[n] = 0;
    return 1;
}

static HRESULT make_thumbnail(const wchar_t *path, MyStreamRef **out)
{
    static wchar_t uri[3 * 4096 + 16];
    MyUriFactory *uf = NULL;
    MyUri *u = NULL;
    MyStreamRefStatics *rs = NULL;
    HRESULT hr;

    *out = NULL;
    if (!build_file_uri(path, uri, sizeof uri / sizeof uri[0])) return E_INVALIDARG;

    hr = get_factory(L"Windows.Foundation.Uri", &IID_My_UriFactory, (void **)&uf);
    if (FAILED(hr)) return hr;
    HSTRING huri = hs_new(uri);
    hr = uf->lpVtbl->CreateUri(uf, huri, &u);
    WindowsDeleteString(huri);
    uf->lpVtbl->Release(uf);
    if (FAILED(hr)) return hr;

    hr = get_factory(L"Windows.Storage.Streams.RandomAccessStreamReference",
                     &IID_My_StreamRefStatics, (void **)&rs);
    if (FAILED(hr)) { u->lpVtbl->Release(u); return hr; }
    hr = rs->lpVtbl->CreateFromUri(rs, u, out);
    rs->lpVtbl->Release(rs);
    u->lpVtbl->Release(u);
    return hr;
}

static wchar_t *utf8_to_wide(const char *s)
{
    if (!s) return NULL;
    int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
    if (n <= 0) return NULL;
    wchar_t *w = (wchar_t *)malloc((size_t)n * sizeof(wchar_t));
    if (w) MultiByteToWideChar(CP_UTF8, 0, s, -1, w, n);
    return w;
}

int media_init()
{
    if (g_smtc) return S_OK;

    HRESULT hr = RoInitialize(RO_INIT_SINGLETHREADED);
    g_ro_inited = SUCCEEDED(hr);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE)
        return hr;

    MyInterop *interop = NULL;
    hr = get_factory(L"Windows.Media.SystemMediaTransportControls",
                     &IID_My_Interop, (void **)&interop);
    if (FAILED(hr)) return hr;

    hr = interop->lpVtbl->GetForWindow(interop, (HWND)GetWindowHandle(), &IID_My_SMTC, (void **)&g_smtc);
    interop->lpVtbl->Release(interop);
    if (FAILED(hr)) { g_smtc = NULL; return hr; }

    g_smtc->lpVtbl->put_IsEnabled(g_smtc, 1);
    g_smtc->lpVtbl->put_IsPlayEnabled(g_smtc, 1);
    g_smtc->lpVtbl->put_IsPauseEnabled(g_smtc, 1);
    g_smtc->lpVtbl->put_PlaybackStatus(g_smtc, PLAYBACK_STOPPED);
    return S_OK;
}

int media_set_track(const wchar_t *title, const wchar_t *artist,
                   const wchar_t *album, const wchar_t *cover_path)
{
    if (!g_smtc) return E_NOT_VALID_STATE;

    MyUpdater *up = NULL;
    MyMusicProps *mp = NULL;
    HRESULT hr = g_smtc->lpVtbl->get_DisplayUpdater(g_smtc, &up);
    if (FAILED(hr)) return hr;

    up->lpVtbl->put_Type(up, PLAYBACKTYPE_MUSIC);

    hr = up->lpVtbl->get_MusicProperties(up, &mp);
    if (SUCCEEDED(hr)) {
        HSTRING t = hs_new(title), a = hs_new(artist);
        mp->lpVtbl->put_Title(mp, t);
        mp->lpVtbl->put_Artist(mp, a);
        WindowsDeleteString(t);
        WindowsDeleteString(a);

        MyMusicProps2 *mp2 = NULL;
        if (SUCCEEDED(mp->lpVtbl->QueryInterface(mp, &IID_My_MusicProps2, (void **)&mp2))) {
            HSTRING al = hs_new(album);
            mp2->lpVtbl->put_AlbumTitle(mp2, al);
            WindowsDeleteString(al);
            mp2->lpVtbl->Release(mp2);
        }
        mp->lpVtbl->Release(mp);
    }

    MyStreamRef *thumb = NULL;
    if (cover_path && *cover_path)
        make_thumbnail(cover_path, &thumb);
    up->lpVtbl->put_Thumbnail(up, thumb);
    if (thumb) thumb->lpVtbl->Release(thumb);

    hr = up->lpVtbl->Update(up);
    up->lpVtbl->Release(up);

    g_smtc->lpVtbl->put_PlaybackStatus(g_smtc, PLAYBACK_PLAYING);
    return hr;
}

int media_set_track_utf8(const char *title, const char *artist,
                        const char *album, const char *cover_path)
{
    wchar_t *t = utf8_to_wide(title), *a = utf8_to_wide(artist);
    wchar_t *al = utf8_to_wide(album), *c = utf8_to_wide(cover_path);
    int hr = media_set_track(t, a, al, c);
    free(t); free(a); free(al); free(c);
    return hr;
}

int media_set_playing(int playing)
{
    if (!g_smtc) return E_NOT_VALID_STATE;
    return g_smtc->lpVtbl->put_PlaybackStatus(g_smtc,
               playing ? PLAYBACK_PLAYING : PLAYBACK_PAUSED);
}

int media_clear(void)
{
    if (!g_smtc) return E_NOT_VALID_STATE;
    MyUpdater *up = NULL;
    HRESULT hr = g_smtc->lpVtbl->get_DisplayUpdater(g_smtc, &up);
    if (SUCCEEDED(hr)) {
        up->lpVtbl->ClearAll(up);
        hr = up->lpVtbl->Update(up);
        up->lpVtbl->Release(up);
    }
    g_smtc->lpVtbl->put_PlaybackStatus(g_smtc, PLAYBACK_CLOSED);
    return hr;
}

void media_shutdown(void)
{
    if (g_smtc) {
        g_smtc->lpVtbl->put_IsEnabled(g_smtc, 0);
        g_smtc->lpVtbl->Release(g_smtc);
        g_smtc = NULL;
    }
    if (g_ro_inited) { RoUninitialize(); g_ro_inited = 0; }
}
#else
int  media_init(void* hwnd) {}
int  media_set_track(
    const wchar_t* title, 
    const wchar_t* artist,
    const wchar_t* album, 
    const wchar_t* cover_path
) {}
int  media_set_track_utf8(
    const char* title, 
    const char* artist,
    const char* album, 
    const char* cover_path
) {}
int  media_set_playing(int playing) {}
int  media_clear(void) {}
void media_shutdown(void) {}
#endif
