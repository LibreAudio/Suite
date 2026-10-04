// Libre Audio Suite
// Copyright (C) 2013, 2019 Robin Gareus <robin@gareus.org>
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <fftw3.h>

class FFTAnalysis
{
public:
    static constexpr const float kSmallestValue = -96.f;

    FFTAnalysis()
    {
        _window_size = 0;
    }

    ~FFTAnalysis()
    {
        free();
    }

    void init(uint32_t window_size, double rate, double fps);
    void free();
    bool run(uint32_t n_samples, float const* data);
    float powerAtBin(int b) const;
    float freqAtBin(int b) const;

private:
    enum window_t {
        W_HANN = 0,
        W_HAMMMIN,
        W_NUTTALL,
        W_BLACKMAN_NUTTALL,
        W_BLACKMAN_HARRIS,
        W_FLAT_TOP
    };

    uint32_t   _window_size;
    window_t   _window_type;
    uint32_t   _data_size;
    double     _rate;
    double     _freq_per_bin;
    double     _phasediff_step;
    float*     _window;
    float*     _fft_in;
    float*     _fft_out;
    float*     _power;
    float*     _phase;
    float*     _phase_h;
    fftwf_plan _fftplan;

    float*   _ringbuf;
    uint32_t _rboff;
    uint32_t _smps;
    uint32_t _sps;
    uint32_t _step;
    double   _phasediff_bin;

    float* _genWindow();
    void _analyze();
    void _reset();
    bool _run(uint32_t n_samples, float const* data);
};
