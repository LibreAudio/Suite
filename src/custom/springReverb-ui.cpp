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
static constexpr const char kTextDYNAMICS[] = "DYNAMICS";
static constexpr const char kTextMIXER[] = "MIXER";

// --------------------------------------------------------------------------------------------------------------------

using ReverbSpringsGroupWidget = StaticContainerWidget<
    Reference::Widgets::BoxFrame<1>,
    Horizontal,
    Expanding,
    NumberBoxWidget<kFaustParameterDwell>,
    NumberBoxWidget<kFaustParameterTension>,
    NumberBoxWidget<kFaustParameterCenter>,
    NumberBoxWidget<kFaustParameterCrosslr>
>;

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
        NumberBoxWidget<kFaustParameterPredelay>,
        NumberBoxWidget<kFaustParameterTone>
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
        NumberBoxWithMeterWidget<kFaustParameterDuck_amount, kFaustParameterDuck_meter>,
        NumberBoxWithMeterWidget<kFaustParameterDeess_amount, kFaustParameterDeess_meter>
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
        NumberBoxWidget<kFaustParameterStereo_width>,
        NumberBoxWidget<kFaustParameterMix>
    >
>;

// --------------------------------------------------------------------------------------------------------------------

using ReverbBottomGroupWidget = StaticContainerWidget<
    Reference::Transparent<Reference::Margin<0, Reference::Widgets::BoxFrame<1, true>>>,
    Horizontal,
    Expanding,
    ReverbInputGroupWidget,
    ReverbDynamicsGroupWidget,
    ReverbMixerGroupWidget
>;

// --------------------------------------------------------------------------------------------------------------------

class ReverbExpertPageWidget : public LabReferenceContainerWidget<Reference::CentralArea, Vertical>
{
    using BaseWidget = LabReferenceContainerWidget<Reference::CentralArea, Vertical>;
    using BaseWidget::BaseWidget;

    const std::array<std::shared_ptr<LabWidget>, 3> widgets {
        this->template addWidget<ReverbSpringsGroupWidget, Fixed>(),
        BaseWidget::addSpacer(),
        this->template addWidget<ReverbBottomGroupWidget, Fixed>()
    };
};

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
