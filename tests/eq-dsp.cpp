// Standalone native EQ regression checks. See src/custom/eq/README.md.
#include "custom/eq/eq-dsp.hpp"
#include "custom/eq/eq-parameters.hpp"
#include "custom/eq/bell-response.hpp"
#include "custom/eq/x42-bell.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <memory>
#include <chrono>

static void configure(FaustDSP& dsp, unsigned slot, float hz, float gain, float q, int channel = 0)
{
    const unsigned i = slot * eq::kBandStride;
    dsp.set(i + eq::kFrequency, hz);
    dsp.set(i + eq::kGain, gain);
    dsp.set(i + eq::kQ, q);
    dsp.set(i + eq::kChannel, channel);
    dsp.set(i + eq::kPresent, 1);
}

int main()
{
    std::unique_ptr<FaustDSP> dsp(eq::createDSP());
    float l[257], r[257]; float* buffers[] = {l, r};
    dsp->compute(0, nullptr);
    for (int i = 0; i < 257; ++i) l[i] = .123f, r[i] = -.432f;
    dsp->compute(257, buffers);
    for (int i = 0; i < 257; ++i) assert(l[i] == .123f && r[i] == -.432f);

    // Mono impulse parity with the original x42 section, including startup smoothing.
    dsp->init(48000);
    configure(*dsp, 0, 1800, 12, 1.4);
    Fil4Paramsect reference; reference.init();
    for (int block = 0; block < 200; ++block)
    {
        float expected[32] {};
        for (int i = 0; i < 32; ++i) l[i] = r[i] = expected[i] = block == 0 && i == 0 ? 1.f : 0.f;
        reference.proc(32, expected, eq::bellFrequency(1800, 48000), eq::bellBandwidth(1.4, 12, 0), std::pow(10.f, .6f));
        dsp->compute(32, buffers);
        for (int i = 0; i < 32; ++i) assert(std::abs(l[i] - expected[i]) < 2e-6f);
    }

    // Measure the actual digital response, including near Nyquist, against the UI curve.
    for (int rate : {44100, 48000, 96000})
    for (float center : {80.f, 1000.f, 16000.f, 30000.f})
    for (float gain : {-24.f, 12.f, 24.f})
    {
        dsp->init(rate); configure(*dsp, 0, center, gain, .7f);
        const float probe = std::min(center, rate * .43f);
        double inPower = 0, outPower = 0;
        for (int block = 0; block < 1500; ++block)
        {
            for (int i = 0; i < 32; ++i) l[i] = r[i] = .1f * std::sin(6.283185307179586 * probe * (block * 32 + i) / rate);
            if (block > 1000) for (int i = 0; i < 32; ++i) inPower += l[i] * l[i];
            dsp->compute(32, buffers);
            if (block > 1000) for (int i = 0; i < 32; ++i) outPower += l[i] * l[i];
            for (int i = 0; i < 32; ++i) assert(std::isfinite(l[i]));
        }
        const double measured = 10 * std::log10(outPower / inPower);
        assert(std::abs(measured - eq::bellResponse(probe, center, gain, .7f, 0, rate)) < .12);
    }
    // Mid must leave pure side intact; side must leave mono intact.
    for (int channel : {1, 2})
    {
        dsp->init(48000); configure(*dsp, 0, 1000, 24, 8, channel);
        for (int block = 0; block < 30; ++block)
        {
            for (int i = 0; i < 257; ++i) l[i] = .2f, r[i] = channel == 1 ? -.2f : .2f;
            dsp->compute(257, buffers);
            for (int i = 0; i < 257; ++i) assert(std::abs(l[i] - .2f) < 1e-5f);
        }
    }
    // Removal fades to exact passthrough, then a slot can be reused.
    for (unsigned field : {eq::kPresent, eq::kEnabled})
    {
        dsp->init(48000); configure(*dsp, 0, 40, -24, 8);
        for (int block = 0; block < 50; ++block) dsp->compute(257, buffers);
        dsp->set(field, 0);
        for (int block = 0; block < 50; ++block) dsp->compute(257, buffers);
        for (int i = 0; i < 257; ++i) l[i] = .123f, r[i] = -.432f;
        dsp->compute(257, buffers);
        for (int i = 0; i < 257; ++i) assert(l[i] == .123f && r[i] == -.432f);
        dsp->set(field, 1); dsp->compute(257, buffers);
        for (int i = 0; i < 257; ++i) assert(std::isfinite(l[i]));
    }
    // Exercise abrupt host automation, slot reuse, sample-rate changes and
    // non-power-of-two block sizes across all slots.
    dsp->init(48000);
    unsigned random = 42;
    for (int block = 0; block < 4000; ++block)
    {
        random = random * 1664525u + 1013904223u;
        const unsigned slot = random % eq::kBandCount;
        configure(*dsp, slot, block % 2 ? 10.f : 30000.f,
                  block % 3 ? 24.f : -24.f, block % 5 ? .3f : 8.01f, block % 3);
        dsp->set(slot * eq::kBandStride + eq::kAdaptiveQ, block % 2);
        dsp->set(slot * eq::kBandStride + eq::kEnabled, block % 4 != 0);
        if (block % 137 == 0) dsp->instanceConstants(block % 2 ? 44100 : 96000);
        const int count = 1 + random % 257;
        for (int i = 0; i < count; ++i) l[i] = .0001f * std::sin(float(i)), r[i] = -l[i];
        dsp->compute(count, buffers);
        for (int i = 0; i < count; ++i) assert(std::isfinite(l[i]) && std::isfinite(r[i]));
    }
    // Unimplemented filter types cannot accidentally run a bell.
    dsp->init(48000); configure(*dsp, 0, 1000, 24, 1);
    dsp->set(eq::kType, 0);
    for (int i = 0; i < 257; ++i) l[i] = .123f, r[i] = -.432f;
    dsp->compute(257, buffers);
    for (int i = 0; i < 257; ++i) assert(l[i] == .123f && r[i] == -.432f);

    for (unsigned n : {0u, 1u, 4u, 16u})
    {
        dsp->init(48000);
        for (unsigned b = 0; b < n; ++b) configure(*dsp, b, 1000, 3, 1);
        const auto start = std::chrono::steady_clock::now();
        for (int block = 0; block < 15000; ++block)
        {
            for (int i = 0; i < 257; ++i) l[i] = r[i] = .001f;
            dsp->compute(257, buffers);
        }
        std::printf("%u bands: %.1f ms\n", n, std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count());
    }
    std::puts("EQ DSP checks passed");
}
