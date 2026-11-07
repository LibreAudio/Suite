// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "container.hpp"
#include "ui/widgets/number-box.hpp"

#include "LibreAudioParameters.hpp"

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

template<Orientation kOrientation>
class BoxFrameWidget : public LabReferenceContainerWidget<Reference::Widgets::BoxFrame, kOrientation>
{
    using R = Reference::Widgets::BoxFrame;
    using BaseWidget = LabReferenceContainerWidget<R, kOrientation>;
    using BaseWidget::BaseWidget;
};

template<Orientation kOrientation>
class TransparentFrameWidget : public LabReferenceContainerWidget<Reference::Transparent<Reference::Widgets::BoxFrame>,
                                                                  kOrientation>
{
    using R = Reference::Widgets::BoxFrame;
    using BaseWidget = LabReferenceContainerWidget<R, kOrientation>;
    using BaseWidget::BaseWidget;
};

// --------------------------------------------------------------------------------------------------------------------
// utility class to allow inline definitions of parameter container widgets

// template<uint kSize, Orientation kOrientation, SizeHint kSizeHint, typename... Args>
// class StaticParameterBoxFrameWidget : public ParameterReferenceContainerWidget<
//     Reference::AutoSize<kOrientation,
//                         kSize + (Reference::Widgets::BoxFrame::border + Reference::Widgets::BoxFrame::margin) * 2,
//                         Reference::Widgets::BoxFrame>,
//     kOrientation
// >
// {
// public:
//     using R = Reference::AutoSize<
//         kOrientation,
//         kSize + (Reference::Widgets::BoxFrame::border + Reference::Widgets::BoxFrame::margin) * 2,
//         Reference::Widgets::BoxFrame
//     >;
//
// private:
//     using BaseWidget = ParameterReferenceContainerWidget<R, kOrientation>;
//     using BaseWidget::BaseWidget;
//
//     const std::array<std::shared_ptr<LabWidget>, sizeof...(Args)> widgets {
//         this->template addParameterWidgetPair<Args, kSizeHint>()...
//     };
// };

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
