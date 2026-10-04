// FFT analysis - spectrogram
// Copyright (C) 2013, 2019 Robin Gareus <robin@gareus.org>
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "fft.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

#ifndef M_PIf
#define M_PIf static_cast<float>(M_PI)
#endif

#include "kiss_fft.h"

struct kiss_fft_cpx_ : kiss_fft_cpx {};

/* ****************************************************************************
 * windows
 */

static double ft_hannhamm(float* window, uint32_t n, double a, double b)
{
    double sum = 0.0;
    const double c = 2.0 * M_PI / (n - 1.0);
    for (uint32_t i = 0; i < n; ++i) {
        window[i] = a - b * std::cos (c * i);
        sum += window[i];
    }
    return sum;
}

static double ft_bnh(float* window, uint32_t n, double a0, double a1, double a2, double a3)
{
    double sum = 0.0;
    const double c  = 2.0 * M_PI / (n - 1.0);
    const double c2 = 2.0 * c;
    const double c3 = 3.0 * c;

    for (uint32_t i = 0; i < n; ++i) {
        window[i] = a0 - a1 * std::cos(c * i) + a2 * std::cos(c2 * i) - a3 * std::cos(c3 * i);
        sum += window[i];
    }
    return sum;
}

static double ft_flattop(float* window, uint32_t n)
{
    double sum = 0.0;
    const double c  = 2.0 * M_PI / (n - 1.0);
    const double c2 = 2.0 * c;
    const double c3 = 3.0 * c;
    const double c4 = 4.0 * c;

    static constexpr const double a0 = 1.0;
    static constexpr const double a1 = 1.93;
    static constexpr const double a2 = 1.29;
    static constexpr const double a3 = 0.388;
    static constexpr const double a4 = 0.028;

    for (uint32_t i = 0; i < n; ++i) {
        window[i] = a0 - a1 * std::cos(c * i) + a2 * std::cos(c2 * i) - a3 * std::cos(c3 * i) + a4 * std::cos(c4 * i);
        sum += window[i];
    }
    return sum;
}

/* ***************************************************************************
 * convenient access functions
 */

static inline float fast_log2 (float val)
{
    union {
        float f;
        int   i;
    } t;
    t.f                = val;
    int* const exp_ptr = &t.i;
    int        x       = *exp_ptr;
    const int  log_2   = ((x >> 23) & 255) - 128;
    x &= ~(255 << 23);
    x += 127 << 23;
    *exp_ptr = x;
    val      = ((-1.0f / 3) * t.f + 2) * t.f - 2.0f / 3;
    return (val + log_2);
}

static inline float fast_log10 (const float val)
{
    return fast_log2 (val) / 3.312500f;
}

static inline float fftx_power_to_dB (float a)
{
    /* 10 instead of 20 because of squared signal -- no sqrt(powerp[]) */
    return a > 1e-12 ? 10.0 * fast_log10 (a) : FFTAnalysis::kSmallestValue;
}

/******************************************************************************
 * public API (static for direct source inclusion)
 */

// const kiss_fft_state rnn_kfft = {
//     _window_size, /* nfft */
//     1.f / _window_size, /* scale */
//     -1, /* shift */
//     fft_factors,
//     fft_bitrev,
//     fft_twiddles,
//     (arch_fft_state *)&arch_fft,
// };

void FFTAnalysis::init(uint32_t window_size, double rate)
{
    if (_fft != nullptr)
        return;
    _window_size    = window_size;
    _window_type    = W_HANN;
    _data_size      = window_size / 2;
    _window         = NULL;
    _freq_per_bin   = rate / _data_size / 2.f;
    _phasediff_step = M_PI / _data_size;
    _phasediff_bin  = 0;
    _phase_scale    = (_data_size / (double)window_size) / M_PI;

    _fft_in  = (kiss_fft_cpx_*)std::malloc(window_size * sizeof(kiss_fft_cpx_));
    _fft_out = (kiss_fft_cpx_*)std::malloc(window_size * sizeof(kiss_fft_cpx_));
    _power   = (float*)std::malloc(_data_size * sizeof(float));
    _phase   = (float*)std::malloc(_data_size * sizeof(float));
    _phase_h = (float*)std::malloc(_data_size * sizeof(float));

    _reset();

    _fft = rnn_fft_alloc(window_size, nullptr, nullptr, 0);
}

