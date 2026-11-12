// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../base.hpp"
#include "../reference.hpp"
#include "../_lab/container.hpp"

#include "LibreAudioParameters.hpp"

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------
// pair a Widget with a parameter or widget index

// TODO remove this
template <FaustParameterIndex index, class W>
struct ParameterWidgetPair {
    static constexpr const FaustParameterIndex kIndex = index;
    using Widget = W;
};

// TODO remove this
template <WidgetIds id, class W>
struct WidgetPair {
    static constexpr const WidgetIds kId = id;
    using Widget = W;
};

// --------------------------------------------------------------------------------------------------------------------
// pair a Widget with a sizeHint

template <SizeHint sizeHint, class W>
struct SizeHintWidgetPair {
    static constexpr const SizeHint kSizeHint = sizeHint;
    using Widget = W;
};

// --------------------------------------------------------------------------------------------------------------------
// extend the reference container to provide `addParameterWidgetPair`

// TODO remove this
template<class R, Orientation orientation>
class ParameterReferenceContainerWidget : public LabReferenceContainerWidget<R, orientation>
{
    using BaseWidget = LabReferenceContainerWidget<R, orientation>;
    using BaseWidget::BaseWidget;

protected:
    using Layout = typename BaseWidget::Layout;

    template<class PWP,
             SizeHint sizeHint,
             typename = std::enable_if_t<std::is_base_of_v<LabWidget, typename PWP::Widget>>>
    std::shared_ptr<typename PWP::Widget> addParameterWidgetPair()
    {
        using W = typename PWP::Widget;
        std::shared_ptr<W> widget { new W(this, PWP::kIndex) };
        Layout::widgets.push_back({ widget.get(), sizeHint });
        if constexpr (sizeHint == Fixed)
        {
            if (widget->getSize().isNull())
                d_stderr2("Error: addParameterWidget called with Fixed sizeHint but widget %u: '%s' does not have a known size",
                          PWP::kIndex,
                          widget->getName());
        }
        return widget;
    }
};

// --------------------------------------------------------------------------------------------------------------------
// utility class to allow inline definitions of container widgets

template<class R, Orientation orientation, SizeHint sizeHint, typename... Args>
class StaticContainerWidget : public LabReferenceContainerWidget<R, orientation>
{
    using BaseWidget = LabReferenceContainerWidget<R, orientation>;
    using BaseWidget::BaseWidget;

    const std::array<std::shared_ptr<LabWidget>, sizeof...(Args)> widgets {
        this->template addWidget<Args, sizeHint>()...
    };

protected:
    using R2 = R;

    static constexpr const uint kNumWidgets = sizeof...(Args);
};

template<class R, Orientation orientation, SizeHint sizeHint, typename... Args>
class StaticAutoSizedContainerWidget : public StaticContainerWidget<R, orientation, sizeHint, Args...>
{
    using BaseWidget = StaticContainerWidget<R, orientation, sizeHint, Args...>;
    using BaseWidget::BaseWidget;

    void updateSize(const bool updateChildren) override
    {
        if (updateChildren)
            BaseWidget::autoSize();
        else
            BaseWidget::updateSize(false);
    }
};

template<class R, Orientation orientation, typename... Args>
class StaticContainerSizeHintPairWidget : public LabReferenceContainerWidget<R, orientation>
{
    using BaseWidget = LabReferenceContainerWidget<R, orientation>;
    using BaseWidget::BaseWidget;

    const std::array<std::shared_ptr<LabWidget>, sizeof...(Args)> widgets {
        this->template addWidget<typename Args::Widget, Args::kSizeHint>()...
    };
};

// --------------------------------------------------------------------------------------------------------------------
// utility class to allow inline definitions of parameter container widgets

template<class R, Orientation orientation, SizeHint sizeHint, typename... Args>
class StaticParameterContainerWidget : public ParameterReferenceContainerWidget<R, orientation>
{
    using BaseWidget = ParameterReferenceContainerWidget<R, orientation>;
    using BaseWidget::BaseWidget;
    using Layout = typename BaseWidget::Layout;

    const std::array<std::shared_ptr<LabWidget>, sizeof...(Args)> widgets {
        this->template addParameterWidgetPair<Args, sizeHint>()...
    };

protected:
    void updateSize(const bool updateChildren) override
    {
        if (updateChildren)
            BaseWidget::autoSize();
        else
            BaseWidget::updateSize(false);
    }
};

template<class R, Orientation orientation, typename... Args>
class StaticParameterContainerPairWidget : public ParameterReferenceContainerWidget<R, orientation>
{
    using BaseWidget = ParameterReferenceContainerWidget<R, orientation>;
    using BaseWidget::BaseWidget;

    const std::array<std::shared_ptr<LabWidget>, sizeof...(Args)> widgets {
        this->template addParameterWidgetPair<typename Args::Widget, Args::kSizeHint>()...
    };
};

// --------------------------------------------------------------------------------------------------------------------

template<class R, typename... Args>
class StaticBottomAlignedContainerWidget : public LabReferenceContainerWidget<R, Vertical>
{
    using BaseWidget = LabReferenceContainerWidget<R, Vertical>;
    using BaseWidget::BaseWidget;

    const std::array<std::shared_ptr<LabWidget>, 1 + sizeof...(Args)> widgets {
        BaseWidget::addSpacer(),
        this->template addWidget<Args, Fixed>()...
    };
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
