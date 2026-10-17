// Libre Audio Suite
// Copyright (C) 2025-2026 Filipe Coelho <falktx@falktx.com>
// Copyright (C) 2026 Klaus Scheuermann
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "extra/RingBuffer.hpp"
#include "extra/ValueSmoother.hpp"

#include "rnnoise-stereo.h"
#include "speex_resampler.h"

#include <algorithm>
#include <cmath>
#include <cstring>

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------
// Stereo RNNoise voice isolation, processing audio in place with a fixed latency.
// RNNoise only works at 48 kHz, other host rates are resampled into and out of it.
// A gate driven by RNNoise's voice activity mutes the denoised signal between speech.

template<uint32_t maxBlockSize>
class LibreAudioVoiceIsolation
{
public:
    static constexpr const uint32_t kNumBands = LAS_RNNOISE_NB_BANDS;

private:
    // rnnoise block size
    static constexpr const uint32_t kFrameSize = 480;
    static constexpr const uint32_t kFrameSizeF = kFrameSize * sizeof(float);

    static constexpr const uint32_t kProcessingRate = 48000;
    static constexpr const float kFrameMs = 1000.f * kFrameSize / kProcessingRate;
    static constexpr const int kResamplerQuality = SPEEX_RESAMPLER_QUALITY_DEFAULT;

    // gate smoothing, in seconds
    static constexpr const float kGateAttack = 0.005f;
    static constexpr const float kGateRelease = 0.1f;

    // voice activity statistics cover the last 2 seconds
    static constexpr const uint32_t kVadHistorySize = 200;

    // a band gain floor this low or lower means no limit
    static constexpr const float kMaxReductionDb = 100.f;

    // rnnoise handles, keep them const so we never modify them
    DenoiseState* const fDenoise[2] = { rnnoise_create(nullptr), rnnoise_create(nullptr) };

    LasRnnoiseStereoSettings fSettings = { 0.6f, 1.f, 0.f, 0 };

    // both null when the host runs at kProcessingRate
    SpeexResamplerState* fResamplerIn = nullptr;   // host rate -> 48 kHz
    SpeexResamplerState* fResamplerOut = nullptr;  // 48 kHz -> host rate

    // in host frames: one rnnoise frame at host rate, rounded up, and the most
    // fResamplerOut can return for one rnnoise frame
    uint32_t fOutputPrefill = kFrameSize;
    uint32_t fHostFrameSize = kFrameSize;

    // in host frames
    uint32_t fLatency = 0;

    // rnnoise frames at 48 kHz, input is collected until a full frame is ready
    uint32_t fFrameInPos = 0;
    float fFrameIn[2][kFrameSize] = {};
    float fFrameOut[2][kFrameSize] = {};

    // host rate, one resampled rnnoise frame
    float* fHostOut[2] = {};
    HeapRingBuffer fRingBufferOut[2];

    // gate on the denoised signal, at 48 kHz
    float fThreshold = 0.5f;
    uint32_t fGracePeriodFrames = kProcessingRate;
    uint32_t fGraceFramesLeft = 0;
    LinearValueSmoother fGate;

    // meters, updated once per rnnoise frame
    float fBandGains[kNumBands] = {};
    float fBandReductionDb[kNumBands] = {};
    float fVadHistory[kVadHistorySize] = {};
    uint32_t fVadHistoryPos = 0;
    uint32_t fVadHistoryCount = 0;
    float fVadCurrent = 0.f;
    float fVadAverage = 0.f;
    float fVadMinimum = 0.f;
    float fVadMaximum = 0.f;

public:
    LibreAudioVoiceIsolation()
    {
        fGate.setSampleRate(kProcessingRate);
        fGate.setTimeConstant(kGateRelease);
        fGate.setTargetValue(0.f);
    }

