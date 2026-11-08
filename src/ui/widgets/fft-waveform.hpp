// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "config.h"
#include "../base.hpp"
#include "../_lab/metrics.hpp"
#include "shader-clean.hpp"

#include "LibreAudioIPC.hpp"

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

class WaveformWidget final : public LabWidget,
                             public GraphInterface,
                            #ifdef LIBREAUDIO_WANT_GRAPH_PARAM
                             private LabWidget::FramebufferCallback,
                            #endif
                             private IdleCallback
{
   #if LIBREAUDIO_WANT_GRAPH_ANALYZER
    static constexpr const uint32_t kTextureSize = LibreAudioAnalyzerIPC::kNumBins;
   #elif LIBREAUDIO_WANT_GRAPH_WAVEFORM
    static constexpr const uint32_t kTextureSize = kNumSamplePointsForWaveform;
   #else
    static constexpr const uint32_t kTextureSize = 1;
   #endif

    // TODO rework smoothness handling
   #if LIBREAUDIO_WANT_GRAPH_WAVEFORM
    static constexpr const uint kUsableTextureSize = kTextureSize - 7;
   #else
    static constexpr const uint kUsableTextureSize = kTextureSize;
   #endif

    // static_assert(R::width == 0, "waveform must have 0 width, size is adjusted by its parent");
    // static_assert(R::height == 0, "waveform must have 0 height, size is adjusted by its parent");

public:
    explicit WaveformWidget(LabWidget* const parent, const float bottom, const float start, const float top)
       #ifdef LIBREAUDIO_WANT_GRAPH_PARAM
        : LabWidget(parent, this)
       #else
        : LabWidget(parent)
       #endif
    {
        setRange(bottom, start, top);
        fTextureData.resize(kTextureSize, bottom);
        fTextureDataParam.resize(kTextureSize, top);
    }

   #if LIBREAUDIO_WANT_GRAPH_ANALYZER
    void replace(const float values[kTextureSize])
    {
        std::memcpy(fTextureData.data(), values, kTextureSize * sizeof(float));

        if (! fPendingDisplay)
        {
            fPendingDisplay = true;
            repaint();
        }
    }
   #elif LIBREAUDIO_WANT_GRAPH_WAVEFORM
    void push(const float value
           #if LIBREAUDIO_WANT_GRAPH_IO_COUNT == 2
            , const float value2
           #endif
             )
    {
        fTextureData[fTextureDataTail] = value;
       #if LIBREAUDIO_WANT_GRAPH_IO_COUNT == 2
        fTextureDataParam[fTextureDataTail] = value2;
       #endif

        fPointsPushed += 1.0;

        if (++fTextureDataTail == kTextureSize)
            fTextureDataTail = 0;

        if (! fPendingDisplay)
        {
            fPendingDisplay = true;
            repaint();
        }
    }
   #endif

    void setBorderRadius(const float radius) override
    {
        fBorderRadius = radius;
    }

    void setRange(const float bottom, const float start, const float top) override
    {
        DISTRHO_SAFE_ASSERT_RETURN(bottom < top,);

        fBottom = bottom;
        fStart = start;
        fTop = top;
        fRange = top - bottom;
    }

private:
    float fBottom = -24.f;
    float fStart = fBottom;
    float fTop = 0.f;
    float fRange = fTop - fBottom;
    float fBorderRadius = 0.f;

    std::vector<float> fTextureData;
    std::vector<float> fTextureDataParam;
    uint32_t fTextureDataTail = 0;
    bool fPendingDisplay = true;

   #if LIBREAUDIO_WANT_GRAPH_WAVEFORM
    // FIXME rework this
    double fPointsPushed = 0;
    double fPointsShown = 0;
    double fLastTime = 0;
    double fHead = 0;
    double waveformHead()
    {
        static constexpr const double kWaveformSize = kNumSamplePointsForWaveform;
        static constexpr const double kWaveformRate = kWaveformSize / kNumSecondsForWaveform;
        static constexpr const double kWaveformLag = kWaveformRate * 0.05;

        const double now = getApp().getTime();
        if (d_isNotZero(fLastTime) && now - fLastTime < 0.001)
            return fHead;

        const double dt = d_isNotZero(fLastTime) ? std::min<double>(now - fLastTime, 0.1) : 0;
        fLastTime = now;

        fPointsShown += dt * kWaveformRate;

        const double error = fPointsPushed - kWaveformLag - fPointsShown;

        if (std::abs(error) > kWaveformRate * 0.5)
            fPointsShown += error;
        else
            fPointsShown += error * std::min<double>(1.0, dt * 2);

        fPointsShown = std::min(fPointsShown, fPointsPushed);

        double head = std::fmod(fPointsShown - 2, kWaveformSize);
        if (head < 0)
            head += kWaveformSize;
        fHead = head;

        return fHead;
    }
   #endif

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

        // FIXME
        const LabMetrics<Reference::CentralArea> m { this };

        const float w = m.width - m.border * 2;
        const float h = m.height - m.border * 2;

      #if LIBREAUDIO_WANT_GRAPH_WAVEFORM
       #ifdef LIBREAUDIO_WANT_GRAPH_PARAM
        const uint head = d_roundToUnsignedInt(fHead);
       #else
        const uint head = d_roundToUnsignedInt(waveformHead());
       #endif
      #endif

        float y;

        beginPath();
        moveTo(m.border, m.border + h);

        for (uint i = 0; i < kUsableTextureSize; ++i)
        {
           #if LIBREAUDIO_WANT_GRAPH_WAVEFORM
            const uint j = (i + head + 11) % kTextureSize;
            const float db = fTextureData[j];
           #else
            const float db = fTextureData[i];
           #endif
            const float x = float(i) / kUsableTextureSize * w;

            if (db <= fBottom)
                y = h;
            else if (db >= fTop)
                y = 0;
            else
                y = h - h * ((db - fBottom) / fRange);

            lineTo(m.border + x, m.border + y);
        }

        lineTo(m.border + w, m.border + h);
        closePath();

        const Color color = Reference::Colors::acc1;
        fillPaint(linearGradient(0,
                                 m.border + (h - (fStart - fBottom) / fRange * h) * 0.5f,
                                 0,
                                 m.border + h,
                                 color.withAlpha(0.45f),
                                 color.withAlpha(0.f)));
        fill();

       #ifdef LIBREAUDIO_WANT_GRAPH_PARAM
        if (const NanoImage fbimg = createImageFromFramebuffer(); fbimg.isValid())
        {
            beginPath();
            roundedRect(m.border, m.border, w, h, fBorderRadius);
            fillPaint(imagePattern(0, 0, m.width, m.height, 0, fbimg, 1.0f));
            fill();
        }
       #endif
    }

   #ifdef LIBREAUDIO_WANT_GRAPH_PARAM
    void onNanoFramebufferDisplay() final
    {
        // FIXME
        const LabMetrics<Reference::CentralArea> m { this };

        const float w = m.width - m.border * 2;
        const float h = m.height - m.border * 2;
        const float a = 0.7f;

       #if LIBREAUDIO_WANT_GRAPH_WAVEFORM
        const uint head = d_roundToUnsignedInt(waveformHead());
       #endif

        translate(m.border, m.border);

        beginPath();
        moveTo(0, -1);

        float lastpc = 0.f;
        float lastx = 0.f;
        float y;
        int acc = 1;
        for (uint i = 0; i < kUsableTextureSize; ++i)
        {
           #if LIBREAUDIO_WANT_GRAPH_WAVEFORM
            const uint j = (i + head + 11) % kTextureSize;
            const float db = fTextureDataParam[j];
           #else
            const float db = fTextureDataParam[i];
           #endif
            const float pc = float(i) / kUsableTextureSize;
            const float x = pc * w;

            if (db <= fBottom)
                y = h;
            else if (db >= fTop)
                y = 0;
            else
                y = h - h * ((db - fBottom) / fRange);

            lineTo(x, y);

            if (pc - lastpc > 0.25f)
            {
                lineTo(x, -1);
                closePath();
                fillPaint(linearGradient(lastx,
                                         0,
                                         x,
                                         0,
                                         Reference::kAccentColors[acc].withAlpha(a),
                                         Reference::kAccentColors[acc + 1].withAlpha(a)));
                fill();

                beginPath();
                moveTo(x - 0.5f, -1);
                lineTo(x - 0.5f, y);

                ++acc;
                lastpc = pc;
                lastx = x;
            }
        }

        lineTo(w, -1);
        closePath();

        fillPaint(linearGradient(lastx,
                                 0,
                                 w,
                                 0,
                                 Reference::kAccentColors[acc].withAlpha(a),
                                 Reference::kAccentColors[acc + 1].withAlpha(a)));
        fill();
    }
   #endif
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
