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
   #if LIBREAUDIO_WANT_GRAPH_ANALYZER
    static constexpr const uint32_t kTextureSize = LibreAudioAnalyzerIPC::kNumBins;
   #elif LIBREAUDIO_WANT_GRAPH_WAVEFORM
    static constexpr const uint32_t kTextureSize = kNumSamplePointsForWaveform;
   #else
    static constexpr const uint32_t kTextureSize = 0;
   #endif

    // static_assert(R::width == 0, "waveform must have 0 width, size is adjusted by its parent");
    // static_assert(R::height == 0, "waveform must have 0 height, size is adjusted by its parent");

public:
    // , const float min, const float max, const float fade
    explicit WaveformWidget(TopLevelWidget* const parent, LabUIWidgetInterface* const iface)
        : NanoSubWidget(parent),
          ShaderWidgetInterface(iface)
    {
        // setRange(min, max, fade);

        fTextureData.resize(kTextureSize);
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

    void replace(const float values[kTextureSize])
    {
        d_stdout("push %f", values[kTextureSize / 2]);

        std::memcpy(fTextureData.data(), values, kTextureSize * sizeof(float));

        if (! fPendingDisplay)
        {
            fPendingDisplay = true;
            repaint();
        }
    }

    void push(const float value)
    {
        fTextureData[fTextureDataTail++] = value;

        if (fTextureDataTail == kTextureSize)
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

        for (uint i = 0; i < kTextureSize; ++i)
        {
            const uint j = (i + fTextureDataTail) % kTextureSize;
            lineTo(float(i) / (kTextureSize - 1) * w, h * 0.8f - h * fTextureData[j] * 0.8f);
            // lineTo(float(i) / (kTextureSize - 1) * w, h * 0.8f - h * (std::clamp(fTextureData[j], 0.5f, 1.f) - 0.5f) * 2.f * 0.8f);
        }

        lineTo(w, h);
        closePath();

        // Color color(0.5569, 0.6667, 0.8471);
        Color color(0.3608, 0.4314, 0.5569);
        fillPaint(linearGradient(0, h * 0.6f, 0, h, color, color.withAlpha(0.f)));
        // fillColor(color);
        fill();
    }
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
