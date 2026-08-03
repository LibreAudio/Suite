// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../base/knob.hpp"
#include "../reference.hpp"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioSmallKnobWidget final : public LibreAudioKnobWidget
{
    using R = LibreAudioReference::Widgets::Knob;

public:
    LibreAudioSmallKnobWidget(LibreAudioWidget* const parent, const FaustParameter& parameter, const uint32_t id)
        : LibreAudioKnobWidget(parent, parameter, id)
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
        // draw text, TESTING

        fillColor(R::foregroundColor);
        fontSize(R::fontSize * fScaleFactor);
        textAlign(ALIGN_CENTER | ALIGN_MIDDLE);

        char text[64];
        std::snprintf(text,
                      sizeof(text),
                      "%.10s\n%.1f%s%s",
                      fParameter.label,
                      getValue(),
                      *fParameter.unit != '\0' ? " " : "",
                      fParameter.unit);

        textBox(0, h * 0.33f, w, text);
    }
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
