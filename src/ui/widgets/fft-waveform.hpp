// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "shader-clean.hpp"

#include "LibreAudioIPC.hpp"

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

class WaveformWidget final : public NanoSubWidget,
                             public ShaderWidgetInterface,
                             private IdleCallback
{
    // static_assert(R::width == 0, "waveform must have 0 width, size is adjusted by its parent");
    // static_assert(R::height == 0, "waveform must have 0 height, size is adjusted by its parent");

public:
    // , const float min, const float max, const float fade
    explicit WaveformWidget(TopLevelWidget* const parent, LabUIWidgetInterface* const iface)
        : NanoSubWidget(parent),
          ShaderWidgetInterface(iface)
    {
        // setRange(min, max, fade);

        fTextureData.resize(kNumSamplePointsForWaveform);
    }

    void setAbsolutePos(const Point<int>& pos) final
    {
        NanoSubWidget::setAbsolutePos(pos);
    }

    void setSize(const Size<uint>& size) final
    {
        NanoSubWidget::setSize(size);
    }

    void setBorderRadius(const float borderRadius) final
    {
        ShaderWidgetInterface::setBorderRadius(borderRadius);
        repaint();
    }

    void push(const float value)
    {
        // d_stdout("push %f", value);
        fTextureData[fTextureDataTail++] = value;

        if (fTextureDataTail == kNumSamplePointsForWaveform)
            fTextureDataTail = 0;

        if (! fPendingDisplay)
        {
            fPendingDisplay = true;
            repaint();
        }
    }

    void setRange(const float min, const float max, const float fade)
    {
        DISTRHO_SAFE_ASSERT_RETURN(min < max,);

        fMin = min;
        fMax = max;
        fFade = fade;
    }

private:
    float fMin = -24.f;
    float fMax = 0.f;
    float fFade = 0.f;

    std::vector<float> fTextureData;
    uint32_t fTextureDataTail = 0;
    bool fPendingDisplay = true;

    void idleCallback() final
    {
        if (! fPendingDisplay)
        {
            fPendingDisplay = true;
            repaint();
        }
    }

    void onNanoDisplay() final
    {
        fPendingDisplay = false;

        const float w = getWidth();
        const float h = getHeight();

        beginPath();
        moveTo(0, h);

        for (uint i = 0; i < kNumSamplePointsForWaveform; ++i)
            lineTo(float(i) / (kNumSamplePointsForWaveform - 1) * w, h - h * fTextureData[i]);

        lineTo(w, h);
        closePath();

        fillColor(0xff, 0, 0);
        fill();
    }
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
