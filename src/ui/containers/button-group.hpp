// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../_lab/button-group.hpp"

#include "../base.hpp"
#include "../reference.hpp"

#include "LibreAudioParameters.hpp"

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

template <class R>
class ButtonGroupWidget : public LabReferenceButtonGroupWidget<R>
{
    using BaseWidget = LabReferenceButtonGroupWidget<R>;
    using BaseWidget::BaseWidget;

public:
    template<class B,
             typename... Args,
             typename = std::enable_if_t<std::is_base_of_v<LabButtonBaseWidget, B>>>
    std::shared_ptr<LabButtonBaseWidget> addButton(const WidgetIds id, Args... args)
    {
        return BaseWidget::template addButton<B>(id, WidgetIds2Str(id), args...);
    }

    template<class B,
             typename... Args,
             typename = std::enable_if_t<std::is_base_of_v<LabButtonBaseWidget, B>>>
    std::shared_ptr<LabButtonBaseWidget> addButton(const FaustParameterIndex index, Args... args)
    {
        const FaustParameter& parameter = kFaustParameters[index];
        DISTRHO_SAFE_ASSERT_RETURN(d_isEqual(parameter.min, 0.f), nullptr);
        DISTRHO_SAFE_ASSERT_RETURN(d_isEqual(parameter.max, 1.f), nullptr);

        return BaseWidget::template addButton<B>(kParametersMainStart + index,
                                                 getFaustParameterLabel(parameter),
                                                 args...);
    }

    template<class BP,
             typename = std::enable_if_t<std::is_base_of_v<LabButtonBaseWidget, typename BP::Widget>>>
    std::shared_ptr<LabButtonBaseWidget> addParameterButton()
    {
        return addButton<typename BP::Widget>(BP::kIndex);
    }

    template<class BP,
             typename = std::enable_if_t<std::is_base_of_v<LabButtonBaseWidget, typename BP::Widget>>>
    std::shared_ptr<LabButtonBaseWidget> addWidgetButton()
    {
        return addButton<typename BP::Widget>(BP::kId);
    }
};

// --------------------------------------------------------------------------------------------------------------------

template <class R, typename... Args>
class StaticButtonGroupWidget : public ButtonGroupWidget<R>
{
    using BaseWidget = ButtonGroupWidget<R>;
    using BaseWidget::BaseWidget;

    const std::array<std::shared_ptr<LabButtonBaseWidget>, sizeof...(Args)> _ {
        this->template addWidgetButton<Args>()...
    };
};

// --------------------------------------------------------------------------------------------------------------------

template <class R, typename... Args>
class StaticParameterButtonGroupWidget : public ButtonGroupWidget<R>
{
    using BaseWidget = ButtonGroupWidget<R>;
    using BaseWidget::BaseWidget;

    const std::array<std::shared_ptr<LabButtonBaseWidget>, sizeof...(Args)> _ {
        this->template addParameterButton<Args>()...
    };
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */

