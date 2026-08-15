// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../base/base.hpp"
#include "../base/interface.hpp"
#include "../reference.hpp"

#include "LibreAudioParameters.hpp"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

enum LibreAudioMeterWidgetType {
    Input,
    Output
};

template<LibreAudioMeterWidgetType type>
class LibreAudioMeterWidget final : public LibreAudioWidget,
                                    private IdleCallback
{
    using R = LibreAudioReference::Meter;

    static constexpr const uint kParameterL = type == Input
        ? kParametersInputStart + common_input::kFaustParameterInput_peak_l
        : kParametersOutputStart + common_output::kFaustParameterOutput_peak_l - 1;
    static constexpr const uint kParameterR = type == Input
        ? kParametersInputStart + common_input::kFaustParameterInput_peak_r
        : kParametersOutputStart + common_output::kFaustParameterOutput_peak_r - 1;

public:
    LibreAudioMeterWidget(LibreAudioWidget* const parent)
        : LibreAudioWidget(parent)
    {
        addIdleCallback(this);

        if constexpr (R::width != 0)
            LibreAudioWidget::setWidth(d_roundToUnsignedInt(R::width * fScaleFactor));

        if constexpr (R::height != 0)
            LibreAudioWidget::setHeight(d_roundToUnsignedInt(R::height * fScaleFactor));
    }

private:
    // FIXME non-hardcoded
    static constexpr const float min = -70.0;
    static constexpr const float max = 24.0;
    static constexpr const float diff = max - min;

    float fValueL = min;
    float fValueR = min;

    void idleCallback() final
    {
        if (const float valueL = std::clamp(fInterface->getParameterValue(kParameterL), min, max);
            d_isNotEqual(fValueL, valueL))
        {
            fValueL = valueL;
            repaint();
        }

        if (const float valueR = std::clamp(fInterface->getParameterValue(kParameterR), min, max);
            d_isNotEqual(fValueR, valueR))
        {
            fValueR = valueR;
            repaint();
        }
    }

    void onNanoDisplay() final
    {
        const float w = getWidth();
        const float h = getHeight();

        // ------------------------------------------------------------------------------------------------------------
        // draw background

        beginPath();

        if constexpr (R::borderRadius != 0)
            roundedRect(0, 0, w, h, R::borderRadius * fScaleFactor);
        else
            rect(0, 0, w, h);

        if constexpr (d_isNotZero(R::backgroundColor.alpha))
        {
            fillColor(R::backgroundColor);
            fill();
        }

        // ------------------------------------------------------------------------------------------------------------
        // draw meters

        {
            const float ts = (R::border + R::margin) * fScaleFactor;
            const float tc = ts + (w - ts * 2) * 0.5f;
            const float tw = R::Track::width * fScaleFactor - 0.5f * fScaleFactor;

            fillPaint(linearGradient(0, 0, 0, h, R::Track::colorGradientStart, R::Track::colorGradientStop));

            // scissor(R::border * fScaleFactor,
            //         R::border * fScaleFactor,
            //         w - R::border * 2 * fScaleFactor,
            //         h - R::border * 2 * fScaleFactor);

            if (d_isNotEqual(fValueL, min))
            {
                const float lh = (h - ts) * (1.f - (fValueL - min) / diff);

                beginPath();
                rect(ts, ts + lh, tw, h - ts - lh);
                fill();
            }

            if (d_isNotEqual(fValueR, min))
            {
                const float rh = (h - ts) * (1.f - (fValueR - min) / diff);

                beginPath();
                rect(tc + 0.5f, ts + rh, tw, h - ts - rh);
                fill();
            }
        }

        // ------------------------------------------------------------------------------------------------------------
        // draw border

        if constexpr (R::border != 0 && d_isNotZero(R::borderColor.alpha))
        {
            beginPath();

            if constexpr (R::borderRadius != 0)
                roundedRect(0, 0, w, h, R::borderRadius * fScaleFactor);
            else
                rect(0, 0, w, h);

            strokeColor(R::borderColor);
            strokeWidth(R::border * 2 * fScaleFactor);
            stroke();
        }

        // ------------------------------------------------------------------------------------------------------------
        // draw ticks

        {
            const float ts = (R::border + R::margin) * fScaleFactor;
            const float te = w - ts;

            strokeWidth(R::Tick::height * fScaleFactor);

            // major
            const float tickMajor = h - (h * 0.74f - 2.4f * fScaleFactor);
            strokeColor(R::Tick::colorMaj);
            beginPath();
            moveTo(ts, tickMajor);
            lineTo(te, tickMajor);
            stroke();

            // minor
            const float ticks[] = {
                h - (h * 1.00f - 5.0f * fScaleFactor),
                h - (h * 0.87f - 3.7f * fScaleFactor),
                h - (h * 0.48f + 0.2f * fScaleFactor),
                h - (h * 0.24f + 2.6f * fScaleFactor),
                h - (h * 0.14f + 3.6f * fScaleFactor),
                h - (h * 0.06f + 4.3f * fScaleFactor),
            };

            strokeColor(R::Tick::color);

            for (float tick : ticks)
            {
                beginPath();
                moveTo(ts, tick);
                lineTo(te, tick);
                stroke();
            }
        }

        // ------------------------------------------------------------------------------------------------------------
        // draw labels

    }
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
