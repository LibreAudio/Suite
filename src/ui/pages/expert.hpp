// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "base.hpp"

#include "../containers/knob-group.hpp"
#include "../containers/pill-toggles-area.hpp"

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

class ExpertPageWidget final : public ReferencePageWidget<Reference::CentralArea, Vertical>
{
    using R = Reference::CentralArea;
    using BaseWidget = ReferencePageWidget<R, Vertical>;
    using BaseWidget::BaseWidget;

    const std::vector<std::shared_ptr<LabWidget>> fWidgets = {
        addWidget<PillAreaWidget<>>(),
        addSpacer(),
        addWidget<ExpertKnobsGroupWidget>(),
    };
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
