// Libre Audio Suite — native dynamically processed EQ bands.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "eq-dsp.hpp"
#include "eq-parameters.hpp"
#include "bell-response.hpp"
#include "x42-bell.hpp"
#include "highpass.hpp"
#include "shelf.hpp"
#include "tilt.hpp"
#include "listen.hpp"
#include <array>

namespace eq {
class DynamicEq final : public FaustDSP
{
    struct Band {
        Fil4Paramsect filter[2];
        HighPass highpass[2];
        LowPass lowpass[2];
        Tilt tilt[2];
        Shelf lowShelf[2], highShelf[2];
        float frequency = .02f, bandwidth = 1.f;
        float gain[2] = {1.f, 1.f};
        bool dirty = true;
    };
    std::array<float, kFaustParameterCount> values {};
    std::array<Band, kBandCount> bands {};
    Listen listen;
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
            for (auto& filter : band.lowpass) filter.clear();
            for (auto& filter : band.tilt) filter.clear();
            for (auto& filter : band.lowShelf) filter.clear();
            for (auto& filter : band.highShelf) filter.clear();
        }
        listen.clear();
    }
    void instanceConstants(int sampleRate) override
    {
        rate = std::max(1, sampleRate);
        for (auto& band : bands)
        {
            band.dirty = true;
            for (auto& filter : band.highpass) filter.setRate(rate);
            for (auto& filter : band.lowpass) filter.setRate(rate);
            for (auto& filter : band.tilt) filter.setRate(rate);
            for (auto& filter : band.lowShelf) filter.setRate(rate, false);
            for (auto& filter : band.highShelf) filter.setRate(rate, true);
        }
        listen.setRate(rate);
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
            for (unsigned c = 0; c < 2; ++c)
            {
                const bool enabled = p[kPresent] > .5f && p[kEnabled] > .5f
                                  && p[kChannel] != (c == 0 ? 2.f : 1.f);
                band.tilt[c].configure(enabled && p[kType] == 5.f, p[kFrequency], p[kGain], tiltShape(p[kQ]));
                band.lowpass[c].configure(enabled && p[kType] == 4.f, p[kFrequency], int(p[kSlope]), p[kQ], rate);
                band.lowShelf[c].configure(enabled && p[kType] == 1.f, p[kFrequency], p[kGain], p[kQ]);
                band.highShelf[c].configure(enabled && p[kType] == 3.f, p[kFrequency], p[kGain], p[kQ]);
            }
            band.dirty = false;
        }

        // The lowest present slot with Listen on is auditioned on the input before
        // the EQ, through the region the band works on: a bell through a band-pass
        // of its Q, a low shelf or high-pass through a low-pass, a high shelf or
        // low-pass through a high-pass, tilt through a 0.707 band-pass.
        {
            const float* l = nullptr;
            for (unsigned slot = 0; slot < bands.size() && l == nullptr; ++slot)
            {
                const float* p = values.data() + slot * kBandStride;
                if (p[kPresent] > .5f && p[kListen] > .5f) l = p;
            }
            if (l != nullptr)
            {
                const int type = int(l[kType]);
                listen.configure(true, l[kChannel] != 2.f, l[kChannel] != 1.f, listenShape(type), l[kFrequency],
                                 listenQ(type, l[kQ]));
            }
            else
                listen.configure(false, false, false, Listen::BandPass, 0.f, 0.f);
        }

        // The active list includes filters fading back to unity. Once settled,
        // disabled/removed/zero-gain bands consume no per-sample filter work.
        struct Active { Band* band; unsigned channel; };
        std::array<Active, kBandCount * 2> active;
        unsigned size = 0;
        for (auto& band : bands)
            for (unsigned c = 0; c < 2; ++c)
                if (band.gain[c] != 1.f || band.filter[c].g0() != 0.f || band.highpass[c].active() || band.lowpass[c].active() || band.tilt[c].active()
                    || band.lowShelf[c].active() || band.highShelf[c].active())
                    active[size++] = {&band, c};
        if (size == 0 && ! listen.active()) return; // exact passthrough, without even an M/S round trip

        for (int offset = 0; offset < count; offset += 32)
        {
            const int n = std::min(32, count - offset);
            float signal[2][32], dry[2][32];
            for (int i = 0; i < n; ++i)
            {
                const float l = buffers[0][offset + i], r = buffers[1][offset + i];
                signal[0][i] = dry[0][i] = .5f * (l + r);
                signal[1][i] = dry[1][i] = .5f * (l - r);
            }
            for (unsigned i = 0; i < size; ++i)
            {
                auto& b = *active[i].band;
                const unsigned c = active[i].channel;
                if (b.gain[c] != 1.f || b.filter[c].g0() != 0.f)
                    b.filter[c].proc(n, signal[c], b.frequency, b.bandwidth, b.gain[c]);
                b.lowShelf[c].process(n, signal[c]);
                b.highShelf[c].process(n, signal[c]);
                b.highpass[c].process(n, signal[c]);
                b.lowpass[c].process(n, signal[c]);
                b.tilt[c].process(n, signal[c]);
            }
            const float* const drySignal[2] = {dry[0], dry[1]};
            float* const ms[2] = {signal[0], signal[1]};
            listen.process(n, drySignal, ms);
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
