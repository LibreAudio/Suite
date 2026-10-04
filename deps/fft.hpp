// Libre Audio Suite
// Copyright (C) 2013, 2019 Robin Gareus <robin@gareus.org>
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

struct kiss_fft_cpx_;
typedef struct kiss_fft_state kiss_fft_state;

class FFTAnalysis
{
public:
    static constexpr const float kSmallestValue = -96.f;

    FFTAnalysis()
    {
        _fft = nullptr;
    }

    ~FFTAnalysis()
    {
        free();
    }

    void init(uint32_t window_size, double rate);
    void free();
    void run(const float* data);
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

    uint32_t        _window_size;
    window_t        _window_type;
    uint32_t        _data_size;
    double          _freq_per_bin;
    double          _phase_scale;
    double          _phasediff_step;
    float*          _window;
    kiss_fft_cpx_*  _fft_in;
    kiss_fft_cpx_*  _fft_out;
    float*          _power;
    float*          _phase;
    float*          _phase_h;
    kiss_fft_state* _fft;

    double _phasediff_bin;

    float* _genWindow();
    void _analyze();
    void _reset();
    void _run(uint32_t n_samples, float const* data);
};
