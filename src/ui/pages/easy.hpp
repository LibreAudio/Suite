// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "base.hpp"

#include "../containers/knob-group.hpp"
#include "../containers/preset-cards.hpp"

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
        addWidget<PresetCardsWidget>(),
    };
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
