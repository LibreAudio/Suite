// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "base.hpp"

#include "../containers/card-group.hpp"
#include "../containers/knob-group.hpp"

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

class EasyPageWidget final : public ReferencePageWidget<Reference::CentralArea, Vertical>
{
    using R = Reference::CentralArea;
    using BaseWidget = ReferencePageWidget<R, Vertical>;
    using BaseWidget::BaseWidget;

    const std::array<std::shared_ptr<LabWidget>, 4> fWidgets = {
        addSpacer(),
        addWidget<EasyKnobsGroupWidget>(),
        addSpacer(),
        addCards(),
    };

    std::shared_ptr<LabWidget> addCards()
    {
        std::shared_ptr<CardGroupWidget> widget { new CardGroupWidget(this) };

        widget->addCard(kWidgetPresetA, "NATURAL DOUBLE", "Subtle ADT thickening");
        widget->addCard(kWidgetPresetB, "TIGHT SLAP", "Short doubled slap");
        widget->addCard(kWidgetPresetC, "WIDE TRIPLE", "Three-voice spread");
        widget->addCard(kWidgetPresetD, "LUSH STACK", "Full ensemble stack");

        Layout::widgets.push_back({ widget.get(), Fixed });
        return widget;
    }
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
