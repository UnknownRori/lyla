#pragma once
#include <stddef.h>
#include "types.h"

#define FFT_SIZE (1 << 13)

typedef struct FFT_Analyzer FFT_Analyzer;

FFT_Analyzer* fft_analyzer_create(void);
void          fft_analyzer_destroy(FFT_Analyzer *a);

void fft_analyzer_reset(FFT_Analyzer* a);

void fft_analyzer_push(FFT_Analyzer* a, f32 sample);

size_t fft_analyzer_analyze(FFT_Analyzer* a, float dt);

const float *fft_analyzer_smooth(const FFT_Analyzer* a);
const float *fft_analyzer_smear(const FFT_Analyzer* a);
