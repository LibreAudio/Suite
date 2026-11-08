// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/containers/central-area.hpp"
#include "ui/containers/main-area.hpp"
#include "ui/containers/top-bar.hpp"
#include "ui/containers/ui.hpp"
#include "ui/containers/box-frame.hpp"
#include "ui/widgets/number-box.hpp"

#include "LibreAudioParameters.hpp"

// --------------------------------------------------------------------------------------------------------------------

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

static constexpr const char kTextINPUT[] = "INPUT";
static constexpr const char kTextSPRINGS[] = "SPRINGS";
static constexpr const char kTextDYNAMICS[] = "DYNAMICS";
static constexpr const char kTextMIXER[] = "MIXER";

// --------------------------------------------------------------------------------------------------------------------

using ReverbInputGroupWidget = StaticContainerWidget<
    Reference::Widgets::BoxFrame<1, true>,
    Vertical,
    Fixed,
    BoxFrameTitle<Reference::Colors::blue, kTextINPUT>,
    StaticContainerWidget<
        Reference::Widgets::BoxFrame<1, true>::Row,
        Horizontal,
        Expanding,
        StaticNumberBoxWidget<kFaustParameterPredelay, Reference::Colors::blue>,
        StaticNumberBoxWidget<kFaustParameterTone, Reference::Colors::yellow>
    >
>;

using ReverbSpringsGroupWidget = StaticContainerWidget<
    Reference::Widgets::BoxFrame<1, true>,
    Vertical,
    Fixed,
    BoxFrameTitle<Reference::Colors::red, kTextSPRINGS>,
    StaticContainerWidget<
        Reference::Widgets::BoxFrame<1, true>::Row,
        Horizontal,
        Expanding,
        StaticNumberBoxWidget<kFaustParameterDwell, Reference::Colors::red>,
        StaticNumberBoxWidget<kFaustParameterTension, Reference::Colors::red>,
        StaticNumberBoxWidget<kFaustParameterCenter, Reference::Colors::red>,
        StaticNumberBoxWidget<kFaustParameterCrosslr, Reference::Colors::red>
    >
>;

using ReverbDynamicsGroupWidget = StaticContainerWidget<
    Reference::Widgets::BoxFrame<1, true>,
    Vertical,
    Fixed,
    BoxFrameTitle<Reference::Colors::yellow, kTextDYNAMICS>,
    StaticContainerWidget<
        Reference::Widgets::BoxFrame<1, true>::Row,
        Horizontal,
        Expanding,
        StaticNumberBoxWithMeterWidget<kFaustParameterDuck_amount, kFaustParameterDuck_meter, Reference::Colors::yellow>,
        StaticNumberBoxWithMeterWidget<kFaustParameterDeess_amount, kFaustParameterDeess_meter, Reference::Colors::yellow>
    >
>;

using ReverbMixerGroupWidget = StaticContainerWidget<
    Reference::Widgets::BoxFrame<1, true>,
    Vertical,
    Fixed,
    BoxFrameTitle<Reference::Colors::blue, kTextMIXER>,
    StaticContainerWidget<
        Reference::Widgets::BoxFrame<1, true>::Row,
        Horizontal,
        Expanding,
        StaticNumberBoxWidget<kFaustParameterStereo_width, Reference::Colors::blue>,
        StaticNumberBoxWidget<kFaustParameterMix, Reference::Colors::yellow>
    >
>;

// --------------------------------------------------------------------------------------------------------------------
// one of the groups is 2x width of others, so we need special handling here

using ReverbBottomGroupWidgetBase = StaticContainerWidget<
    Reference::Transparent<Reference::Margin<0, Reference::Widgets::BoxFrame<1, true>>>,
    Horizontal,
    Fixed,
    ReverbInputGroupWidget,
    ReverbSpringsGroupWidget,
    ReverbDynamicsGroupWidget,
    ReverbMixerGroupWidget
>;

class ReverbBottomGroupWidget final : public ReverbBottomGroupWidgetBase
{
    using R = ReverbBottomGroupWidgetBase::R2;
    using BaseWidget = ReverbBottomGroupWidgetBase;
    using BaseWidget::BaseWidget;

    using Layout = typename ReverbBottomGroupWidgetBase::Layout;

    static_assert(kNumWidgets == 4, "wrong number of widgets");

    void updateSize(const bool updateChildren)
    {
        static constexpr const uint kWidthMultipliers[4] = { 1, 2, 1, 1 };

        const LabMetrics<R, uint> m { this };

        const uint usableWidth = m.width - (m.border + m.margin) * 2 - m.padding * 3;
        const uint equalItemWidth = usableWidth / 5;

        uint i = 0;
        for (const SubWidgetWithSizeHint& ssh : Layout::widgets)
            ssh.widget->setWidth(equalItemWidth * kWidthMultipliers[i++]);

        BaseWidget::updateSize(updateChildren);
    }
};

// --------------------------------------------------------------------------------------------------------------------

using ReverbExpertPageWidget  = StaticBottomAlignedContainerWidget<Reference::CentralArea, ReverbBottomGroupWidget>;

// --------------------------------------------------------------------------------------------------------------------

class ReverbCentralAreaWidget final : public CentralAreaWidget<EasyPageWidget, ReverbExpertPageWidget>
{
    using BaseWidget = CentralAreaWidget<EasyPageWidget, ReverbExpertPageWidget>;
    using BaseWidget::BaseWidget;

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

using ReverbMainArea = MainAreaContainerWidget<ReverbCentralAreaWidget>;
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
