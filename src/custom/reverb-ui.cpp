// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/containers/main-area.hpp"
#include "ui/containers/stage.hpp"
#include "ui/containers/top-bar.hpp"
#include "ui/containers/ui.hpp"

#include "LibreAudioParameters.hpp"

// --------------------------------------------------------------------------------------------------------------------

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

struct ReverbEasyExpertPageReference : Reference::Zero {
    static constexpr const uint margin = 12;
    static constexpr const uint padding = 8;
};

class TestButton : public TextButtonWidget<Reference::Widgets::TopBarButton, kCornerNone, Vertical>
{
    using BaseWidget = TextButtonWidget<Reference::Widgets::TopBarButton, kCornerNone, Vertical>;

public:
    TestButton(LabWidget* const parent)
        : BaseWidget(parent, 0, "Test button, please ignore") {}
};

using ReverbExpertPageWidget = StaticBottomAlignedContainerWidget<
    ReverbEasyExpertPageReference,
    TestButton
>;

// --------------------------------------------------------------------------------------------------------------------

class ReverbStageWidget final : public StageWidget<EasyStageWidget, ReverbExpertPageWidget>
{
    using BaseWidget = StageWidget<EasyStageWidget, ReverbExpertPageWidget>;
    using BaseWidget::BaseWidget;

public:
    Size<uint> getGraphSize() const noexcept final
    {
        const LabMetrics<R> m { this };

        const float space = m.margin + m.border + m.padding + (
            fLastPage == kPageEasy ? getEasyWidget()->getLastWidgetHeight()
                                   : getExpertWidget()->getLastWidgetHeight()
        );

        return Size<uint>(getWidth() - m.border * 2, getHeight() - space);
    }
};

// --------------------------------------------------------------------------------------------------------------------

using ReverbMainArea = MainAreaContainerWidget<ReverbStageWidget>;
using ReverbRootWidget = RootWidget<TopBar, ReverbMainArea>;

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

UI* createUI()
{
    return new LibreAudio::UI<LibreAudio::ReverbRootWidget>();
}

END_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------
