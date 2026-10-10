// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dpf/Fifo.hpp"
#include "dpf/SharedMemory.hpp"

#include "fft.hpp"

#include <array>
#include <cmath>

#ifndef M_PIf
#define M_PIf static_cast<float>(M_PI)
#endif

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

// Overall target frame-rate for graphs (FFT analyzer and Waveform)
inline constexpr const uint32_t kTargetFrameRate = 60;

// Idle time as double of target rate, so that we don't miss a frame in the worst case scenario
// Repaints must only be requested after pending drawing completes, which ensures we don't bottleneck the system
inline constexpr const uint32_t kTargetIdleTimeMs = d_roundToUnsignedInt(1000.0 / (kTargetFrameRate * 2));

// How many seconds the waveform area should hold
inline constexpr const uint32_t kNumSecondsForWaveform = 8;

// How many samples to use
inline constexpr const uint32_t kNumSamplePointsForWaveform = 8192;

// --------------------------------------------------------------------------------------------------------------------

inline constexpr float db2coef(float db)
{
    return db > FFTAnalysis::kSmallestValue ? std::pow(10.f, db * 0.05f) : 0.f;
}

inline constexpr float coef2db(float coef)
{
    return std::pow(2.0, coef) * (-FFTAnalysis::kSmallestValue) - FFTAnalysis::kSmallestValue;
}

// coef = (+pow(2.0, coef * factor) - 1.0) / logfac * (max - min) + min;

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioAnalyzerIPC {
public:
    static constexpr const uint32_t kNumBins = 1024;
    static constexpr const uint32_t kWindowSize = kNumBins * 4;

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
        fTmpData.fill(0.f);

        return fSharedMem.getDataFilename();
    }

    bool connect(const char* const filename)
    {
        if (fSharedMem.isCreatedOrConnected())
            fSharedMem.close();

        fTmpData.fill(0.f);
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
        static constexpr const float tc = std::exp(-2.f * M_PIf * kResponseTimeSecs / 30.f);
       #else
        const float tc = std::exp(-2.f * M_PIf * kResponseTimeSecs / 30.f);
       #endif

        SharedData* const data = fSharedMem.getDataPointer();
        float* const tmp = fTmpData.data();

        for (uint32_t b = 0; b < kNumBins; ++b)
            tmp[b] = data->bins[b] * tc;

        bool hasBin[kNumBins] = {};
        float norm, pab, pwr;

        for (uint32_t i = 1; i < kDataSize - 1; ++i)
        {
            pab = analysis.powerAtBin(i);
            if (pab <= FFTAnalysis::kSmallestValue)
                continue;

            const float frq = analysis.freqAtBin(i);
            uint b = d_roundToUnsignedInt(kNumBins * std::log(frq / 20.f) / log1k); // 20..20k
            if (b >= kNumBins - 1)
                continue;
            if (b < 2)
                b = 1;

            hasBin[b] = true;
            if (pwr = 1.f - pab / FFTAnalysis::kSmallestValue; pwr > tmp[b]) {
                tmp[b] = pwr;
            }
        }

        // linear interpolation for bins without data
        {
            static constexpr const uint32_t kMaxBinGaps = kNumBins / 8;

            for (uint32_t b = 1, gap = 0, good = 0; b < kNumBins - 1; ++b)
            {
                if (hasBin[b])
                {
                    if (gap != 0 && b - gap < kMaxBinGaps)
                    {
                        for (uint32_t b2 = gap; b2 < b; ++b2)
                        {
                            norm = static_cast<float>(b2 - gap + 1) / (b - gap + 1);
                            // db1 = (1.f - tmp[good]) * FFTAnalysis::kSmallestValue;
                            // db2 = (1.f - tmp[b]) * FFTAnalysis::kSmallestValue;
                            // pab2 = db1 * (1.f - norm) + db2 * norm;
                            // pwr = 1.f - pab2 / FFTAnalysis::kSmallestValue;
                            if (pwr = tmp[good] * (1.f - norm) + tmp[b] * norm; pwr > tmp[b2])
                                tmp[b2] = pwr;
                        }
                    }
                    gap = 0;
                    good = b;
                }
                else
                {
                    if (gap == 0)
                        gap = b;
                }
            }
        }

        // smoothing/blur  (change i < ???)
        for (uint32_t i = 0; i < 12; ++i)
            blur(tmp);

        std::memcpy(data->bins, tmp, sizeof(float) * kNumBins);

        // __atomic_store_n(&data->hasNewData, true, __ATOMIC_RELAXED);
        return true;
    }

    const float* get() noexcept
    {
        SharedData* const data = fSharedMem.getDataPointer();

        // if (! __atomic_exchange_n(&data->hasNewData, false, __ATOMIC_RELAXED))
        //     return nullptr;

        float* const tmp = fTmpData.data();

        // copying while mixing previous data for smoother updates
        for (uint32_t b = 0; b < kNumBins; ++b)
            tmp[b] = tmp[b] * 0.9f + data->bins[b] * 0.1f;

        return tmp;
    }

private:
    struct SharedData {
        float bins[kNumBins];
        // bool hasNewData;
    };

    SharedMemory<SharedData> fSharedMem;
    std::array<float, kNumBins> fTmpData;

    static inline void blur(float d[kNumBins])
    {
        d[0] = (d[0] + d[1]) * 0.5f;
        d[0] = (d[0] + d[1] + d[2]) * 0.3333f;

        for (uint32_t b = 2; b < kNumBins - 2; ++b)
            d[b] = (d[b] + d[b - 1] + d[b - 2] + d[b + 1] + d[b + 2]) * 0.2f;

        d[kNumBins - 2] = (d[kNumBins - 1] + d[kNumBins - 2] + d[kNumBins - 3]) * 0.3333f;
        d[kNumBins - 1] = (d[kNumBins - 1] + d[kNumBins - 2]) * 0.5f;

    };
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
