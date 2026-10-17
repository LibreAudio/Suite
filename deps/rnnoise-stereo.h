// Copyright 2026 Klaus Scheuermann
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "rnnoise.h"

#ifdef __cplusplus
extern "C" {
#endif

// number of rnnoise bands, as reported through bandGains
#define LAS_RNNOISE_NB_BANDS 32

typedef struct {
    // per-frame limit on how fast band gains fall, upstream uses 0.6 (RT60 of 135 ms)
    float decay;
    // blend of the band gains with unity, as a dry/wet mix of the denoiser does: 1 leaves them as is, 0 removes them
    float depth;
    // lower limit for the applied band gains, after depth, 0 for none
    float minGain;
    // adds rnnoise's pitch reconstruction, estimated per channel
    int pitchFilter;
} LasRnnoiseStereoSettings;

// Denoises one 480-sample frame per channel with a shared, stereo-linked mask.
// Writes the applied gain of each band into bandGains, which may be NULL.
// Returns the higher voice activity probability of both channels.
float las_rnnoise_process_stereo(DenoiseState* left, DenoiseState* right,
                                 float* outLeft, float* outRight,
                                 const float* inLeft, const float* inRight,
                                 const LasRnnoiseStereoSettings* settings,
                                 float* bandGains);

#ifdef __cplusplus
}
#endif
