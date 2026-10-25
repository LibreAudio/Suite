// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../reference.hpp"
#include "../_lab/container.hpp"

#include "LibreAudioParameters.hpp"

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------
// pair a Widget with a parameter index

template <FaustParameterIndex index, class W>
struct ParameterWidgetPair {
    static constexpr const FaustParameterIndex kIndex = index;
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
// transparent container widget class

template<Orientation orientation>
using TransparentContainerWidget = ReferenceContainerWidget<Reference::Zero, orientation>;

using TransparentHorizontalContainerWidget = TransparentContainerWidget<Horizontal>;
using TransparentVerticalContainerWidget = TransparentContainerWidget<Vertical>;

// --------------------------------------------------------------------------------------------------------------------
// transparent container widget class with a known padding

template<Orientation orientation, uint padding>
using TransparentPaddedContainerWidget = ReferenceContainerWidget<Reference::Padding<padding>, orientation>;

// --------------------------------------------------------------------------------------------------------------------
// transparent container widget class with a known size (width or height depending on orientation)

// NOTE horizontal containers get fixed height, vertical containers get fixed width
template<Orientation orientation, uint size>
using TransparentSizedContainerReference = std::conditional_t<orientation == Horizontal,
                                                              Reference::Height<size>,
                                                              Reference::Width<size>>;

template<Orientation orientation, uint size>
using TransparentSizedContainerWidget = ReferenceContainerWidget<TransparentSizedContainerReference<orientation, size>,
                                                                 orientation>;

// --------------------------------------------------------------------------------------------------------------------
// extend the reference container to provide `addParameterWidgetPair`

template<class R, Orientation orientation>
class ParameterReferenceContainerWidget : public ReferenceContainerWidget<R, orientation>
{
    using BaseWidget = ReferenceContainerWidget<R, orientation>;
    using Layout = typename BaseWidget::Layout;

public:
    explicit ParameterReferenceContainerWidget(LabWidget* const parent)
        : BaseWidget(parent) {}

protected:
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
                          PWP::Index,
                          widget->getName());
        }
        return widget;
    }
};

// --------------------------------------------------------------------------------------------------------------------
// utility class to allow inline definitions of container widgets

template<class R, Orientation orientation, SizeHint sizeHint, typename... Args>
class StaticContainerWidget : public ReferenceContainerWidget<R, orientation>
{
    using BaseWidget = ReferenceContainerWidget<R, orientation>;

    const std::array<std::shared_ptr<LabWidget>, sizeof...(Args)> widgets {
        BaseWidget::template addWidget<Args, sizeHint>()...
    };

public:
    explicit StaticContainerWidget(LabWidget* const parent)
        : BaseWidget(parent) {}
};

template<class R, Orientation orientation, typename... Args>
class StaticContainerPairWidget : public ReferenceContainerWidget<R, orientation>
{
    using BaseWidget = ReferenceContainerWidget<R, orientation>;

    const std::array<std::shared_ptr<LabWidget>, sizeof...(Args)> widgets {
        this->template addWidget<typename Args::Widget, Args::kSizeHint>()...
    };

public:
    explicit StaticContainerPairWidget(LabWidget* const parent)
        : BaseWidget(parent) {}
};

// --------------------------------------------------------------------------------------------------------------------
// utility class to allow inline definitions of parameter container widgets

template<class R, Orientation orientation, SizeHint sizeHint, typename... Args>
class StaticParameterContainerWidget : public ParameterReferenceContainerWidget<R, orientation>
{
    using BaseWidget = ParameterReferenceContainerWidget<R, orientation>;

    const std::array<std::shared_ptr<LabWidget>, sizeof...(Args)> widgets {
        this->template addParameterWidgetPair<Args, sizeHint>()...
    };

public:
    explicit StaticParameterContainerWidget(LabWidget* const parent)
        : BaseWidget(parent) {}
};

template<class R, Orientation orientation, typename... Args>
class StaticParameterContainerPairWidget : public ParameterReferenceContainerWidget<R, orientation>
{
    using BaseWidget = ParameterReferenceContainerWidget<R, orientation>;

    const std::array<std::shared_ptr<LabWidget>, sizeof...(Args)> widgets {
        this->template addParameterWidgetPair<typename Args::Widget, Args::kSizeHint>()...
    };

public:
    explicit StaticParameterContainerPairWidget(LabWidget* const parent)
        : BaseWidget(parent) {}
};

// --------------------------------------------------------------------------------------------------------------------

template<class R, typename... Args>
class StaticBottomAlignedContainerWidget : public ReferenceContainerWidget<R, Vertical>
{
    using BaseWidget = ReferenceContainerWidget<R, Vertical>;

    const std::array<std::shared_ptr<LabWidget>, 1 + sizeof...(Args)> widgets {
        BaseWidget::addSpacer(),
        this->template addWidget<Args, Fixed>()...
    };

public:
    explicit StaticBottomAlignedContainerWidget(LabWidget* const parent)
        : BaseWidget(parent) {}
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
