// Libre Audio Suite — native variable-shape tilt.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <complex>

namespace eq {
inline float tiltShape(float q) { return std::clamp((q - .3f) / 7.71f, 0.f, 1.f); }
struct TiltCoefficients {
    static constexpr unsigned count = 12;
    struct Section { double b0 = 1, b1 = 0, a1 = 0; };
    std::array<Section, count> sections {};
    double level = 1;
};
inline std::complex<double> tiltTransfer(const TiltCoefficients& c, double hz, double rate)
{
    const auto z = std::polar(1., -6.283185307179586 * std::clamp(hz / rate, 0., .5));
    std::complex<double> h(c.level, 0);
    for (const auto& s : c.sections) h *= (s.b0 + s.b1 * z) / (1. + s.a1 * z);
    return h;
}
inline TiltCoefficients tiltCoefficients(double frequency, double gain, double shape, double rate)
{
    TiltCoefficients c;
    if (gain == 0) return c;
    const double pivot = std::clamp(frequency, 1., rate * .49);
    // Spread small first-order shelves over the audible range for a near-linear
    // dB/log-frequency tilt; collapse their centers for a shelf-shaped tilt.
    const double width = (1. - std::clamp(shape, 0., 1.)) * std::log2(22000. / 10.) * .5;
    const double A = std::pow(10., 2. * gain / (20. * c.count));
    for (unsigned i = 0; i < c.count; ++i) {
        const double center = std::clamp(pivot * std::exp2(width * (2. * (i + .5) / c.count - 1.)), 1., rate * .49);
        const double g = std::tan(3.14159265358979323846 * center / rate) * std::sqrt(A);
        c.sections[i] = {(A + g) / (1. + g), (g - A) / (1. + g), (g - 1.) / (1. + g)};
    }
    c.level = 1. / std::abs(tiltTransfer(c, pivot, rate));
    return c;
}
inline float tiltResponse(float hz, float frequency, float gain, float shape, float rate)
{
    return float(20. * std::log10(std::max(1e-15, std::abs(tiltTransfer(tiltCoefficients(frequency, gain, shape, rate), hz, rate)))));
}
class Tilt {
    TiltCoefficients current, target;
    std::array<double, TiltCoefficients::count> state {};
    double rate = 48000, smooth = .002, mix = 0, step = 1. / 240;
    bool enabled = false;
public:
    void clear() { state.fill(0); mix = 0; current = target; }
    void setRate(int r) { rate = std::max(1, r); smooth = 1. - std::exp(-1. / (.01 * rate)); step = 1. / (.005 * rate); clear(); }
    void configure(bool on, float frequency, float gain, float shape) {
        enabled = on && gain != 0;
        if (enabled) target = tiltCoefficients(frequency, gain, shape, rate);
        if (mix == 0) current = target;
    }
    bool active() const { return enabled || mix > 0; }
    void process(int count, float* buffer) {
        if (!active()) return;
        for (int i = 0; i < count; ++i) {
            mix = enabled ? std::min(1., mix + step) : std::max(0., mix - step);
            double x = buffer[i];
            for (unsigned j = 0; j < state.size(); ++j) {
                auto& c = current.sections[j]; const auto& t = target.sections[j];
                c.b0 += smooth * (t.b0 - c.b0); c.b1 += smooth * (t.b1 - c.b1); c.a1 += smooth * (t.a1 - c.a1);
                const double y = c.b0 * x + state[j];
                state[j] = c.b1 * x - c.a1 * y;
                x = y;
            }
            current.level += smooth * (target.level - current.level);
            buffer[i] += float(mix * (x * current.level - buffer[i]));
            if (mix == 0) { clear(); break; }
        }
    }
};
}
