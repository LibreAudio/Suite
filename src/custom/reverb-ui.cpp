// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/containers/central-area.hpp"
#include "ui/containers/main-area.hpp"
#include "ui/containers/top-bar.hpp"
#include "ui/containers/ui.hpp"
#include "ui/widgets/number-box.hpp"

#include "LibreAudioParameters.hpp"

// --------------------------------------------------------------------------------------------------------------------

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

using ReverbEarlyGroupReference = Reference::Height<
    Reference::Widgets::NumberBox<>::height + (Reference::Widgets::BoxFrame::border + Reference::Widgets::BoxFrame::margin) * 2,
    Reference::Widgets::BoxFrame
>;

using ReverbEarlyGroupWidget = StaticParameterContainerWidget<
    ReverbEarlyGroupReference,
    Horizontal,
    Expanding,
    ParameterWidgetPair<kFaustParameterEr_size, NumberBoxWidget<Reference::Colors::acc3>>,
    ParameterWidgetPair<kFaustParameterEr_diffusion, NumberBoxWidget<Reference::Colors::acc3>>,
    ParameterWidgetPair<kFaustParameterEr_damping, NumberBoxWidget<Reference::Colors::acc3>>,
    ParameterWidgetPair<kFaustParameterEr_spread, NumberBoxWidget<Reference::Colors::acc3>>,
    ParameterWidgetPair<kFaustParameterTail_feed, NumberBoxWidget<Reference::Colors::acc3>>
    // kFaustParameterEr_level
>;

// --------------------------------------------------------------------------------------------------------------------

using ReverbTailGroupReference = Reference::Height<
    Reference::Widgets::NumberBox<>::height * 2 + Reference::Widgets::BoxFrame::padding + (Reference::Widgets::BoxFrame::border + Reference::Widgets::BoxFrame::margin) * 2,
    Reference::Widgets::BoxFrame
>;

using ReverbTailSubGroupReference = Reference::Padding<
    ReverbTailGroupReference::padding,
    Reference::Height<
        Reference::Widgets::NumberBox<>::height
    >
>;

using ReverbTail1GroupWidget = StaticParameterContainerWidget<
    ReverbTailSubGroupReference,
    Horizontal,
    Expanding,
    ParameterWidgetPair<kFaustParameterDecay, NumberBoxWidget<Reference::Colors::acc4>>,
    ParameterWidgetPair<kFaustParameterTail_size, NumberBoxWidget<Reference::Colors::acc4>>,
    ParameterWidgetPair<kFaustParameterTail_shape, NumberBoxWidget<Reference::Colors::acc4>>,
    ParameterWidgetPair<kFaustParameterTail_spread, NumberBoxWidget<Reference::Colors::acc4>>,
    ParameterWidgetPair<kFaustParameterTail_level, NumberBoxWidget<Reference::Colors::acc4>>
>;

using ReverbTail2GroupWidget = StaticParameterContainerWidget<
    ReverbTailSubGroupReference,
    Horizontal,
    Expanding,
    ParameterWidgetPair<kFaustParameterBass_mult, NumberBoxWidget<Reference::Colors::acc4>>,
    ParameterWidgetPair<kFaustParameterBass_freq, NumberBoxWidget<Reference::Colors::acc4>>,
    ParameterWidgetPair<kFaustParameterHf_damp, NumberBoxWidget<Reference::Colors::acc4>>,
    ParameterWidgetPair<kFaustParameterMod_rate, NumberBoxWidget<Reference::Colors::acc4>>,
    ParameterWidgetPair<kFaustParameterMod_depth, NumberBoxWidget<Reference::Colors::acc4>>
>;

using ReverbTailGroupWidget = StaticContainerWidget<
    ReverbTailGroupReference,
    Vertical,
    Fixed,
    ReverbTail1GroupWidget,
    ReverbTail2GroupWidget
>;

// kFaustParameterDensity,
// kFaustParameterDeess_amount,
// kFaustParameterLowcut,
// kFaustParameterHighcut,
// kFaustParameterPredelay,
// kFaustParameterDim,
// kFaustParameterDuck_amount,
// kFaustParameterStereo_width,
// kFaustParameterDrywet,
// kFaustParameterEr_meter_l,
// kFaustParameterEr_meter_r,
// kFaustParameterTail_meter_l,
// kFaustParameterTail_meter_r,
// kFaustParameterDeess_meter,
// kFaustParameterDuck_meter,

// --------------------------------------------------------------------------------------------------------------------

class ReverbExpertPageWidget final : public ReferencePageWidget<Reference::CentralArea, Vertical>
{
    using R = Reference::CentralArea;
    using BaseWidget = ReferencePageWidget<R, Vertical>;
    using BaseWidget::BaseWidget;

    const std::vector<std::shared_ptr<LabWidget>> fWidgets = {
        addWidget<ReverbEarlyGroupWidget>(),
        addWidget<ReverbTailGroupWidget>(),
        addSpacer(),
        addWidget<ReverbEarlyGroupWidget>(),
    };
};

// --------------------------------------------------------------------------------------------------------------------

class ReverbCentralAreaWidget final : public CentralAreaWidget<EasyPageWidget, ReverbExpertPageWidget>
{
    using BaseWidget = CentralAreaWidget<EasyPageWidget, ReverbExpertPageWidget>;
    using BaseWidget::BaseWidget;

public:
    Size<uint> getGraphSize() const noexcept final
    {
        const LabMetrics<R, uint> m { this };

        const uint space = m.margin + m.border + m.padding + (
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
