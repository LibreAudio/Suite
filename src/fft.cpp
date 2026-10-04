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
    fftwf_execute (_fftplan);

    std::memcpy (_phase_h, _phase, sizeof (float) * _data_size);
    _power[0] = _fft_out[0] * _fft_out[0];
    _phase[0] = 0;

#define FRe (_fft_out[i])
#define FIm (_fft_out[_window_size - i])
    for (uint32_t i = 1; i < _data_size - 1; ++i) {
        _power[i] = (FRe * FRe) + (FIm * FIm);
        _phase[i] = atan2f (FIm, FRe); // FIXME
    }
#undef FRe
#undef FIm
}

void FFTAnalysis::_reset()
{
    for (uint32_t i = 0; i < _data_size; ++i) {
        _power[i]   = 0;
        _phase[i]   = 0;
        _phase_h[i] = 0;
    }
    for (uint32_t i = 0; i < _window_size; ++i) {
        _ringbuf[i] = 0;
        _fft_out[i] = 0;
    }
    _rboff = 0;
    _smps  = 0;
    _step  = 0;
}

bool FFTAnalysis::_run (const uint32_t n_samples, float const* const data)
{
    assert (n_samples <= _window_size);

    float* const f_buf = _fft_in;
    float* const r_buf = _ringbuf;

    const uint32_t n_off = _rboff;
    const uint32_t n_siz = _window_size;
    const uint32_t n_old = n_siz - n_samples;

    for (uint32_t i = 0; i < n_samples; ++i) {
        r_buf[(i + n_off) % n_siz] = data[i];
        f_buf[n_old + i]           = data[i];
    }

    _rboff = (_rboff + n_samples) % n_siz;
#if 1
    _smps += n_samples;
    if (_smps < _sps) {
        return false;
    }
    _step = _smps;
    _smps = 0;
#else
    _step = n_samples;
#endif

    /* copy samples from ringbuffer into fft-buffer */
    const uint32_t p0s = (n_off + n_samples) % n_siz;
    if (p0s + n_old >= n_siz) {
        const uint32_t n_p1 = n_siz - p0s;
        const uint32_t n_p2 = n_old - n_p1;
        std::memcpy (f_buf, &r_buf[p0s], sizeof (float) * n_p1);
        std::memcpy (&f_buf[n_p1], &r_buf[0], sizeof (float) * n_p2);
    } else {
        std::memcpy (&f_buf[0], &r_buf[p0s], sizeof (float) * n_old);
    }

    /* apply window function */
    float const* const window = _genWindow();
    for (uint32_t i = 0; i < _window_size; i++) {
        _fft_in[i] *= window[i];
    }

    /* ..and analyze */
    _analyze();

    _phasediff_bin = _phasediff_step * (double)_step;
    return true;
}

/******************************************************************************
 * public API (static for direct source inclusion)
 */

void FFTAnalysis::init(uint32_t window_size, double rate, double fps)
{
    if (_window_size != 0)
        return;
    _rate           = rate;
    _window_size    = window_size;
    _window_type    = W_HANN;
    _data_size      = window_size / 2;
    _window         = NULL;
    _rboff          = 0;
    _smps           = 0;
    _step           = 0;
    _sps            = (fps > 0) ? std::ceil (rate / fps) : 0;
    _freq_per_bin   = _rate / _data_size / 2.f;
    _phasediff_step = M_PI / _data_size;
    _phasediff_bin  = 0;

    _ringbuf = (float*)std::malloc (window_size * sizeof (float));
    _fft_in  = (float*)fftwf_malloc (sizeof (float) * window_size);
    _fft_out = (float*)fftwf_malloc (sizeof (float) * window_size);
    _power   = (float*)std::malloc (_data_size * sizeof (float));
    _phase   = (float*)std::malloc (_data_size * sizeof (float));
    _phase_h = (float*)std::malloc (_data_size * sizeof (float));

    _reset();

    _fftplan = fftwf_plan_r2r_1d (window_size, _fft_in, _fft_out, FFTW_R2HC, FFTW_MEASURE);
}

void FFTAnalysis::free()
{
    if (_window_size == 0)
        return;
    fftwf_destroy_plan (_fftplan);
    std::free (_window);
    std::free (_ringbuf);
    fftwf_free (_fft_in);
    fftwf_free (_fft_out);
    std::free (_power);
    std::free (_phase);
    std::free (_phase_h);
    // sentinel
    _window_size = 0;
}

bool FFTAnalysis::run(const uint32_t n_samples, float const* const data)
{
    if (n_samples <= _window_size) {
        return _run (n_samples, data);
    }

    bool     rv = false;
    uint32_t n  = 0;
    while (n < n_samples) {
        uint32_t step = std::min (_window_size, n_samples - n);
        if (_run (step, &data[n])) {
            rv = true;
        }
        n += step;
    }
    return rv;
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

float FFTAnalysis::powerAtBin(const int b) const
{
    return fftx_power_to_dB(_power[b]);
}

float FFTAnalysis::freqAtBin(const int b) const
{
    /* calc phase: difference minus expected difference */
    float phase = _phase[b] - _phase_h[b] - (float)b * _phasediff_bin;
    /* clamp to -M_PI .. M_PI */
    int over = phase / M_PI;
    over += (over >= 0) ? (over & 1) : -(over & 1);
    phase -= M_PI * (float)over;
    /* scale according to overlap */
    phase *= (_data_size / _step) / M_PI;
    return _freq_per_bin * ((float)b + phase);
}
