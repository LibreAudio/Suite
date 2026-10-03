// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "DistrhoPluginInfo.h"

#include "dpf/Fifo.hpp"
#include "dpf/SharedMemory.hpp"

#include <array>

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

// #define LIBREAUDIO_WAVEFORM_MONO
#define LIBREAUDIO_WAVEFORM_STEREO

#if defined(LIBREAUDIO_WAVEFORM_MONO)
using LibreAudioFifoType = float;
#elif defined(LIBREAUDIO_WAVEFORM_STEREO)
union LibreAudioFifoType {
    float ptr[DISTRHO_PLUGIN_NUM_OUTPUTS];
    struct {
        float l, r;
    };
};
#elif defined(LIBREAUDIO_WAVEFORM_FFT)
#error TODO
#endif

class LibreAudioIPC {
    static constexpr const uint32_t kFloatFifoSize = 2048;

    using FifoT = Fifo<LibreAudioFifoType, kFloatFifoSize>;
    using FifoControlT = FifoControl<LibreAudioFifoType, kFloatFifoSize>;

    struct LineGraphFifos {
        FifoT data;
        bool closed;
    };

    FifoControlT lineGraph;
    SharedMemory<LineGraphFifos> lineGraphData;
    bool lineGraphActive = false;

public:
    const char* create()
    {
        DISTRHO_SAFE_ASSERT(! lineGraphActive);

        if (! lineGraphData.create())
            return nullptr;

        LineGraphFifos* const fifos = lineGraphData.getDataPointer();
        lineGraph.setFifo(&fifos->data, true);

        return lineGraphData.getDataFilename();
    }

    bool connect(const char* const filename)
    {
        if (lineGraphData.isCreatedOrConnected())
        {
            DISTRHO_SAFE_ASSERT(! lineGraphActive);

            lineGraph.setFifo(nullptr);
            lineGraphData.close();
        }

        if (LineGraphFifos* const fifos = lineGraphData.connect(filename))
        {
            lineGraph.setFifo(&fifos->data);
            lineGraphActive = true;
            return true;
        }

        return false;
    }

    void close()
    {
        lineGraphActive = false;

        if (lineGraphData.isCreatedOrConnected())
        {
            if (LineGraphFifos* const fifos = lineGraphData.getDataPointer(); fifos != nullptr)
                fifos->closed = true;

            lineGraphData.close();
        }
    }

    bool isCreatedOrConnected() const noexcept
    {
        return lineGraphData.isCreatedOrConnected();
    }

    bool push(const LibreAudioFifoType& value)
    {
        DISTRHO_SAFE_ASSERT_RETURN(lineGraphActive, false);

        LineGraphFifos* const fifos = lineGraphData.getDataPointer();

        if (fifos == nullptr)
        {
            lineGraphActive = false;
            return false;
        }
        if (fifos->closed)
        {
            lineGraphActive = false;
            lineGraphData.close();
            return false;
        }

        lineGraph.write(value);

        return true;
    }

    bool read(LibreAudioFifoType& value)
    {
        if (! lineGraph.canRead())
            return false;

        value = lineGraph.read();
        return true;
    }
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
