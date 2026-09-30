// Thank you tsoding:3

#include "fft.h"
#include "rstb_common.h"

#include <complex.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define FFT_PI 3.14159265358979323846f

typedef float complex Cplx;

struct FFT_Analyzer {
    float in_raw[FFT_SIZE];
    float in_win[FFT_SIZE];
    Cplx  out_raw[FFT_SIZE];
    float out_log[FFT_SIZE];
    float out_smooth[FFT_SIZE];
    float out_smear[FFT_SIZE];
};

FFT_Analyzer *fft_analyzer_create(void)
{
    return calloc(1, sizeof(FFT_Analyzer));
}

void fft_analyzer_destroy(FFT_Analyzer* a)
{
    free(a);
}

void fft_analyzer_reset(FFT_Analyzer* a)
{
    memset(a, 0, sizeof(*a));
}

// Ported from https://cp-algorithms.com/algebra/fft.html
static void fft(const float in[], Cplx out[], usize n)
{
    for (usize i = 0; i < n; i++) out[i] = in[i];

    for (usize i = 1, j = 0; i < n; i++) {
        usize bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) {
            Cplx tmp = out[i];
            out[i] = out[j];
            out[j] = tmp;
        }
    }

    for (usize len = 2; len <= n; len <<= 1) {
        f32 ang = 2 * FFT_PI / len;
        Cplx wlen = cosf(ang) + sinf(ang)*I;
        for (size_t i = 0; i < n; i += len) {
            Cplx w = 1;
            for (size_t j = 0; j < len/2; j++) {
                Cplx u = out[i + j];
                Cplx v = out[i + j + len/2]*w;
                out[i + j] = u + v;
                out[i + j + len/2] = u - v;
                w *= wlen;
            }
        }
    }
}

static inline float amp(Cplx z)
{
    f32 re = crealf(z), im = cimagf(z);
    return logf(re*re + im*im);
}

void fft_analyzer_push(FFT_Analyzer* a, f32 sample)
{
    RORI_ASSERT(a != NULL && "dummy dumb dumb");
    memmove(a->in_raw, a->in_raw + 1, (FFT_SIZE - 1)*sizeof(a->in_raw[0]));
    a->in_raw[FFT_SIZE - 1] = sample;
}

size_t fft_analyzer_analyze(FFT_Analyzer* a, f32 dt)
{
    RORI_ASSERT(a != NULL && "dummy dumb dumb");
    // Hann window
    for (size_t i = 0; i < FFT_SIZE; ++i) {
        float t = (float)i/(FFT_SIZE - 1);
        a->in_win[i] = a->in_raw[i]*(0.5f - 0.5f*cosf(2*FFT_PI*t));
    }

    fft(a->in_win, a->out_raw, FFT_SIZE);

    // Squash into logarithmic scale
    f32 step = 1.06f;
    f32 lowf = 1.0f;
    usize m = 0;
    f32 max_amp = 1.0f;
    for (f32 f = lowf; (usize)f < FFT_SIZE/2; f = ceilf(f*step)) {
        f32 f1 = ceilf(f*step);
        f32 best = 0.0f;
        for (usize q = (usize)f; q < FFT_SIZE/2 && q < (usize)f1; ++q) {
            f32 b = amp(a->out_raw[q]);
            if (b > best) best = b;
        }
        if (max_amp < best) max_amp = best;
        a->out_log[m++] = best;
    }

    // Normalize to 0..1
    for (usize i = 0; i < m; ++i) a->out_log[i] /= max_amp;

    // Smooth and smear
    for (usize i = 0; i < m; ++i) {
        f32 smoothness = 8, smearness = 3;
        a->out_smooth[i] += (a->out_log[i] - a->out_smooth[i])*smoothness*dt;
        a->out_smear[i]  += (a->out_smooth[i] - a->out_smear[i])*smearness*dt;
    }

    return m;
}

const f32 *fft_analyzer_smooth(const FFT_Analyzer* a) { return a->out_smooth; }
const f32 *fft_analyzer_smear(const FFT_Analyzer* a)  { return a->out_smear; }
