// Native adapter for x42/fil4's resonant 12 dB/oct high-pass.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <cstring>
#include <math.h>

namespace eq::fil4 {
#define RESHP(X) (0.7 + 0.78 * tanh(1.82 * ((X) - .8)))
#include "x42-highpass.hpp"
#undef RESHP
}

namespace eq {
class X42HighPass {
    fil4::HighPass state {};
    float rate = 48000.f, frequency = 80.f, quality = .707f;
    bool enabled = false;
public:
    X42HighPass() { clear(); }
    void clear() {
        // Force the first interpolation to initialize omega, and start at unity
        // so inserting a band does not fade the whole signal up from silence.
        fil4::hip_setup(&state, rate, 0.f, .707f);
        state.g = 1.f;
    }
    void setRate(float sampleRate) { rate = std::max(1.f, sampleRate); clear(); }
    void configure(bool on, float freq, float q) {
        enabled = on;
        frequency = freq;
        quality = std::clamp(q, .0625f, 4.f);
    }
    bool active() const { return enabled || state.a != 1.f || state.q != 0.f || state.g != 1.f; }
    void process(int count, float* buffer) {
        if (!active()) return;
        for (int offset = 0; offset < count; offset += 32) {
            fil4::hip_interpolate(&state, enabled, frequency, quality);
            fil4::hip_compute(&state, std::min(32, count - offset), buffer + offset);
            if (!active()) { clear(); return; }
        }
    }
};

inline float x42HighPassResponse(float f, float frequency, float quality, float rate) {
    const float omega = std::max(5.f, std::min(frequency, rate / 12.f)) / rate;
    const float a = std::exp(-2.0 * M_PI * omega);
    const float q = std::clamp(float(.7 + .78 * std::tanh(1.82 * (std::clamp(quality, .0625f, 4.f) - .8))), 0.f, 1.6f);
    const float g = 1.f + omega + 2.f * q * omega;
    const auto z = std::polar(1.0, -2.0 * M_PI * std::clamp(f / rate, 0.f, .5f));
    const auto h = double(a) * (1.0 - z) / (1.0 - double(a) * z);
    const auto response = double(g / a) * h * h / (1.0 + double(g * q) * z * (h * h - h));
    return float(20.0 * std::log10(std::max(1e-15, std::abs(response))));
}
}
