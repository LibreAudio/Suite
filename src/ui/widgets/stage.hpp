// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../base/color.hpp"
#include "knob-group.hpp"
#include "line.hpp"
#include "pill-toggle.hpp"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioStageWidget final : public LibreAudioContainerSubWidget<LibreAudioReference::Stage, kVertical>
{
    using R = LibreAudioReference::Stage;

    // std::unique_ptr<LibreAudioLineWidget> fLine { new LibreAudioLineWidget(this) };
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

        fillPaint(linearGradient(0, h - h * 0.4f, 0, h - h * 0.2f, LibreAudioReference::Colors::transparent, Color(0.f, 0.f, 0.f, 0.75f)));
        fill();

        if constexpr (R::border != 0 && d_isNotZero(R::borderColor.alpha))
        {
            strokeColor(R::borderColor);
            strokeWidth(R::border * 2 * fScaleFactor);
            stroke();
        }
    }

    void onPositionChanged(const PositionChangedEvent& ev) override
    {
        LibreAudioContainerSubWidget::onPositionChanged(ev);

        // const uint border = d_roundToUnsignedInt(R::border * fScaleFactor);
        // fLine->setAbsolutePos(ev.pos.getX() + border, ev.pos.getY() + border);
    }

    void onResize(const ResizeEvent& ev) final
    {
        LibreAudioContainerSubWidget::onResize(ev);

        // const uint border = d_roundToUnsignedInt(R::border * fScaleFactor);
        // fLine->setSize(ev.size.getWidth() - border, ev.size.getHeight() - border);
    }
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
