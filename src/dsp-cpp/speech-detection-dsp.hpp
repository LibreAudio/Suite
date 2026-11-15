// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "LibreAudioProcessor.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

#include "rnnoise.h"

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioSpeechDetection : public LibreAudioProcessor
{
    static constexpr const uint8_t kNumChannels = 2;

    // rnnoise block size
    static constexpr const uint32_t rnnoiseFrameSize = 480;
    static constexpr const uint32_t rnnoiseFrameSizeF = rnnoiseFrameSize * sizeof(float);

    // rnnoise handles
    DenoiseState* rnnoise[kNumChannels] = {};

    // buffers for rnnoise block processing
    uint32_t bufferInPos = 0;
    float* bufferIn[kNumChannels] = {};
    float* bufferOut = new float[rnnoiseFrameSize];

    // current state
    int sampleRate = 0;

    // current speech detection value
    float vad = 0.f;

public:
    LibreAudioSpeechDetection()
    {
        for (uint8_t c = 0; c < kNumChannels; ++c)
        {
            bufferIn[c] = new float[rnnoiseFrameSize];
            rnnoise[c] = rnnoise_create(nullptr);
        }
    }

    ~LibreAudioSpeechDetection() override
    {
        delete[] bufferOut;

        for (uint8_t c = 0; c < kNumChannels; ++c)
        {
            delete[] bufferIn[c];
            rnnoise_destroy(rnnoise[c]);
        }
    }

    float get(int) const noexcept override
    {
        return vad;
    }

    void init(const int rate) override
    {
        sampleRate = rate;
    }

    void rate(const int rate) override
    {
        sampleRate = rate;
    }

    void reset() override
    {
        bufferInPos = 0;
        vad = 0.f;
    }

    void set(int, float) override
    {
    }

    void run(const int frames, float* inputs[kNumChannels]) override
    {
        // optimize for non-denormal usage
        for (uint8_t c = 0; c < kNumChannels; ++c)
        {
            for (int i = 0; i < frames; ++i)
            {
                if (!std::isfinite(inputs[c][i]))
                    __builtin_unreachable();
            }
        }

        // process in rnnoise block-size chunks
        for (int offset = 0; offset != frames;)
        {
            const uint32_t framesCycle = std::min<int>(rnnoiseFrameSize - bufferInPos, frames - offset);
            const uint32_t framesCycleF = framesCycle * sizeof(float);

            // copy input data into buffer
            for (uint8_t c = 0; c < kNumChannels; ++c)
                std::memcpy(bufferIn[c] + bufferInPos, inputs[c] + offset, framesCycleF);

            // run rnnoise once input buffer is full
            if ((bufferInPos += framesCycle) == rnnoiseFrameSize)
            {
                bufferInPos = 0;

                // run rnnoise
                vad = 0.f;
                for (uint8_t c = 0; c < kNumChannels; ++c)
                    vad = std::max(vad, rnnoise_process_frame(rnnoise[c], bufferOut, bufferIn[c]));
            }

            offset += framesCycle;
        }
    }

    [[nodiscard]] float latency() const override
    {
        // TODO
        return 0.f;
    }
};

// --------------------------------------------------------------------------------------------------------------------
