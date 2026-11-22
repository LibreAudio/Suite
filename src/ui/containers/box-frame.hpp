// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "container.hpp"
#include "ui/widgets/number-box.hpp"
#include "ui/_lab/text.hpp"

#include "LibreAudioParameters.hpp"

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

template<Orientation kOrientation, uint kNumRows = 1, bool kIncludeHeader = false>
class BoxFrameWidget : public LabReferenceContainerWidget<Reference::Widgets::BoxFrame<kNumRows, kIncludeHeader>,
                                                          kIncludeHeader ? Vertical : kOrientation>
{
    using R = Reference::Widgets::BoxFrame<kNumRows, kIncludeHeader>;
    using BaseWidget = LabReferenceContainerWidget<R, kIncludeHeader ? Vertical : kOrientation>;
    using BaseWidget::BaseWidget;

    static constexpr const bool kSeparateBodyLayout = kIncludeHeader && kOrientation == Horizontal;

    // Optional header widget
    std::conditional_t<kIncludeHeader, std::shared_ptr<LabWidget>, Unused> fHeader;

    // Optional body widget
    using BodyWidgetType = LabReferenceContainerWidget<typename R::Row, Horizontal>;
    std::conditional_t<kSeparateBodyLayout, std::shared_ptr<BodyWidgetType>, Unused> fBody;

protected:
    template<class W>
    std::enable_if_t<kIncludeHeader, std::shared_ptr<LabWidget>> addHeader()
    {
        fHeader = BaseWidget::template addWidget<W, Fixed>();

        if constexpr (kSeparateBodyLayout)
            fBody = BaseWidget::template addWidget<BodyWidgetType, Expanding>();

        return fHeader;
    }

protected:
    template<class W = LabEmptyWidget,
             SizeHint sizeHint = Expanding,
             typename = std::enable_if_t<std::is_base_of_v<LabWidget, W>>>
    std::shared_ptr<W> addSpacer()
    {
        if constexpr (kSeparateBodyLayout)
            return fBody->template addSpacer<W, sizeHint>();

        return BaseWidget::template addSpacer<W, sizeHint>();
    }

    template<class W,
             SizeHint sizeHint = Fixed,
             typename... Args,
             typename = std::enable_if_t<std::is_base_of_v<LabWidget, W>>>
    std::shared_ptr<W> addWidget(Args... args)
    {
        if constexpr (kSeparateBodyLayout)
            return fBody->template addWidget<W, sizeHint, Args...>();

        return BaseWidget::template addWidget<W, sizeHint, Args...>();
    }
};

// --------------------------------------------------------------------------------------------------------------------

template<const Color& color, const char text[]>
using StaticBoxFrameTitleWidget = LabReferenceStaticTextWidget<
    Reference::Widgets::BoxFrame<1>::Header::Title<color>,
    text
>;

// --------------------------------------------------------------------------------------------------------------------

template<typename... Args>
using StaticHorizontalBoxFrameWidget = StaticContainerWidget<
    Reference::Widgets::BoxFrame<1>,
    Horizontal,
    Expanding,
    Args...
>;

template<typename... Args>
using StaticVerticalBoxFrameWidget = StaticContainerWidget<
    Reference::Widgets::BoxFrame<0>,
    Vertical,
    Fixed,
    Args...
>;

// --------------------------------------------------------------------------------------------------------------------

template<typename... Args>
using StaticBoxFrameRowWidget = StaticContainerWidget<
    Reference::Widgets::BoxFrame<1>::Row,
    Horizontal,
    Expanding,
    Args...
>;

// --------------------------------------------------------------------------------------------------------------------

template<const Color& color, const char text[], typename... Args>
using StaticHorizontalTitledBoxFrameWidget = StaticContainerWidget<
    Reference::Widgets::BoxFrame<1, true>,
    Vertical,
    Fixed,
    StaticBoxFrameTitleWidget<color, text>,
    StaticContainerWidget<
        Reference::Widgets::BoxFrame<1, true>::Row,
        Horizontal,
        Expanding,
        Args...
    >
>;

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
