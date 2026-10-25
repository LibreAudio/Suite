// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../reference.hpp"
#include "../_lab/container.hpp"

namespace LibreAudio {

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

// --------------------------------------------------------------------------------------------------------------------

template<class R, Orientation orientation, SizeHint sizeHint, typename... Args>
class StaticParameterContainerWidget : public ReferenceContainerWidget<R, orientation>
{
    using BaseWidget = ReferenceContainerWidget<R, orientation>;
    using Layout = typename BaseWidget::Layout;

    const std::array<std::shared_ptr<LabWidget>, sizeof...(Args)> widgets {
        addParameterWidget<Args>()...
    };

    template<class PW, typename = std::enable_if_t<std::is_base_of_v<LabWidget, typename PW::Widget>>>
    std::shared_ptr<typename PW::Widget> addParameterWidget()
    {
        using W = typename PW::Widget;
        std::shared_ptr<W> widget { new W(this, PW::Index) };
        Layout::widgets.push_back({ widget.get(), sizeHint });
        if constexpr (sizeHint == Fixed)
        {
            if (widget->getSize().isNull())
                d_stderr2("Error: addParameterWidget called with Fixed sizeHint but widget %u: '%s' does not have a known size",
                          PW::Index,
                          widget->getName());
        }
        return widget;
    }

public:
    explicit StaticParameterContainerWidget(LabWidget* const parent)
        : BaseWidget(parent) {}
};

// --------------------------------------------------------------------------------------------------------------------

template<class R, typename... Args>
class StaticBottomAlignedContainerWidget : public ReferenceContainerWidget<R, Vertical>
{
    using BaseWidget = ReferenceContainerWidget<R, Vertical>;

    const std::array<std::shared_ptr<LabWidget>, 1 + sizeof...(Args)> widgets {
        BaseWidget::addSpacer(),
        BaseWidget::template addWidget<Args, Fixed>()...
    };

public:
    explicit StaticBottomAlignedContainerWidget(LabWidget* const parent)
        : BaseWidget(parent) {}
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
