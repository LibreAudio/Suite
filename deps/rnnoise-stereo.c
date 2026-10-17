// Copyright 2026 Klaus Scheuermann
// SPDX-License-Identifier: GPL-3.0-or-later

// Compiles upstream's implementation here to access its private FFT analysis
// and synthesis helpers without changing the submodule.
// Replaces denoise.c in the las-rnnoise library, so the public rnnoise API stays available.
#include "rnnoise/src/denoise.c"
#include "rnnoise-stereo.h"

_Static_assert(NB_BANDS == LAS_RNNOISE_NB_BANDS, "rnnoise band count changed");

float las_rnnoise_process_stereo(DenoiseState* left, DenoiseState* right,
                                 float* outLeft, float* outRight,
                                 const float* inLeft, const float* inRight,
                                 const LasRnnoiseStereoSettings* settings,
                                 float* bandGains)
{
    DenoiseState* states[2] = {left, right};
    const float* inputs[2] = {inLeft, inRight};
    float* outputs[2] = {outLeft, outRight};
    kiss_fft_cpx X[2][FREQ_SIZE], P[2][FREQ_SIZE];
    float Ex[2][NB_BANDS], Ep[2][NB_BANDS], Exp[2][NB_BANDS];
    float gains[2][NB_BANDS] = {{0}}, raw[NB_BANDS], linked[NB_BANDS], applied[NB_BANDS], gf[FREQ_SIZE] = {0};
    float vad = 0;
    static const float a_hp[2] = {-1.99599f, 0.99600f};
    static const float b_hp[2] = {-2, 1};

    // Separate analysis avoids cancellation for wide/anti-phase inputs.
    // The less attenuated estimate wins in each band, protecting speech on
    // either side; precisely the same resulting mask is applied to both.
    for (int c = 0; c < 2; ++c) {
        float x[FRAME_SIZE], features[NB_FEATURES], probability = 0;
        DenoiseState* st = states[c];
        rnn_biquad(x, st->mem_hp_x, inputs[c], b_hp, a_hp, FRAME_SIZE);
        const int silence = rnn_compute_frame_features(st, X[c], P[c], Ex[c], Ep[c], Exp[c], features, x);
        if (!silence)
            compute_rnn(&st->model, &st->rnn, gains[c], &probability, features, st->arch);
        vad = MAX16(vad, probability);
    }
    for (int i = 0; i < NB_BANDS; ++i) {
        raw[i] = MAX16(gains[0][i], gains[1][i]);
        linked[i] = MAX16(raw[i], settings->decay * MAX16(left->lastg[i], right->lastg[i]));
        // Use joint energy for release compensation: a silent channel must
        // not hold the other channel's suppression open.
        const float previous = left->delayed_Ex[i] + right->delayed_Ex[i];
        const float current = Ex[0][i] + Ex[1][i];
        left->lastg[i] = right->lastg[i] =
            MIN16(1.f, linked[i] * (previous + 1e-3f) / (current + 1e-3f));
        // the floor only limits what is applied, release tracking stays unchanged
        applied[i] = MAX16(linked[i], settings->minGain);
        if (bandGains != NULL)
            bandGains[i] = applied[i];
    }
    interp_band_gain(gf, applied);
    for (int c = 0; c < 2; ++c) {
        DenoiseState* st = states[c];
        // Pitch reconstruction is estimated per channel, so it is optional:
        // without it, a common multiplicative mask preserves the stereo phase/image.
        if (settings->pitchFilter)
            rnn_pitch_filter(st->delayed_X, st->delayed_P, st->delayed_Ex, st->delayed_Ep, st->delayed_Exp, raw);
        for (int i = 0; i < FREQ_SIZE; ++i) {
            st->delayed_X[i].r *= gf[i];
            st->delayed_X[i].i *= gf[i];
        }
        frame_synthesis(st, outputs[c], st->delayed_X);
        RNN_COPY(st->delayed_X, X[c], FREQ_SIZE);
        RNN_COPY(st->delayed_P, P[c], FREQ_SIZE);
        RNN_COPY(st->delayed_Ex, Ex[c], NB_BANDS);
        RNN_COPY(st->delayed_Ep, Ep[c], NB_BANDS);
        RNN_COPY(st->delayed_Exp, Exp[c], NB_BANDS);
    }
    return vad;
}