    ~LibreAudioVoiceIsolation()
    {
        rnnoise_destroy(fDenoise[0]);
        rnnoise_destroy(fDenoise[1]);
        destroyResamplers();
        delete[] fHostOut[0];
        delete[] fHostOut[1];
    }

   /* -----------------------------------------------------------------------------------------------------------------
    * Parameters */

    // voice activity needed to open the gate, in %
    void setThreshold(const float value) noexcept
    {
        fThreshold = value * 0.01f;
    }

    // how long the gate stays open after voice activity drops, in ms
    void setGracePeriod(const float value) noexcept
    {
        fGracePeriodFrames = d_roundToUnsignedInt(value * kProcessingRate / 1000.0);
    }

    // blends every band's gain with unity, like a dry/wet mix of the denoiser alone, in %
    void setDepth(const float value) noexcept
    {
        fSettings.depth = value * 0.01f;
    }

    // lower limit for the band gains, in dB, kMaxReductionDb or more for no limit
    void setMaxReduction(const float value) noexcept
    {
        fSettings.minGain = value >= kMaxReductionDb ? 0.f : std::pow(10.f, value * -0.05f);
    }

    // time for band gains to fall by 60 dB at most, in ms
    void setRelease(const float value) noexcept
    {
        fSettings.decay = std::pow(0.001f, kFrameMs / std::max(value, kFrameMs));
    }

    void setPitchEnhancement(const bool enabled) noexcept
    {
        fSettings.pitchFilter = enabled ? 1 : 0;
    }

   /* -----------------------------------------------------------------------------------------------------------------
    * Meters */

    // in host frames
    [[nodiscard]] uint32_t latency() const noexcept { return fLatency; }

    // voice activity of the latest frame, and over the last 2 seconds, in %
    [[nodiscard]] float vadCurrent() const noexcept { return fVadCurrent * 100.f; }
    [[nodiscard]] float vadAverage() const noexcept { return fVadAverage * 100.f; }
    [[nodiscard]] float vadMinimum() const noexcept { return fVadMinimum * 100.f; }
    [[nodiscard]] float vadMaximum() const noexcept { return fVadMaximum * 100.f; }

    // gain applied to a band by the denoiser, in dB, 0 or below
    [[nodiscard]] float bandReduction(const uint32_t band) const noexcept { return fBandReductionDb[band]; }

   /* -----------------------------------------------------------------------------------------------------------------
    * Processing */

    // allocates, must not be called during processing
    void setSampleRate(const double sampleRate)
    {
        destroyResamplers();

        const uint32_t hostRate = std::max(1u, d_roundToUnsignedInt(sampleRate));
        const double ratio = static_cast<double>(hostRate) / kProcessingRate;

        // two frames inside rnnoise, at the processing rate
        double latency = kFrameSize * 2 * ratio;

        // one rnnoise frame of buffering, in host frames
        fOutputPrefill = static_cast<uint32_t>(std::ceil(kFrameSize * ratio));
        fHostFrameSize = fOutputPrefill;
        latency += fOutputPrefill;

        if (hostRate != kProcessingRate)
        {
            int err = RESAMPLER_ERR_SUCCESS;
            fResamplerIn = speex_resampler_init(2, hostRate, kProcessingRate, kResamplerQuality, &err);
            DISTRHO_SAFE_ASSERT(err == RESAMPLER_ERR_SUCCESS);
            fResamplerOut = speex_resampler_init(2, kProcessingRate, hostRate, kResamplerQuality, &err);
            DISTRHO_SAFE_ASSERT(err == RESAMPLER_ERR_SUCCESS);

            if (fResamplerIn == nullptr || fResamplerOut == nullptr)
            {
                // leave the plugin usable, just without resampling
                destroyResamplers();
            }
            else
            {
                // speex delays by half its filter length, in its own input frames
                latency += speex_resampler_get_input_latency(fResamplerIn);
                latency += speex_resampler_get_input_latency(fResamplerOut) * ratio;

                // fResamplerOut may return one sample more than the exact ratio
                fHostFrameSize += 2;
            }
        }

        fLatency = static_cast<uint32_t>(latency + 0.5);

        // holds the prefill plus at most one block and one resampled frame, with a margin on top
        const uint32_t ringBufferSize = (fOutputPrefill + maxBlockSize + fHostFrameSize * 2) * sizeof(float);

        for (int c = 0; c < 2; ++c)
        {
            // fHostOut and the ring buffer are always allocated together
            if (fHostOut[c] != nullptr)
            {
                fRingBufferOut[c].deleteBuffer();
                delete[] fHostOut[c];
            }

            fRingBufferOut[c].createBuffer(ringBufferSize);
            fHostOut[c] = new float[fHostFrameSize];
        }

        reset();
    }

