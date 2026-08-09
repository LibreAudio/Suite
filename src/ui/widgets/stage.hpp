// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../base/color.hpp"
#include "knob-group.hpp"
#include "pill-toggle.hpp"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioStageWidget final : public LibreAudioContainerSubWidget<LibreAudioReference::Stage, kVertical>
{
    using R = LibreAudioReference::Stage;

    std::unique_ptr<LibreAudioPillAreaWidget> fTopArea = addWidget<LibreAudioPillAreaWidget>();
    std::unique_ptr<LibreAudioWidget> fSpacer = addSpacer();
    std::unique_ptr<LibreAudioExpertKnobsGroupWidget> fExpertKnobs = addWidget<LibreAudioExpertKnobsGroupWidget>();

public:
    LibreAudioStageWidget(LibreAudioWidget* const parent)
        : LibreAudioContainerSubWidget(parent)
    {
        fTopArea->setHeight(30 * fScaleFactor);
    }

private:
    void onNanoDisplay() final
    {
        const float w = getWidth();
        const float h = getHeight();

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
