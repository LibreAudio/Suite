// Libre Audio Suite — band listen: auditions the frequency region one band works on.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <cmath>

namespace eq {
// Trapezoidal SVF on the dry (pre-EQ) mid and side, crossfaded against the EQ
// output: a unity-peak band-pass, a low-pass or a high-pass, crossfading between
// them when the shape changes. A channel the band does not route to is muted
// while listening.
class Listen
{
public:
    enum Shape { BandPass, LowPass, HighPass, kShapeCount };

private:
    struct Channel { double ic1 = 0, ic2 = 0; float mix = 0.f; };
    Channel channels[2];
    float weight[kShapeCount] { 1.f, 0.f, 0.f };
    float targetWeight[kShapeCount] { 1.f, 0.f, 0.f };
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
        std::copy(std::begin(targetWeight), std::end(targetWeight), weight);
    }

    // listening: a band is selected; routeMid/routeSide: the channels it covers;
    // shape: the filter, at the band's frequency, with q setting its resonance.
    // The other arguments are ignored while not listening.
    void configure(const bool listening, const bool routeMid, const bool routeSide,
                   const Shape shape, const float frequency, const float q)
    {
        const bool wasActive = active();
        targetMix[0] = targetMix[1] = listening ? 1.f : 0.f;
        if (! listening) return; // fade out with the last band's settings
        const double f = std::clamp(double(frequency), 10., 0.45 * rate);
        targetG = std::tan(3.14159265358979323846 * f / rate);
        targetK = 1. / std::clamp(double(q), 0.3, 8.01);
        included[0] = routeMid;
        included[1] = routeSide;
        for (int i = 0; i < kShapeCount; ++i) targetWeight[i] = i == shape ? 1.f : 0.f;
        if (! wasActive)
        {
            g = targetG; k = targetK;
            std::copy(std::begin(targetWeight), std::end(targetWeight), weight);
        }
    }

    bool active() const noexcept
    {
        return channels[0].mix > 0.f || channels[1].mix > 0.f || targetMix[0] > 0.f;
    }

    // dry: the input before the EQ; signal: the EQ output, replaced in place
    void process(const int n, const float* const dry[2], float* const signal[2])
    {
        if (! active()) return;
        g += (targetG - g) * smooth;
        k += (targetK - k) * smooth;
        const double a1 = 1. / (1. + g * (g + k)), a2 = g * a1, a3 = g * a2;
        float w[kShapeCount];
        for (unsigned c = 0; c < 2; ++c)
        {
            Channel& ch = channels[c];
            const float* const d = dry[c];
            float* const x = signal[c];
            for (int i = 0; i < kShapeCount; ++i) w[i] = weight[i];
            for (int i = 0; i < n; ++i)
            {
                const double v0 = d[i];
                const double v3 = v0 - ch.ic2;
                const double v1 = a1 * ch.ic1 + a2 * v3;
                const double v2 = ch.ic2 + a2 * ch.ic1 + a3 * v3;
                ch.ic1 = 2. * v1 - ch.ic1;
                ch.ic2 = 2. * v2 - ch.ic2;
                for (int s = 0; s < kShapeCount; ++s)
                    w[s] = w[s] < targetWeight[s] ? std::min(targetWeight[s], w[s] + mixStep)
                                                  : std::max(targetWeight[s], w[s] - mixStep);
                const double bp = k * v1, lp = v2, hp = v0 - k * v1 - v2;
                const float wet = included[c] ? float(w[BandPass] * bp + w[LowPass] * lp + w[HighPass] * hp) : 0.f;
                ch.mix = ch.mix < targetMix[c] ? std::min(targetMix[c], ch.mix + mixStep)
                                               : std::max(targetMix[c], ch.mix - mixStep);
                x[i] += (wet - x[i]) * ch.mix;
            }
        }
        for (int i = 0; i < kShapeCount; ++i) weight[i] = w[i];
        if (! active()) clear();
    }
};

// the listen filter for a band type (0 HP, 1 low shelf, 2 bell, 3 high shelf, 4 LP, 5 tilt):
// what the band works on, below or above its frequency or around it
inline Listen::Shape listenShape(const int type) noexcept
{
    return type == 0 || type == 1 ? Listen::LowPass
         : type == 3 || type == 4 ? Listen::HighPass
         : Listen::BandPass;
}

// a bell listens with its own Q, the other types at 0.707
inline float listenQ(const int type, const float q) noexcept
{
    return type == 2 ? q : .707f;
}

// dB response at f of the listen filter, as Listen::configure sets it up
inline float listenResponse(const Listen::Shape shape, const float f, const float frequency, const float q,
                            const double rate) noexcept
{
    constexpr double pi = 3.14159265358979323846;
    const double fc = std::clamp(double(frequency), 10., 0.45 * rate);
    const double k = 1. / std::clamp(double(q), 0.3, 8.01);
    const double w = std::tan(pi * std::min(double(f), 0.4999 * rate) / rate) / std::tan(pi * fc / rate);
    const double den = std::sqrt((1. - w * w) * (1. - w * w) + k * k * w * w);
    const double num = shape == Listen::LowPass ? 1. : shape == Listen::HighPass ? w * w : k * w;
    return float(20. * std::log10(std::max(num / den, 1e-15)));
}
}