    // realtime safe
    void reset() noexcept
    {
        for (int c = 0; c < 2; ++c)
        {
            rnnoise_init(fDenoise[c], nullptr);
            std::memset(fFrameOut[c], 0, kFrameSizeF);
        }

        if (fResamplerIn != nullptr)
        {
            speex_resampler_reset_mem(fResamplerIn);
            speex_resampler_reset_mem(fResamplerOut);
        }

        fFrameInPos = 0;

        fGraceFramesLeft = 0;
        fGate.setTimeConstant(kGateRelease);
        fGate.setTargetValue(0.f);
        fGate.clearToTargetValue();

        std::fill(fBandGains, fBandGains + kNumBands, 1.f);
        std::fill(fBandReductionDb, fBandReductionDb + kNumBands, 0.f);
        fVadHistoryPos = fVadHistoryCount = 0;
        fVadCurrent = fVadAverage = fVadMinimum = fVadMaximum = 0.f;

        // Prefill the output so that reading a block straight after feeding it
        // never underruns: at most one rnnoise frame is still incomplete, and
        // speex returns every output sample as soon as its input is available.
        for (int c = 0; c < 2; ++c)
        {
            fRingBufferOut[c].flush();
            writeSilence(fRingBufferOut[c], fOutputPrefill);
        }
    }

    // frames must not exceed maxBlockSize
    void process(float* const buffers[2], const uint32_t frames) noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(frames <= maxBlockSize,);

        feed(buffers, frames);

        // the prefill guarantees a full block is ready
        for (int c = 0; c < 2; ++c)
            fRingBufferOut[c].readCustomData(buffers[c], frames * sizeof(float));
    }

