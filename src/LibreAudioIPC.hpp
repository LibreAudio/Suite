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

class LibreAudioIPC {
    static constexpr const uint32_t kFloatFifoSize = 2048;

    using FloatFifoN = FloatFifo<kFloatFifoSize>;
    using FloatFifoControlN = FloatFifoControl<kFloatFifoSize>;

    struct LineGraphFifos {
        FloatFifoN data[DISTRHO_PLUGIN_NUM_OUTPUTS];
        bool closed;
    };

    FloatFifoControlN lineGraphs[DISTRHO_PLUGIN_NUM_OUTPUTS];
    SharedMemory<LineGraphFifos> lineGraphsData;
    bool lineGraphActive = false;

public:
    const char* create()
    {
        DISTRHO_SAFE_ASSERT(! lineGraphActive);

        if (! lineGraphsData.create())
            return nullptr;

        LineGraphFifos* const fifos = lineGraphsData.getDataPointer();

        for (uint8_t i = 0; i < DISTRHO_PLUGIN_NUM_OUTPUTS; ++i)
            lineGraphs[i].setFifo(&fifos->data[i], true);

        return lineGraphsData.getDataFilename();
    }

    bool connect(const char* const filename)
    {
        if (lineGraphsData.isCreatedOrConnected())
        {
            DISTRHO_SAFE_ASSERT(! lineGraphActive);

            for (uint8_t i = 0; i < DISTRHO_PLUGIN_NUM_OUTPUTS; ++i)
                lineGraphs[i].setFifo(nullptr);

            lineGraphsData.close();
        }

        if (LineGraphFifos* const fifos = lineGraphsData.connect(filename))
        {
            for (uint8_t i = 0; i < DISTRHO_PLUGIN_NUM_OUTPUTS; ++i)
                lineGraphs[i].setFifo(&fifos->data[i]);

            lineGraphActive = true;
            return true;
        }

        return false;
    }

    void close()
    {
        lineGraphActive = false;

        if (lineGraphsData.isCreatedOrConnected())
        {
            if (LineGraphFifos* const fifos = lineGraphsData.getDataPointer(); fifos != nullptr)
                fifos->closed = true;

            lineGraphsData.close();
        }
    }

    bool isCreatedOrConnected() const noexcept
    {
        return lineGraphsData.isCreatedOrConnected();
    }

    bool push(const std::array<float, DISTRHO_PLUGIN_NUM_OUTPUTS>& values)
    {
        DISTRHO_SAFE_ASSERT_RETURN(lineGraphActive, false);

        LineGraphFifos* const fifos = lineGraphsData.getDataPointer();

        if (fifos == nullptr)
        {
            lineGraphActive = false;
            return false;
        }
        if (fifos->closed)
        {
            lineGraphActive = false;
            lineGraphsData.close();
            return false;
        }

        for (uint8_t i = 0; i < DISTRHO_PLUGIN_NUM_OUTPUTS; ++i)
            lineGraphs[i].write(values[i]);

        return true;
    }

    bool read(std::array<float, DISTRHO_PLUGIN_NUM_OUTPUTS>& values)
    {
        if (! lineGraphs[0].canRead())
            return false;

        for (uint8_t i = 0; i < DISTRHO_PLUGIN_NUM_OUTPUTS; ++i)
            values[i] = lineGraphs[i].read();

        return true;
    }
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
