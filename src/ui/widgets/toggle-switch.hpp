// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../_lab/toggle-switch.hpp"

#include "../reference.hpp"

#include "LibreAudioParameters.hpp"

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

template<FaustParameterIndex kParameterIndex>
class ToggleSwitchBaseWidget : public LabToggleSwitchBaseWidget
{
    using BaseWidget = LabToggleSwitchBaseWidget;

public:
    static constexpr const FaustParameter& kParameter = kFaustParameters[kParameterIndex];
    static_assert(kParameter.isBoolean, "check-box parameter must be boolean");

    explicit ToggleSwitchBaseWidget(LabWidget* const parent)
        : BaseWidget(parent, kParametersMainStart + kParameterIndex)
    {
        setChecked(d_isNotEqual(kParameter.init, kParameter.min), false);
        setName(kParameter.name);
    }

protected:
    void buttonClicked(SubWidget*, int) override
    {
        fInterface->parameterControlModified(getId(), isChecked() ? kParameter.max : kParameter.min);
    }

    void idleCallback() override
    {
        if (setChecked(d_isNotEqual(fInterface->getParameterValue(getId()), kParameter.min), false))
            parameterChangedByHost();
    }
};

// --------------------------------------------------------------------------------------------------------------------

template <uint8_t accent, bool small = false>
class ToggleSwitchWidget : public LabReferenceToggleSwitchWidget<Reference::Widgets::ToggleSwitch<accent, small>>
{
    using BaseWidget = LabReferenceToggleSwitchWidget<Reference::Widgets::ToggleSwitch<accent, small>>;

public:
    explicit ToggleSwitchWidget(LabWidget* const parent, const uint id, const char* const name)
        : BaseWidget(parent, id, name) {}

    explicit ToggleSwitchWidget(LabWidget* const parent, const uint id, const FaustParameter& parameter)
        : BaseWidget(parent, id, parameter.name)
    {
        BaseWidget::setChecked(d_isNotZero(parameter.init), false);
    }

    explicit ToggleSwitchWidget(LabWidget* const parent, const FaustParameterIndex index)
        : BaseWidget(parent, kParametersMainStart + index, kFaustParameters[index].name)
    {
        BaseWidget::setChecked(d_isNotZero(kFaustParameters[index].init), false);
    }
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