private:
    // uses fFrameOut as its source of zeros, so only valid right after clearing it
    void writeSilence(HeapRingBuffer& ringBuffer, uint32_t frames) noexcept
    {
        while (frames != 0)
        {
            const uint32_t count = std::min(frames, kFrameSize);
            ringBuffer.writeCustomData(fFrameOut[0], count * sizeof(float));
            ringBuffer.commitWrite();
            frames -= count;
        }
    }

    // brings host-rate input into the 48 kHz rnnoise frames, processing each frame as it fills
    void feed(const float* const in[2], const uint32_t frames) noexcept
    {
        for (uint32_t done = 0; done != frames;)
        {
            uint32_t consumed, produced;

            if (fResamplerIn == nullptr)
            {
                consumed = produced = std::min(kFrameSize - fFrameInPos, frames - done);

                for (int c = 0; c < 2; ++c)
                    std::memcpy(fFrameIn[c] + fFrameInPos, in[c] + done, produced * sizeof(float));
            }
            else
            {
                // both channels share every rate parameter, so they advance identically
                spx_uint32_t inLen, outLen;

                for (int c = 0; c < 2; ++c)
                {
                    inLen = frames - done;
                    outLen = kFrameSize - fFrameInPos;
                    speex_resampler_process_float(fResamplerIn, c, in[c] + done, &inLen,
                                                  fFrameIn[c] + fFrameInPos, &outLen);
                }

                consumed = inLen;
                produced = outLen;

                // cannot happen with frame space left and input pending; guards the loop
                DISTRHO_SAFE_ASSERT_BREAK(consumed != 0 || produced != 0);
            }

            done += consumed;

            if ((fFrameInPos += produced) == kFrameSize)
            {
                fFrameInPos = 0;
                processFrame();
            }
        }
    }

    // denoises one full 48 kHz frame, then queues the result at host rate
    void processFrame() noexcept
    {
        const float vad = las_rnnoise_process_stereo(fDenoise[0], fDenoise[1],
                                                     fFrameOut[0], fFrameOut[1],
                                                     fFrameIn[0], fFrameIn[1],
                                                     &fSettings, fBandGains);

        updateMeters(vad);

        // open the gate on voice activity, close it once the grace period has passed
        if (vad >= fThreshold)
        {
            fGate.setTimeConstant(kGateAttack);
            fGate.setTargetValue(1.f);
            fGraceFramesLeft = fGracePeriodFrames;
        }
        else if (fGraceFramesLeft == 0)
        {
            fGate.setTimeConstant(kGateRelease);
            fGate.setTargetValue(0.f);
        }

        for (uint32_t i = 0; i < kFrameSize; ++i)
        {
            if (fGraceFramesLeft != 0 && --fGraceFramesLeft == 0)
            {
                fGate.setTimeConstant(kGateRelease);
                fGate.setTargetValue(0.f);
            }

            const float gate = fGate.next();

            for (int c = 0; c < 2; ++c)
                fFrameOut[c][i] *= gate;
        }

        for (int c = 0; c < 2; ++c)
            writeOutput(c);
    }

    void updateMeters(const float vad) noexcept
    {
        for (uint32_t i = 0; i < kNumBands; ++i)
            fBandReductionDb[i] = std::max(-kMaxReductionDb, 20.f * std::log10(std::max(fBandGains[i], 1e-5f)));

        fVadCurrent = vad;
        fVadHistory[fVadHistoryPos] = vad;

        if (++fVadHistoryPos == kVadHistorySize)
            fVadHistoryPos = 0;
        if (fVadHistoryCount < kVadHistorySize)
            ++fVadHistoryCount;

        float sum = 0.f, min = 1.f, max = 0.f;
        for (uint32_t i = 0; i < fVadHistoryCount; ++i)
        {
            sum += fVadHistory[i];
            min = std::min(min, fVadHistory[i]);
            max = std::max(max, fVadHistory[i]);
        }

        fVadAverage = sum / fVadHistoryCount;
        fVadMinimum = min;
        fVadMaximum = max;
    }

    void writeOutput(const int c) noexcept
    {
        if (fResamplerOut == nullptr)
        {
            fRingBufferOut[c].writeCustomData(fFrameOut[c], kFrameSizeF);
            fRingBufferOut[c].commitWrite();
            return;
        }

        // fHostOut is large enough for a whole frame, so this normally runs once
        for (uint32_t done = 0; done != kFrameSize;)
        {
            spx_uint32_t inLen = kFrameSize - done;
            spx_uint32_t outLen = fHostFrameSize;
            speex_resampler_process_float(fResamplerOut, c, fFrameOut[c] + done, &inLen, fHostOut[c], &outLen);
            DISTRHO_SAFE_ASSERT_BREAK(inLen != 0 || outLen != 0);
            done += inLen;

            if (outLen != 0)
            {
                fRingBufferOut[c].writeCustomData(fHostOut[c], outLen * sizeof(float));
                fRingBufferOut[c].commitWrite();
            }
        }
    }

    void destroyResamplers() noexcept
    {
        if (fResamplerIn != nullptr)
        {
            speex_resampler_destroy(fResamplerIn);
            fResamplerIn = nullptr;
        }
        if (fResamplerOut != nullptr)
        {
            speex_resampler_destroy(fResamplerOut);
            fResamplerOut = nullptr;
        }
    }
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
