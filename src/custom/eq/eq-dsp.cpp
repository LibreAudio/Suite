// Libre Audio Suite — native dynamically processed EQ bands.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "eq-dsp.hpp"
#include "eq-parameters.hpp"
#include "bell-response.hpp"
#include "x42-bell.hpp"
#include "highpass.hpp"
#include "ladder-highpass.hpp"
#include <array>

namespace eq {
class DynamicEq final : public FaustDSP
{
    struct Band {
        Fil4Paramsect filter[2];
        HighPass highpass[2];
        LadderHighPass ladder[2];
        LadderHighPass ladderStereo[2];
        float frequency = .02f, bandwidth = 1.f;
        float gain[2] = {1.f, 1.f};
        bool dirty = true;
    };
    std::array<float, kFaustParameterCount> values {};
    std::array<Band, kBandCount> bands {};
    int rate = 48000;

public:
    DynamicEq() { instanceInit(rate); }
    FaustDSP* clone() override { return new DynamicEq; }
    int getNumInputs() override { return 2; }
    int getNumOutputs() override { return 2; }
    int getSampleRate() override { return rate; }
    void buildUserInterface(FaustUI*) override {}
    float get(uint32_t index) const override { return index < values.size() ? values[index] : 0.f; }
    void set(uint32_t index, float value) override
    {
        if (index >= values.size() || !std::isfinite(value)) return;
        const auto& p = kFaustParameters[index];
        value = std::clamp(value, p.min, p.max);
        if (p.isInteger) value = std::round(value);
        if (values[index] == value) return;
        values[index] = value;
        bands[index / kBandStride].dirty = true;
    }
    void instanceResetUserInterface() override
    {
        for (unsigned i = 0; i < values.size(); ++i) values[i] = kFaustParameters[i].init;
        for (auto& band : bands) band.dirty = true;
    }
    void instanceClear() override
    {
        for (auto& band : bands)
        {
            for (auto& filter : band.filter) filter.init();
            for (auto& filter : band.highpass) filter.clear();
            for (auto& filter : band.ladder) filter.clear();
            for (auto& filter : band.ladderStereo) filter.clear();
        }
    }
    void instanceConstants(int sampleRate) override
    {
        rate = std::max(1, sampleRate);
        for (auto& band : bands)
        {
            band.dirty = true;
            for (auto& filter : band.highpass) filter.setRate(rate);
            for (auto& filter : band.ladder) filter.setRate(rate);
            for (auto& filter : band.ladderStereo) filter.setRate(rate);
        }
        instanceClear();
    }
    void instanceInit(int sampleRate) override
    {
        instanceConstants(sampleRate);
        instanceResetUserInterface();
    }
    void init(int sampleRate) override { instanceInit(sampleRate); }

    void compute(int count, float** buffers) override
    {
        if (count <= 0) return; // upstream proc divides coefficient deltas by count
        for (unsigned slot = 0; slot < bands.size(); ++slot)
        {
            auto& band = bands[slot];
            if (!band.dirty) continue;
            const float* p = values.data() + slot * kBandStride;
            band.frequency = bellFrequency(p[kFrequency], rate);
            band.bandwidth = bellBandwidth(p[kQ], p[kGain], p[kAdaptiveQ]);
            const bool enabled = p[kPresent] > .5f && p[kEnabled] > .5f && p[kType] == 2.f;
            const float gain = enabled ? std::pow(10.f, p[kGain] / 20.f) : 1.f;
            band.gain[0] = p[kChannel] == 2.f ? 1.f : gain;
            band.gain[1] = p[kChannel] == 1.f ? 1.f : gain;
            const bool hp = p[kPresent] > .5f && p[kEnabled] > .5f && p[kType] == 0.f;
            for (unsigned c = 0; c < 2; ++c)
                band.highpass[c].configure(hp && p[kChannel] != (c == 0 ? 2.f : 1.f),
                                           p[kFrequency], int(p[kSlope]), p[kQ], rate);
            const bool ladder = p[kPresent] > .5f && p[kEnabled] > .5f && p[kType] == 5.f;
            for (unsigned c = 0; c < 2; ++c)
            {
                band.ladder[c].configure(ladder && p[kChannel] == (c == 0 ? 1.f : 2.f),
                                         p[kFrequency], int(p[kSlope]), p[kQ], rate);
                band.ladderStereo[c].configure(ladder && p[kChannel] == 0.f,
                                               p[kFrequency], int(p[kSlope]), p[kQ], rate);
            }
            band.dirty = false;
        }

        // The active list includes filters fading back to unity. Once settled,
        // disabled/removed/zero-gain bands consume no per-sample filter work.
        struct Active { Band* band; unsigned channel; };
        std::array<Active, kBandCount * 2> active;
        unsigned size = 0;
        for (auto& band : bands)
            for (unsigned c = 0; c < 2; ++c)
                if (band.gain[c] != 1.f || band.filter[c].g0() != 0.f || band.highpass[c].active() || band.ladder[c].active() || band.ladderStereo[0].active() || band.ladderStereo[1].active())
                    active[size++] = {&band, c};
        if (size == 0) return; // exact passthrough, without even an M/S round trip

        for (int offset = 0; offset < count; offset += 32)
        {
            const int n = std::min(32, count - offset);
            float signal[2][32];
            for (int i = 0; i < n; ++i)
            {
                const float l = buffers[0][offset + i], r = buffers[1][offset + i];
                signal[0][i] = .5f * (l + r);
                signal[1][i] = .5f * (l - r);
            }
            for (unsigned i = 0; i < size; ++i)
            {
                auto& b = *active[i].band;
                const unsigned c = active[i].channel;
                if (b.gain[c] != 1.f || b.filter[c].g0() != 0.f)
                    b.filter[c].proc(n, signal[c], b.frequency, b.bandwidth, b.gain[c]);
                b.highpass[c].process(n, signal[c]);
                b.ladder[c].process(n, signal[c]);
                // Nonlinear stereo filtering must run on L/R, not M/S: those
                // transforms commute only for linear processors. Separate state
                // banks also let routing changes fade out without reinterpreting history.
                if (c == 1 && (b.ladderStereo[0].active() || b.ladderStereo[1].active()))
                {
                    for (int j = 0; j < n; ++j)
                    {
                        const float m = signal[0][j], side = signal[1][j];
                        signal[0][j] = m + side;
                        signal[1][j] = m - side;
                    }
                    b.ladderStereo[0].process(n, signal[0]);
                    b.ladderStereo[1].process(n, signal[1]);
                    for (int j = 0; j < n; ++j)
                    {
                        const float l = signal[0][j], r = signal[1][j];
                        signal[0][j] = .5f * (l + r);
                        signal[1][j] = .5f * (l - r);
                    }
                }
            }
            for (int i = 0; i < n; ++i)
            {
                buffers[0][offset + i] = signal[0][i] + signal[1][i];
                buffers[1][offset + i] = signal[0][i] - signal[1][i];
            }
        }
        // Discard stale delay history before a retired band can be reused.
        for (auto& band : bands)
            for (unsigned c = 0; c < 2; ++c)
                if (band.gain[c] == 1.f && band.filter[c].g0() == 0.f)
                    band.filter[c].init();
    }
};
FaustDSP* createDSP() { return new DynamicEq; }
}
START_NAMESPACE_DISTRHO
FaustDSP* createDSP() { return eq::createDSP(); }
END_NAMESPACE_DISTRHO
