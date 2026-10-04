// Libre Audio Suite
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "x42-shelf.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <complex>

namespace eq {
inline void shelfCoefficients(IIRProc& filter, bool high)
{
    if (high) iir_calc_highshelf(&filter); else iir_calc_lowshelf(&filter);
}
inline float shelfResponse(float hz, float frequency, float gainDb, float q, float rate, bool high)
{
    if (gainDb == 0) return 0;
    // Evaluate x42's shelf equations before rounding the coefficients to float.
    // At low cutoff, the near-cancelling biquad sums amplify that rounding into
    // visible jumps as the node moves. Keep the response calculation in double.
    const double frequencyClamped = std::clamp(double(frequency), .0004 * rate, .47 * rate);
    const double w0 = 6.283185307179586 * frequencyClamped / rate;
    const double cosine = std::cos(w0);
    const double A = std::pow(10., double(gainDb) / 40.);
    const double alpha = std::sin(w0) / (2. * std::clamp(double(q), .25, 2.));
    const double beta = 2. * std::sqrt(A) * alpha;
    const double sign = high ? 1. : -1.;
    const double b0 = A * ((A + 1.) + sign * (A - 1.) * cosine + beta);
    const double b1 = -2. * A * (sign * (A - 1.) + (A + 1.) * cosine);
    const double b2 = A * ((A + 1.) + sign * (A - 1.) * cosine - beta);
    const double a0 = (A + 1.) - sign * (A - 1.) * cosine + beta;
    const double a1 = 2. * (sign * (A - 1.) - (A + 1.) * cosine);
    const double a2 = (A + 1.) - sign * (A - 1.) * cosine - beta;
    const auto z = std::polar(1., -6.283185307179586 * std::clamp(double(hz) / rate, 0., .5));
    const auto h = (b0 + b1 * z + b2 * z * z) / (a0 + a1 * z + a2 * z * z);
    return float(10 * std::log10(std::max(1e-30, std::norm(h))));
}
class Shelf
{
    IIRProc filter {};
    std::array<float, 32> smoothing {};
    float gain = 1, frequency = 1000, q = .7f;
    bool high = false;
public:
    void setRate(int rate, bool highShelf)
    {
        high = highShelf;
        iir_init(&filter, rate);
        shelfCoefficients(filter, high);
        for (unsigned i = 0; i < smoothing.size(); ++i)
            smoothing[i] = float(1. - std::pow(1. - 440. / rate, (i + 1) / 32.));
    }
    void clear()
    {
        filter.y0 = filter.y1 = filter.y2 = 0;
        filter.gain = 1;
        shelfCoefficients(filter, high);
    }
    void configure(bool enabled, float hz, float gainDb, float quality)
    {
        frequency = hz;
        q = quality;
        gain = enabled ? std::pow(10.f, gainDb / 20.f) : 1.f;
    }
    bool active() const { return gain != 1.f || filter.gain != 1.f; }
    void process(int count, float* signal)
    {
        if (!active()) return;
        for (int offset = 0; offset < count; offset += 32)
        {
            const int n = std::min(32, count - offset);
            // x42 updates once per approximately 32 samples. Preserve its
            // interpolation at that size, adjusting the time constant for tails.
            filter.lpf = smoothing[n - 1];
            if (iir_interpolate(&filter, gain, frequency, q))
                shelfCoefficients(filter, high);
            if (!active())
            {
                clear(); // exact passthrough and no stale state when a slot is reused
                break;
            }
            iir_compute(&filter, uint32_t(n), signal + offset);
        }
    }
};
}
