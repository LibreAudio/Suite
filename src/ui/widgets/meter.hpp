// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../base/widgets.hpp"
#include "../reference.hpp"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioMeter final : public LibreAudioWidget
{
    using R = LibreAudioReference::Meter;

public:
    LibreAudioMeter(LibreAudioWidget* const parent)
        : LibreAudioWidget(parent)
    {
        if constexpr (R::width != 0)
            LibreAudioWidget::setWidth(d_roundToUnsignedInt(R::width * fScaleFactor));

        if constexpr (R::height != 0)
            LibreAudioWidget::setHeight(d_roundToUnsignedInt(R::height * fScaleFactor));
    }

private:
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

            beginPath();
            rect(ts, ts, tw, h - ts);
            fill();

            beginPath();
            rect(tc + 0.5f, ts, tw, h - ts);
            fill();

            // resetScissor();
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
