// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <cmath>
#include <complex>

namespace eq {
inline float bellFrequency(float hz, float rate)
{
    return std::clamp(hz / rate, 0.0002f, 0.4998f);
}

// x42's bandwidth control is approximately octaves. This conversion gives the
// UI's Q its analog half-gain bandwidth (7 is the upstream filter's constant).
inline float bellBandwidth(float q, float gain, float adaptive)
{
    return 6.283185f / (7.f * q * (1.f + adaptive * std::abs(gain) / 18.f * 1.6f));
}

inline float bellResponse(float hz, float frequency, float gain, float q, float adaptive, float rate)
{
    const double f = bellFrequency(frequency, rate);
    const double g = std::pow(10., gain / 20.);
    const double b = bellBandwidth(q, gain, adaptive) * 7 * f / std::sqrt(g);
    const double s2 = (1 - b) / (1 + b);
    const double s1 = -std::cos(6.283185f * f) * (1 + s2);
    const double g0 = .5 * (g - 1) * (1 - s2);
    const auto z = std::polar(1., -6.283185307179586 * std::min<double>(hz / rate, .5));
    return 10 * std::log10(std::norm((1. + g0 + s1 * z + (s2 - g0) * z * z)
                                  / (1. + s1 * z + s2 * z * z)));
}
}
