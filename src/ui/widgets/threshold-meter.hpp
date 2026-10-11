// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../_lab/knob.hpp"

#include "../reference.hpp"

#include "LibreAudioParameters.hpp"

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

class ThresholdMeterWidget final : public LabKnobWidget
{
    using R = Reference::Widgets::ThresholdMeter;
    using BaseWidget = LabKnobWidget;

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

    [[nodiscard]] float db2height(const float db, const float height) noexcept
    {
        const float normalized = d_clamp((db - kParameterSlider.min) / (kParameterSlider.max - kParameterSlider.min), 0.f, 1.f);
        return d_roundToIntPositive(height * normalized);
    }

    void idleCallback() final
    {
        if (const float valueL = std::clamp(fInterface->getParameterValue(kParameterIdMeterL),
                                            kParameterMeterL.min,
                                            kParameterMeterL.max); d_isNotEqual(fMeterValueL, valueL))
        {
            fMeterValueL = valueL;
            repaint();
        }

        if (const float valueR = std::clamp(fInterface->getParameterValue(kParameterIdMeterR),
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

        const float mx = border + margin + sw + padding;
        const float my = border + margin;
        const float mw = w - (border + margin) * 2 - sw - padding;
        const float mh = h - (border + margin) * 2;

        // ------------------------------------------------------------------------------------------------------------
        // draw background

        drawReferenceBackground<R>();

        // ------------------------------------------------------------------------------------------------------------
        // meter area

        {
            const float mborder = R::border * fScaleFactor;
            const float mborderRadius = R::Meter::borderRadius * fScaleFactor;

            // inset shading
            beginPath();
            roundedRect(mx, my, mw, mh, mborderRadius);
            fillColor(R::Meter::backgroundColor);
            fill();
            fillPaint(linearGradient(0, my, 0, my + 4 * fScaleFactor, R::Meter::insetShadingColor, Reference::Colors::transparent));
            fill();
            strokeColor(R::backgroundColor);
            strokeWidth(mborder * 0.5f);
            stroke();

            // meters
            fillPaint(linearGradient(0, my, 0, mh, R::Track::colorGradientStart, R::Track::colorGradientStop));

            if (d_isNotEqual(fMeterValueL, kParameterMeterL.min))
            {
                const float lh = db2height(fMeterValueL, mh);

                beginPath();
                rect(mx, my + lh, mw * 0.5f, my + mh - lh);
                fill();
            }

            if (d_isNotEqual(fMeterValueR, kParameterMeterR.min))
            {
                const float rh = db2height(fMeterValueR, mh);

                beginPath();
                rect(mx + mw * 0.5f, my + rh, mw * 0.5f, my + mh - rh);
                fill();
            }
        }

        // ------------------------------------------------------------------------------------------------------------
        // slider

        {
            const float sx = border + margin;
            const float sy = border + margin + (1.f - getNormalizedValue()) * (h - (border + margin) * 2);

            beginPath();
            moveTo(sx, sy - sw * 0.5f);
            lineTo(sx + sw, sy);
            lineTo(sx, sy + sw * 0.5f);
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
