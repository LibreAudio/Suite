// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../base/color.hpp"
#include "knob-group.hpp"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioStageWidget final : public LibreAudioContainer<LibreAudioReference::Stage, kVertical>
{
    using R = LibreAudioReference::Stage;

    static constexpr const float kTopAreaColor[] = { 1.f, 0.f, 0.f, 0.5f };
    std::unique_ptr<LibreAudioWidget> fTopArea = createWidget<LibreAudioColorWidget<kTopAreaColor>>();
    std::unique_ptr<LibreAudioWidget> fSpacer = createSpacer();
    std::unique_ptr<LibreAudioExpertKnobsGroupWidget> fExpertKnobs = createWidget<LibreAudioExpertKnobsGroupWidget>();

public:
    LibreAudioStageWidget(LibreAudioWidget* const parent)
        : LibreAudioContainer(parent)
    {
        fTopArea->setHeight(30 * fScaleFactor);
    }

private:
    void onNanoDisplay() final
    {
        const float w = getWidth();
        const float h = getHeight();
        d_stdout("stage size %f %f", w, h);

        // ------------------------------------------------------------------------------------------------------------
        // draw background and border

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
    }
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
