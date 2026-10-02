// Libre Audio Suite
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "highpass.hpp"
#include <array>

namespace eq {
// Cascaded trapezoidal high-pass stages with global negative feedback.
// A high-pass ladder transformation of the classic ladder structure; see
// Vadim Zavalishin, The Art of VA Filter Design, high-pass ladder filters.
// This is a Moog-inspired design, not a component model of a specific circuit.
inline double ladderFeedback(int order, double q)
{
    if (order == 1) return 0; // never resonance on the 6dB slope
    const double resonance = std::clamp((q - kHighPassDefaultQ) / (8.01 - kHighPassDefaultQ), 0., 1.);
    // Three/four stages have oscillation thresholds 8/4. Stay below them.
    return resonance * (order == 3 ? 7.6 : 3.8);
}
inline float ladderHighPassResponse(float hz, float frequency, int order, float q, float rate)
{
    if (hz >= rate * .5f) return 0;
    const std::complex<double> s(0, std::tan(kPi * std::max(0.f, hz) / rate) / highPassG(frequency, rate));
    const auto stage = s / (s + 1.);
    std::complex<double> cascade(1, 0);
    for (int i = 0; i < order; ++i) cascade *= stage;
    const double k = ladderFeedback(order, q);
    const auto h = (1 + k) * cascade / (1. + k * cascade);
    return float(10 * std::log10(std::max(1e-30, std::norm(h))));
}
class LadderHighPass
{
    std::array<double, 4> state {};
    double g = .1, targetG = .1, feedback = 0, targetFeedback = 0;
    double mix = 0, mixStep = 1. / 240, smooth = .002;
    int order = 4, targetOrder = 4;
    bool enabled = false;
public:
    void setRate(int rate)
    {
        mixStep = 1. / std::max(1., .005 * rate);
        smooth = 1 - std::exp(-1. / std::max(1., .01 * rate));
        clear();
    }
    void clear() { state.fill(0); mix = 0; order = targetOrder; g = targetG; feedback = targetFeedback; }
    void configure(bool on, float frequency, int slope, float q, int rate)
    {
        enabled = on;
        targetOrder = std::clamp(slope, 1, 4);
        targetG = highPassG(frequency, rate);
        targetFeedback = ladderFeedback(targetOrder, q);
        if (mix == 0) clear();
    }
    bool active() const { return enabled || mix > 0; }
    void process(int count, float* signal)
    {
        if (!active()) return;
        for (int i = 0; i < count; ++i)
        {
            if (mix == 0)
            {
                clear();
                if (!enabled) break;
            }
            const bool wet = enabled && order == targetOrder;
            mix = wet ? std::min(1., mix + mixStep) : std::max(0., mix - mixStep);
            g += smooth * (targetG - g);
            if (order == targetOrder) feedback += smooth * (targetFeedback - feedback);
            const double h = 1 / (1 + g);
            double p = 1, memory = 0;
            for (int j = 0; j < order; ++j)
            {
                p *= h;
                memory = h * (memory - state[j]);
            }
            // Soft saturation at the input and in the global feedback path.
            // Solve y = p*(tanh(input) - k*tanh(y)) + memory with bounded,
            // safeguarded Newton iterations. The derivative is always >= 1.
            const double input = std::tanh(double(signal[i]));
            double y = (p * input + memory) / (1 + feedback * p);
            if (feedback > 0)
            {
                double lo = memory + p * (input - feedback);
                double hi = memory + p * (input + feedback);
                for (int iteration = 0; iteration < 16; ++iteration)
                {
                    const double t = std::tanh(y);
                    const double error = y + p * feedback * t - p * input - memory;
                    if (std::abs(error) < 1e-12) break;
                    if (error > 0) hi = y; else lo = y;
                    const double candidate = y - error / (1 + p * feedback * (1 - t * t));
                    y = candidate > lo && candidate < hi ? candidate : (lo + hi) * .5;
                }
            }
            double x = input - feedback * std::tanh(y);
            for (int j = 0; j < order; ++j)
            {
                const double hp = h * (x - state[j]);
                state[j] += 2 * g * hp;
                x = hp;
            }
            const double output = (1 + feedback) * x; // small-signal passband unity
            signal[i] += float(mix * (output - signal[i]));
        }
        if (mix == 0) state.fill(0);
    }
};
}
