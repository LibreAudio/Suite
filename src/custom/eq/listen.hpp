// Libre Audio Suite — band listen: auditions one band's frequency region.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <cmath>

namespace eq {
// Unity-peak trapezoidal SVF band-pass on mid and side, crossfaded against the
// EQ output. A channel the band does not route to is muted while listening.
class Listen
{
    struct Channel { double ic1 = 0, ic2 = 0; float mix = 0.f; };
    Channel channels[2];
    double g = 0, k = 1;
    double targetG = 0, targetK = 1;
    float targetMix[2] {};
    bool included[2] {};
    float mixStep = 1.f, smooth = 1.f;
    double rate = 48000;

public:
    void setRate(const double sampleRate)
    {
        rate = sampleRate;
        mixStep = float(1. / (0.005 * rate));                 // 5ms crossfade
        smooth = float(1. - std::exp(-32. / (0.010 * rate))); // 10ms per 32-sample chunk
        clear();
    }

    void clear()
    {
        for (auto& c : channels) c = {};
        g = targetG; k = targetK;
    }

    // listening: a band is selected; routeMid/routeSide: the channels it covers.
    // The other arguments are ignored while not listening.
    void configure(const bool listening, const bool routeMid, const bool routeSide,
                   const float frequency, const float q)
    {
        const bool wasActive = active();
        targetMix[0] = targetMix[1] = listening ? 1.f : 0.f;
        if (! listening) return; // fade out with the last band's settings
        const double f = std::clamp(double(frequency), 10., 0.45 * rate);
        targetG = std::tan(3.14159265358979323846 * f / rate);
        targetK = 1. / std::clamp(double(q), 0.3, 8.01);
        included[0] = routeMid;
        included[1] = routeSide;
        if (! wasActive) { g = targetG; k = targetK; }
    }

    bool active() const noexcept
    {
        return channels[0].mix > 0.f || channels[1].mix > 0.f || targetMix[0] > 0.f;
    }

    void process(const int n, float* const signal[2])
    {
        if (! active()) return;
        g += (targetG - g) * smooth;
        k += (targetK - k) * smooth;
        const double a1 = 1. / (1. + g * (g + k)), a2 = g * a1, a3 = g * a2;
        for (unsigned c = 0; c < 2; ++c)
        {
            Channel& ch = channels[c];
            float* const x = signal[c];
            for (int i = 0; i < n; ++i)
            {
                const double v0 = x[i];
                const double v3 = v0 - ch.ic2;
                const double v1 = a1 * ch.ic1 + a2 * v3;
                const double v2 = ch.ic2 + a2 * ch.ic1 + a3 * v3;
                ch.ic1 = 2. * v1 - ch.ic1;
                ch.ic2 = 2. * v2 - ch.ic2;
                const float wet = included[c] ? float(k * v1) : 0.f;
                ch.mix = ch.mix < targetMix[c] ? std::min(targetMix[c], ch.mix + mixStep)
                                               : std::max(targetMix[c], ch.mix - mixStep);
                x[i] += (wet - x[i]) * ch.mix;
            }
        }
        if (! active()) clear();
    }
};
}
