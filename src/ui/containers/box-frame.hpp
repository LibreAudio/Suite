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

template<const Color& color, const char text[]>
using BoxFrameTitle = LabReferenceStaticTextWidget<Reference::Widgets::BoxFrame<>::Header::Title<color>, text>;

template<Orientation kOrientation, uint kNumRows = 1, bool kIncludeHeader = false>
class BoxFrameWidget : public LabReferenceContainerWidget<Reference::Widgets::BoxFrame<kNumRows, kIncludeHeader>,
                                                          kIncludeHeader ? Vertical : kOrientation>
{
    using R = Reference::Widgets::BoxFrame<kNumRows, kIncludeHeader>;
    using BaseWidget = LabReferenceContainerWidget<R, kIncludeHeader ? Vertical : kOrientation>;
    using BaseWidget::BaseWidget;

    static constexpr const bool kSeparateBodyLayout = kIncludeHeader && kOrientation == Horizontal;

    // Provide a class that does nothing, so we can have a conditional type
    struct Unused {};

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

} /* namespace LibreAudio */
