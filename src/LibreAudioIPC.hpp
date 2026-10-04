// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dpf/Fifo.hpp"
#include "dpf/SharedMemory.hpp"

#include "fft.hpp"

#include <array>
#include <cmath>

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

// Overall target frame-rate for graphs (FFT analyzer and Waveform)
static constexpr const uint32_t kTargetFrameRate = 60;

// Idle time as double of target rate, so that we don't miss a frame in the worst case scenario
// Repaints must only be requested after pending drawing completes, which ensures we don't bottleneck the system
static constexpr const uint32_t kTargetIdleTimeMs = d_roundToUnsignedInt(1000.0 / (kTargetFrameRate * 2));

// How many seconds the waveform area should hold
static constexpr const uint32_t kNumSecondsForWaveform = 8;

// How many samples to use
static constexpr const uint32_t kNumSamplePointsForWaveform = 8192;

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioAnalyzerIPC {
public:
    static constexpr const uint32_t kNumBins = 256;
    static constexpr const uint32_t kWindowSize = kNumBins * 16;

public:
    LibreAudioAnalyzerIPC() = default;
    ~LibreAudioAnalyzerIPC() = default;

    const char* create()
    {
        DISTRHO_SAFE_ASSERT(! fSharedMem.isCreatedOrConnected());

        if (! fSharedMem.create())
            return nullptr;

        SharedData* const data = fSharedMem.getDataPointer();
        std::memset(data, 0, sizeof(SharedData));

        return fSharedMem.getDataFilename();
    }

    bool connect(const char* const filename)
    {
        if (fSharedMem.isCreatedOrConnected())
            fSharedMem.close();

        return fSharedMem.connect(filename) != nullptr;
    }

    void close()
    {
        fSharedMem.close();
    }

    bool isCreatedOrConnected() const noexcept
    {
        return fSharedMem.isCreatedOrConnected();
    }

    // adapted from https://github.com/x42/modspectre.lv2/blob/master/src/modspectre.c
    // Copyright (C) 2017 Robin Gareus <robin@gareus.org>
    // SPDX-License-Identifier: GPL-2.0-or-later
    bool push(const FFTAnalysis& analysis)
    {
        static constexpr const uint32_t kDataSize = kWindowSize / 2;
        static constexpr const float kResponseTimeSecs = 1.f;

        static constexpr const float log1k = 6.907755279f;  // logf (1000);

       #if defined(__GNUC__) && !defined(__MINGW32__) && !defined(__clang__)
        static constexpr const float tc = std::expf (-2.0 * M_PI * kResponseTimeSecs / 30.f);
       #else
        const float tc = std::expf (-2.0 * M_PI * kResponseTimeSecs / 30.f);
       #endif

        SharedData* const data = fSharedMem.getDataPointer();

        for (uint32_t b = 0; b < kNumBins; ++b)
            data->bins[b] *= tc;

        for (uint32_t i = 1; i < kDataSize - 1; ++i)
        {
            const float pab = analysis.powerAtBin(i);
            if (pab <= FFTAnalysis::kSmallestValue)
                continue;

            const float frq = analysis.freqAtBin(i);
            uint b = d_roundToUnsignedInt(kNumBins * std::logf (frq / 20.f) / log1k); // 20..20k
            if (b >= kNumBins) {
                continue;
            }
            if (b < 2) {
                b = 1;
            }
            if (const float pwr = 1.f - pab / FFTAnalysis::kSmallestValue; pwr > data->bins[b]) {
                data->bins[b] = pwr;
            }
        }

        __atomic_store_n(&data->hasNewData, true, __ATOMIC_RELAXED);
        return true;
    }

    float* get() noexcept
    {
        SharedData* const data = fSharedMem.getDataPointer();

        if (! __atomic_exchange_n(&data->hasNewData, false, __ATOMIC_RELAXED))
            return nullptr;

        return data->bins;
    }

private:
    struct SharedData {
        float bins[kNumBins];
        bool hasNewData;
    };

    SharedMemory<SharedData> fSharedMem;
};

// --------------------------------------------------------------------------------------------------------------------

template<uint numChannels>
class LibreAudioWaveformIPC {
public:
    struct ValueStructType {
        float ptr[numChannels];
    };
    using ValueType = std::conditional_t<numChannels != 1, ValueStructType, float>;

    LibreAudioWaveformIPC() = default;
    ~LibreAudioWaveformIPC() = default;

    const char* create()
    {
        DISTRHO_SAFE_ASSERT(! fFifoIsActive);

        if (! fSharedMem.create())
            return nullptr;

        SharedData* const fifos = fSharedMem.getDataPointer();
        fFifoControl.setFifo(&fifos->data, true);

        return fSharedMem.getDataFilename();
    }

    bool connect(const char* const filename)
    {
        if (fSharedMem.isCreatedOrConnected())
        {
            DISTRHO_SAFE_ASSERT(! fFifoIsActive);

            fFifoControl.setFifo(nullptr);
            fSharedMem.close();
        }

        if (SharedData* const fifos = fSharedMem.connect(filename))
        {
            fFifoControl.setFifo(&fifos->data);
            fFifoIsActive = true;
            return true;
        }

        return false;
    }

    void close()
    {
        fFifoIsActive = false;

        if (fSharedMem.isCreatedOrConnected())
        {
            if (SharedData* const fifos = fSharedMem.getDataPointer(); fifos != nullptr)
                fifos->closed = true;

            fSharedMem.close();
        }
    }

    bool isCreatedOrConnected() const noexcept
    {
        return fSharedMem.isCreatedOrConnected();
    }

    bool push(const ValueType& value)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fFifoIsActive, false);

        SharedData* const fifos = fSharedMem.getDataPointer();

        if (fifos == nullptr)
        {
            fFifoIsActive = false;
            return false;
        }
        if (fifos->closed)
        {
            fFifoIsActive = false;
            fSharedMem.close();
            return false;
        }

        fFifoControl.write(value);

        return true;
    }

    bool read(ValueType& value)
    {
        if (! fFifoControl.canRead())
            return false;

        value = fFifoControl.read();
        return true;
    }

private:
    static constexpr const uint32_t kFloatFifoSize = 2048;

    using FifoT = Fifo<ValueType, kFloatFifoSize>;
    using FifoControlT = FifoControl<ValueType, kFloatFifoSize>;

    struct SharedData {
        FifoT data;
        bool closed;
    };

    FifoControlT fFifoControl;
    SharedMemory<SharedData> fSharedMem;
    bool fFifoIsActive = false;
};

// template class LibreAudioWaveformIPC<1>;
// template class LibreAudioWaveformIPC<2>;

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
