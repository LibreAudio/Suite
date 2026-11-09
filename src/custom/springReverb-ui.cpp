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
    StaticNumberBoxWidget<kFaustParameterDwell, Reference::Colors::red>,
    StaticNumberBoxWidget<kFaustParameterTension, Reference::Colors::red>,
    StaticNumberBoxWidget<kFaustParameterCenter, Reference::Colors::red>,
    StaticNumberBoxWidget<kFaustParameterCrosslr, Reference::Colors::red>
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
        StaticNumberBoxWidget<kFaustParameterPredelay, Reference::Colors::blue>,
        StaticNumberBoxWidget<kFaustParameterTone, Reference::Colors::yellow>
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
