// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "shader-clean.hpp"

#include "LibreAudioIPC.hpp"

#define NANOVG_GL2 1
#include "src/nanovg/nanovg.h"
#include "src/nanovg/nanovg_gl.h"
#include "src/nanovg/nanovg_gl_utils.h"

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

class WaveformWidget final : public LabWidget,
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
    explicit WaveformWidget(LabWidget* const parent, const float bottom, const float start, const float top)
        : LabWidget(parent)
    {
        setRange(bottom, start, top);
        fTextureData.resize(kTextureSize, bottom);
        fTextureDataParam.resize(kTextureSize, top);
    }

    void replace(const float values[kTextureSize])
    {
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

        fPointsPushed += 1.0;

        if (fTextureDataTail == kTextureSize)
            fTextureDataTail = 0;

        if (! fPendingDisplay)
        {
            fPendingDisplay = true;
            repaint();
        }
    }

    void pushGainReduction(const float value)
    {
        fTextureDataParam[fTextureDataTail] = value;
    }

    void setBorderRadius(const float radius)
    {
        fBorderRadius = radius;
    }

    void setRange(const float bottom, const float start, const float top)
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

    NVGcontext* fFbContext = nullptr;
    NVGLUframebuffer* fFramebuffer = nullptr;

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

        const uint head = d_roundToUnsignedInt(waveformHead());
        float y;

        beginPath();
        moveTo(0, h);

        for (uint i = 0; i < kTextureSize - 7; ++i)
        {
            const uint j = (i + head + 11) % kTextureSize;
            const float db = fTextureData[j];
            const float x = float(i) / (kTextureSize - 7) * w;

            if (db <= fBottom)
                y = h;
            else if (db >= fTop)
                y = 0;
            else
                y = h - h * ((db - fBottom) / fRange);

            lineTo(x, y);
        }

        lineTo(w, h);
        closePath();

        const Color color = Reference::Colors::acc1;
        fillPaint(linearGradient(0,
                                 (h - (fStart - fBottom) / fRange * h) * 0.5f,
                                 0,
                                 h,
                                 color.withAlpha(0.45f),
                                 color.withAlpha(0.f)));
        fill();

        if (hasFrameBuffer() && renderExtra(w, h, head))
        {
            beginPath();
            roundedRect(0, 0, w, h, fBorderRadius);
            fillPaint(imagePattern(0, 0, w, h, 0, *fFramebuffer, 1.0f));
            fill();
        }
    }

    bool hasFrameBuffer()
    {
        if (fFbContext == nullptr)
            fFbContext = nvgCreateSharedGL2(getContext(), NVG_ANTIALIAS);

        if (fFramebuffer != nullptr)
        {
            int w, h;
            nvgImageSize(fFbContext, fFramebuffer->image, &w, &h);
            if (w == (int)getWidth() && h == (int)getHeight())
                return true;
        }

        nvgluDeleteFramebuffer(fFramebuffer);

        fFramebuffer = nvgluCreateFramebuffer(fFbContext, getWidth(), getHeight(), 0);
        return fFramebuffer != nullptr;
    }

    bool renderExtra(const float w, const float h, const uint head)
    {
        nvgluBindFramebuffer(fFramebuffer);

        glViewport(0, 0, w, h);
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        nvgBeginFrame(fFbContext, w, h, 1.f);
        nvgBeginPath(fFbContext);

        nvgMoveTo(fFbContext, 0, -1);

        float y;
        for (uint i = 0; i < kTextureSize - 7; ++i)
        {
            const uint j = (i + head + 11) % kTextureSize;
            const float db = fTextureDataParam[j];
            const float x = float(i) / (kTextureSize - 7) * w;

            if (db <= fBottom)
                y = h;
            else if (db >= fTop)
                y = 0;
            else
                y = h - h * ((db - fBottom) / fRange);

            nvgLineTo(fFbContext, x,  y);
        }

        nvgLineTo(fFbContext, w, -1);
        nvgClosePath(fFbContext);

        nvgFillColor(fFbContext, Reference::Colors::acc6);
        nvgFill(fFbContext);

        nvgEndFrame(fFbContext);
        nvgluBindFramebuffer(nullptr);

        // restore viewport
        const Window& window(getWindow());
        glViewport(0, 0, window.getWidth(), window.getHeight());

        return true;
    }
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
