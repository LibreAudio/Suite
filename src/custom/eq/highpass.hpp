// Libre Audio Suite
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <cmath>
#include <complex>

namespace eq {
inline constexpr double kHighPassDefaultQ = 0.707; // existing UI neutral value
inline constexpr double kPi = 3.14159265358979323846;

inline double highPassG(double frequency, double rate)
{
    return std::tan(kPi * std::clamp(frequency / rate, 0.000001, 0.4998));
}

// Butterworth pole pairs: N=2: Q=sqrt(1/2), N=3: Q=1,
// N=4: Q=0.5411961001 and 1.306562965. Resonance scales only the
// highest-Q pair; at the neutral UI value the complete cascade is Butterworth.
inline double highPassDamping(int order, double q)
{
    const double sectionQ = order == 4 ? 1.3065629648763766
                         : order == 3 ? 1.0 : std::sqrt(0.5);
    return kHighPassDefaultQ / (sectionQ * q);
}

inline float highPassResponse(float hz, float frequency, int order, float q, float rate)
{
    if (hz >= rate * .5f) return 0.f;
    const std::complex<double> s(0, std::tan(kPi * std::max(0.f, hz) / rate) / highPassG(frequency, rate));
    std::complex<double> h(1, 0);
    if (order & 1) h *= s / (s + 1.);
    if (order == 4) h *= s * s / (s * s + 1.8477590650225735 * s + 1.);
    if (order >= 2) h *= s * s / (s * s + highPassDamping(order, q) * s + 1.);
    return float(10 * std::log10(std::max(1e-30, std::norm(h))));
}

inline float lowPassResponse(float hz, float frequency, int order, float q, float rate)
{
    if (hz >= rate * .5f) return -300.f;
    const std::complex<double> s(0, std::tan(kPi * std::max(0.f, hz) / rate) / highPassG(frequency, rate));
    std::complex<double> h(1, 0);
    if (order & 1) h /= s + 1.;
    if (order == 4) h /= s * s + 1.8477590650225735 * s + 1.;
    if (order >= 2) h /= s * s + highPassDamping(order, q) * s + 1.;
    return float(10 * std::log10(std::max(1e-30, std::norm(h))));
}

// Trapezoidal one-pole and state-variable sections. The SVF equations follow
// the standard trapezoidal integrator formulation described by Andrew Simper:
// https://www.cytomic.com/files/dsp/SvfLinearTrapOptimised.pdf
template<bool lowPass>
class ButterworthCut
{
    struct Pair {
        double z1 = 0, z2 = 0;
        double process(double x, double g, double damping)
        {
            const double hp = (x - (damping + g) * z1 - z2) / (1 + g * (damping + g));
            const double bp = g * hp + z1;
            const double lp = g * bp + z2;
            z1 = g * hp + bp;
            z2 = g * bp + lp;
            return lowPass ? lp : hp;
        }
    };
    Pair pairs[2];
    double onePole = 0;
    double g = .1, damping = 1.4142135623730951;
    double targetG = .1, targetDamping = 1.4142135623730951;
    double mix = 0, mixStep = 1. / 240, smooth = .002;
    int order = 2, targetOrder = 2;
    bool enabled = false;

    void clearHistory() { onePole = 0; pairs[0] = {}; pairs[1] = {}; }
public:
    void setRate(int rate)
    {
        mixStep = 1. / std::max(1., .005 * rate);
        smooth = 1 - std::exp(-1. / std::max(1., .01 * rate));
        clear();
    }
    void clear() { clearHistory(); mix = 0; order = targetOrder; g = targetG; damping = targetDamping; }
    void configure(bool on, float frequency, int slope, float q, int rate)
    {
        enabled = on;
        targetOrder = std::clamp(slope, 1, 4);
        targetG = highPassG(frequency, rate);
        targetDamping = highPassDamping(targetOrder, q);
        if (mix == 0)
        {
            order = targetOrder;
            g = targetG;
            damping = targetDamping;
        }
    }
    bool active() const { return enabled || mix > 0; }
    void process(int count, float* signal)
    {
        if (!active()) return;
        for (int i = 0; i < count; ++i)
        {
            // Change topology only at dry, then fade the new topology in.
            // This also handles bypass, removal, type changes and M/S routing.
            if (mix == 0)
            {
                clearHistory();
                order = targetOrder;
                g = targetG;
                damping = targetDamping;
                if (!enabled) break;
            }
            const bool wet = enabled && order == targetOrder;
            mix = wet ? std::min(1., mix + mixStep) : std::max(0., mix - mixStep);
            g += smooth * (targetG - g);
            if (order == targetOrder) damping += smooth * (targetDamping - damping);
            double x = signal[i];
            if (order & 1)
            {
                const double hp = (x - onePole) / (1 + g);
                const double lp = g * hp + onePole;
                onePole = g * hp + lp;
                x = lowPass ? lp : hp;
            }
            if (order == 4) x = pairs[0].process(x, g, 1.8477590650225735);
            if (order >= 2) x = pairs[1].process(x, g, damping);
            signal[i] += float(mix * (x - signal[i]));
        }
        if (mix == 0) clearHistory();
    }
};
using HighPass = ButterworthCut<false>;
using LowPass = ButterworthCut<true>;
}
