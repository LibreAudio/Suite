// Libre Audio Suite
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <array>
#include <cmath>

namespace LibreAudio::MbCompResponse {
constexpr double pi = 3.14159265358979323846;
inline double frequency(const double t) { return 20.0 * std::pow(1000.0, t); }
inline double position(const double f) { return std::log(f / 20.0) / std::log(1000.0); }

inline std::array<double, 4> effectiveCrossovers(std::array<double, 4> xo, const double sr)
{
    for (size_t i = 0; i < xo.size(); ++i)
        xo[i] = std::max(20.0, std::min(i == 0 ? xo[i] : std::max(xo[i], xo[i-1] * 1.02), sr * 0.45));
    return xo;
}

// Magnitude of the same bilinear-transform shelves as mbComp5.dsp. This is
// the instantaneous wet response, before the DSP's dry/wet blend.
inline double shelf(const double f, const double fc, const double gain, const int slope, const double sr)
{
    const double r = std::tan(pi * std::min(f, sr * 0.499) / sr) / std::tan(pi * fc / sr);
    if (slope == 0)
    {
        const double g = std::pow(10.0, gain / 20.0);
        return 10.0 * std::log10((g*g + r*r) / (1.0 + r*r));
    }
    const auto svf = [r](const double db, const double q) {
        const double a = std::pow(10.0, db / 40.0);
        const double u = r * std::sqrt(a), u2 = u*u;
        const double num = (a*a-u2)*(a*a-u2) + a*a*u2/(q*q);
        const double den = (1.0-u2)*(1.0-u2) + u2/(q*q);
        return 10.0 * std::log10(num / den);
    };
    return slope == 1 ? svf(gain, 0.70710678)
                      : svf(gain * 0.5, 0.54119610) + svf(gain * 0.5, 1.30656296);
}

inline double response(const double f, const std::array<double, 4>& xo,
                       const std::array<double, 5>& gains, const int slope, const double sr)
{
    double db = gains[4];
    for (size_t i = 0; i < xo.size(); ++i)
        db += shelf(f, xo[i], gains[i] - gains[i+1], slope, sr);
    return db;
}
} // namespace LibreAudio::MbCompResponse