void FFTAnalysis::free()
{
    if (_fft == nullptr)
        return;
    rnn_fft_free(_fft, 0);
    std::free(_window);
    std::free(_fft_in);
    std::free(_fft_out);
    std::free(_power);
    std::free(_phase);
    std::free(_phase_h);
    // sentinel
    _fft = nullptr;
}

void FFTAnalysis::run(const float* const data)
{
    /* import and apply window function */
    const float* const window = _genWindow();

    for (uint32_t i = 0; i < _window_size; i++)
        _fft_in[i].r = data[i] * window[i];

    /* ..and analyze */
    _analyze();

    _phasediff_bin = _phasediff_step * (double)_window_size;
}

float FFTAnalysis::powerAtBin(const int b) const
{
    return fftx_power_to_dB(_power[b]);
}

float FFTAnalysis::freqAtBin(const int b) const
{
    /* calc phase: difference minus expected difference */
    float phase = _phase[b] - _phase_h[b] - (float)b * _phasediff_bin;
    /* clamp to -M_PI .. M_PI */
    int over = phase / M_PIf;
    over += (over >= 0) ? (over & 1) : -(over & 1);
    phase -= M_PIf * (float)over;
    /* scale according to overlap */
    phase *= _phase_scale;
    return _freq_per_bin * ((float)b + phase);
}

/* ****************************************************************************
 * internal private functions
 */
float* FFTAnalysis::_genWindow()
{
    if (_window) {
        return _window;
    }

    _window = (float*)std::malloc (sizeof (float) * _window_size);
    double sum = .0;

    /* https://en.wikipedia.org/wiki/Window_function */
    switch (_window_type) {
        default:
        case W_HANN:
            sum = ft_hannhamm (_window, _window_size, .5, .5);
            break;
        case W_HAMMMIN:
            sum = ft_hannhamm (_window, _window_size, .54, .46);
            break;
        case W_NUTTALL:
            sum = ft_bnh (_window, _window_size, .355768, .487396, .144232, .012604);
            break;
        case W_BLACKMAN_NUTTALL:
            sum = ft_bnh (_window, _window_size, .3635819, .4891775, .1365995, .0106411);
            break;
        case W_BLACKMAN_HARRIS:
            sum = ft_bnh (_window, _window_size, .35875, .48829, .14128, .01168);
            break;
        case W_FLAT_TOP:
            sum = ft_flattop (_window, _window_size);
            break;
    }

    const double isum = 2.0 / sum;
    for (uint32_t i = 0; i < _window_size; i++) {
        _window[i] *= isum;
    }

    return _window;
}

void FFTAnalysis::_analyze()
{
    rnn_fft(_fft, _fft_in, _fft_out, 0);

    std::memcpy (_phase_h, _phase, sizeof (float) * _data_size);

    for (uint32_t i = 0; i < _data_size; ++i) {
        const float re = _window_size * _fft_out[i].r;
        const float im = _window_size * _fft_out[i].i;
        _power[i] = (re * re) + (im * im);
        _phase[i] = atan2f (im, re); // FIXME
    }
}

void FFTAnalysis::_reset()
{
    for (uint32_t i = 0; i < _data_size; ++i) {
        _power[i]   = 0;
        _phase[i]   = 0;
        _phase_h[i] = 0;
    }
    // for (uint32_t i = 0; i < _window_size; ++i) {
    //     _fft_out[i] = 0;
    // }
}
