// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ui/containers/knob-group.hpp"
#include "ui/widgets/button.hpp"
#include "ui/widgets/dual-slider.hpp"
#include "ui/widgets/pill-toggle.hpp"

#include LIBREAUDIO_PLUGIN_PARAMETERS_INCLUDE

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

template<class R>
class FrameContainerReferenceWidget : public ReferenceContainerWidget<R, kVertical>
{
    using BaseWidget = ReferenceContainerWidget<R, kVertical>;
    using Layout = typename BaseWidget::Layout;

    std::list<std::shared_ptr<LabWidget>> fWidgets;

    struct TextReference : Reference::Zero {
        static constexpr const Color color = Reference::Colors::ink2;
        static constexpr const float fontSize = 12;
        static constexpr const float letterSpacing = fontSize * 0.01;
        static constexpr const uint margin = 0;
    };

public:
    explicit FrameContainerReferenceWidget(LabWidget* const parent)
        : BaseWidget(parent) {}

    template <FaustParameterIndex parameterA, FaustParameterIndex parameterB>
    void addDualSlider()
    {
        std::shared_ptr<LabWidget> widget { new DualSliderWidget<parameterA, parameterB>(this) };
        Layout::widgets.push_back({ widget.get(), Fixed });
        fWidgets.emplace_back(std::move(widget));
    }

    void addPillToggle(const FaustParameterIndex parameter)
    {
        std::shared_ptr<LabWidget> widget { new PillAreaWidget<1>(this, parameter) };
        Layout::widgets.push_back({ widget.get(), Fixed });
        fWidgets.emplace_back(std::move(widget));
    }

    template<class W, uint maxNumParameters>
    std::shared_ptr<KnobGroupWidget<W, maxNumParameters>> addKnobGroup(const FaustParameterIndex parameterStart)
    {
        std::shared_ptr<KnobGroupWidget<W, maxNumParameters>> widget {
            new KnobGroupWidget<W, maxNumParameters>(this, kParametersMainStart, parameterStart, maxNumParameters <= 2)
        };
        Layout::widgets.push_back({ widget.get(), Fixed });
        fWidgets.push_back(widget);
        return widget;
    }

    void addSpacer()
    {
        std::shared_ptr<LabWidget> spacer { new LabEmptyWidget(this) };
        Layout::widgets.push_back({ spacer.get(), Expanding });
        fWidgets.emplace_back(std::move(spacer));
    }

    void addText(const char* const text)
    {
        std::shared_ptr<LabWidget> spacer { new TextButtonWidget<kCornerNone, TextReference, kVertical>(this, text) };
        Layout::widgets.push_back({ spacer.get(), Fixed });
        fWidgets.emplace_back(std::move(spacer));
    }

    std::shared_ptr<LabWidget> getWidgetById(const uint32_t parameter) const noexcept
    {
        for (const std::shared_ptr<LabWidget>& widget : fWidgets)
            if (widget->getId() == parameter)
                return widget;

        return {};
    }
};

// --------------------------------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
