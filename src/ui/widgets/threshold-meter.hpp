// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../_lab/knob.hpp"

#include "../reference.hpp"

#include "LibreAudioParameters.hpp"

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

template <class R = Reference::Widgets::ThresholdMeter<>>
class ThresholdMeterWidget final : public LabKnobWidget
{
    using BaseWidget = LabKnobWidget;

    static constexpr const float ticks[] = { -6, -12, -24, -36, -48 };

public:
    ThresholdMeterWidget(LabWidget* const parent,
                         const FaustParameterIndex slider,
                         const FaustParameterIndex meterL,
                         const FaustParameterIndex meterR)
        : BaseWidget(parent, kParametersMainStart + slider),
          kParameterIdSlider(slider),
          kParameterIdMeterL(meterL),
          kParameterIdMeterR(meterR),
          kParameterSlider(kFaustParameters[slider]),
          kParameterMeterL(kFaustParameters[meterL]),
          kParameterMeterR(kFaustParameters[meterR])
    {
        updateReferenceSize<R>();

        setName(kParameterSlider.label);
        setDefault(kParameterSlider.init);
        setRange(kParameterSlider.min, kParameterSlider.max);
        setStep(kParameterSlider.step);
        setUsingLogScale(kParameterSlider.isLogarithmic);
        setValue(kParameterSlider.init, false);

        updateSize(false);

        DISTRHO_CUSTOM_SAFE_ASSERT("gain vs meter max mismatch", kParameterSlider.max <= kParameterMeterL.max);
        DISTRHO_CUSTOM_SAFE_ASSERT("gain vs meter min mismatch", kParameterSlider.min >= kParameterMeterL.min);
        DISTRHO_CUSTOM_SAFE_ASSERT("meter L vs R max mismatch", kParameterMeterL.max == kParameterMeterR.max);
        DISTRHO_CUSTOM_SAFE_ASSERT("meter L vs R min mismatch", kParameterMeterL.min == kParameterMeterR.min);
    }

private:
    const uint kParameterIdSlider;
    const uint kParameterIdMeterL;
    const uint kParameterIdMeterR;

    const FaustParameter& kParameterSlider;
    const FaustParameter& kParameterMeterL;
    const FaustParameter& kParameterMeterR;

    float fMeterValueL = kParameterMeterL.min;
    float fMeterValueR = kParameterMeterR.min;

    [[nodiscard]] float meterLevelToHeight(const float db, const float height) noexcept
    {
        const float normalized = 1.f - d_clamp((db - kParameterMeterL.min) / (kParameterMeterL.max - kParameterMeterL.min), 0.f, 1.f);
        // const float normalized = 1.f - d_clamp(std::pow(10.f, db * 0.05f), 0.f, 1.f);
        return d_roundToIntPositive(height * normalized);
    }

    void idleCallback() final
    {
        if (const float valueL = std::clamp(fInterface->getParameterValue(kParametersMainStart + kParameterIdMeterL),
                                            kParameterMeterL.min,
                                            kParameterMeterL.max); d_isNotEqual(fMeterValueL, valueL))
        {
            fMeterValueL = valueL;
            repaint();
        }

        if (const float valueR = std::clamp(fInterface->getParameterValue(kParametersMainStart + kParameterIdMeterR),
                                            kParameterMeterR.min,
                                            kParameterMeterR.max); d_isNotEqual(fMeterValueR, valueR))
        {
            fMeterValueR = valueR;
            repaint();
        }

        BaseWidget::idleCallback();
    }

    void onNanoDisplay() final
    {
        const float border = R::border * fScaleFactor;
        const float margin = R::margin * fScaleFactor;
        const float padding = R::padding * fScaleFactor;
        const float w = getWidth();
        const float h = getHeight();

        const float sw = R::Slider::width * fScaleFactor;

        const float mborder = R::Meter::border * fScaleFactor;
        const float mx = border + margin + sw + padding;
        const float my = border + margin;
        const float mw = w - (border + margin) * 2 - sw - padding;
        const float mh = h - (border + margin) * 2;

        const float tx = mx + mborder;
        const float ty = my + mborder;
        const float tw2 = mw - mborder * 2;
        const float tw1 = R::Track::width * fScaleFactor - 1.5f * fScaleFactor;
        const float th = mh - mborder * 2;

        // ------------------------------------------------------------------------------------------------------------
        // draw background

        drawReferenceBackground<R>();

        // ------------------------------------------------------------------------------------------------------------
        // meter area

        // background
        beginPath();

        if constexpr (R::Meter::borderRadius != 0)
            roundedRect(mx, my, mw, mh, R::Meter::borderRadius * fScaleFactor);
        else
            rect(mx, my, mw, mh);

        fillColor(R::Meter::backgroundColor);
        fill();

        // meter tracks
        beginPath();

        if constexpr (R::Meter::borderRadius != 0)
            roundedRect(tx, ty, tw2, th, R::Meter::borderRadius * fScaleFactor);
        else
            rect(tx, ty, tw2, th);

        if (d_isNotEqual(fMeterValueL, kParameterMeterL.min))
        {
            const float vy = meterLevelToHeight(fMeterValueL, th);
            const float vh = th - vy;
            const float ly = ty + vy;

            fillPaint(linearGradient(0, ly, 0, ly + vh, R::Track::colorGradientStart, R::Track::colorGradientStop));

            save();
            scissor(tx, ly, tw1, vh);
            fill();
            restore();
        }

        if (d_isNotEqual(fMeterValueR, kParameterMeterR.min))
        {
            const float vy = meterLevelToHeight(fMeterValueR, th);
            const float vh = th - vy;
            const float ly = ty + vy;

            fillPaint(linearGradient(0, ly, 0, ly + vh, R::Track::colorGradientStart, R::Track::colorGradientStop));

            save();
            scissor(w - border - mborder - tw1, ly, tw1, vh);
            fill();
            restore();
        }

        beginPath();

        if constexpr (R::Meter::borderRadius != 0)
            roundedRect(mx, my, mw, mh, R::Meter::borderRadius * fScaleFactor);
        else
            rect(mx, my, mw, mh);

        // inset shading
        // FIXME define sizes elsewhere
        fillPaint(linearGradient(0, my, 0, my + 4 * fScaleFactor, R::Meter::insetShadingColor, Reference::Colors::transparent));
        fill();

        // border
        if constexpr (R::Meter::border != 0)
        {
            strokeColor(R::backgroundColor);
            strokeWidth(mborder * 0.5f);
            stroke();
        }

        // ------------------------------------------------------------------------------------------------------------
        // draw ticks

        strokeColor(R::Tick::color);
        strokeWidth(R::Tick::height * fScaleFactor);

        for (float tick : ticks)
        {
            const float lh = meterLevelToHeight(tick, th);

            beginPath();
            moveTo(tx, ty + lh);
            lineTo(tx + tw2, ty + lh);
            stroke();
        }

        // ------------------------------------------------------------------------------------------------------------
        // slider

        {
            const float sx = border + margin;
            const float sy = border + margin + meterLevelToHeight(getValue(), th);

            beginPath();
            moveTo(sx, sy - (sw - padding) * 0.5f);
            lineTo(sx + sw - padding, sy);
            lineTo(sx, sy + (sw - padding) * 0.5f);
            closePath();
            fillColor(R::Slider::color);
            fill();

            beginPath();
            moveTo(sx + sw + padding, sy);
            lineTo(w - border - margin, sy);
            strokeColor(R::Slider::color);
            strokeWidth(1 * fScaleFactor);
            stroke();
        }

        // ------------------------------------------------------------------------------------------------------------
        // draw border

        drawReferenceBorder<R>();
    }

    void updateSize(const bool updateChildren) final
    {
        updateReferenceSize<R>();
        BaseWidget::updateSize(updateChildren);
    }
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
