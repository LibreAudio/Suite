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

// TODO meters for these
// kFaustParameterEr_meter_l
// kFaustParameterEr_meter_r
// kFaustParameterTail_meter_l
// kFaustParameterTail_meter_r

// --------------------------------------------------------------------------------------------------------------------

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

static constexpr const char kTextEARLY[] = "EARLY";
static constexpr const char kTextTAIL[] = "TAIL";
static constexpr const char kTextINPUT[] = "INPUT";
static constexpr const char kTextFILTERS[] = "FILTERS";
static constexpr const char kTextDYNAMICS[] = "DYNAMICS";
static constexpr const char kTextMIXER[] = "MIXER";
static constexpr const char kTextOUTPUT[] = "OUTPUT";

// --------------------------------------------------------------------------------------------------------------------

using ReverbEarlyGroupWidget = StaticContainerWidget<
    Reference::Widgets::BoxFrame<1, true>,
    Vertical,
    Fixed,
    BoxFrameTitle<Reference::Colors::yellow, kTextEARLY>,
    StaticContainerWidget<
        Reference::Widgets::BoxFrame<1, true>::Row,
        Horizontal,
        Expanding,
        NumberBoxWidget<kFaustParameterEr_size>,
        NumberBoxWidget<kFaustParameterEr_diffusion>,
        NumberBoxWidget<kFaustParameterEr_damping>,
        NumberBoxWidget<kFaustParameterEr_spread>,
        NumberBoxWidget<kFaustParameterTail_feed>
    >
>;

// --------------------------------------------------------------------------------------------------------------------

using ReverbTailGroupWidget = StaticContainerWidget<
    Reference::Widgets::BoxFrame<2, true>,
    Vertical,
    Fixed,
    BoxFrameTitle<Reference::Colors::green, kTextTAIL>,
    StaticContainerWidget<
        Reference::Widgets::BoxFrame<2, true>::Row,
        Horizontal,
        Expanding,
        NumberBoxWidget<kFaustParameterDecay>,
        NumberBoxWidget<kFaustParameterTail_size>,
        NumberBoxWidget<kFaustParameterTail_shape>,
        NumberBoxWidget<kFaustParameterTail_spread>,
        NumberBoxWidget<kFaustParameterDensity>
    >,
    StaticContainerWidget<
        Reference::Widgets::BoxFrame<2, true>::Row,
        Horizontal,
        Expanding,
        NumberBoxWidget<kFaustParameterBass_mult>,
        NumberBoxWidget<kFaustParameterBass_freq>,
        NumberBoxWidget<kFaustParameterHf_damp>,
        NumberBoxWidget<kFaustParameterMod_rate>,
        NumberBoxWidget<kFaustParameterMod_depth>
    >
>;

// --------------------------------------------------------------------------------------------------------------------

using ReverbBottomGroupWidget = StaticContainerWidget<
    Reference::Transparent<Reference::Margin<0, Reference::Widgets::BoxFrame<2, true>>>,
    Horizontal,
    Expanding,
    StaticContainerWidget<
        Reference::Widgets::BoxFrame<2, true>,
        Vertical,
        Fixed,
        BoxFrameTitle<Reference::Colors::red, kTextINPUT>,
        NumberBoxWidget<kFaustParameterPredelay>,
        NumberBoxWidget<kFaustParameterDim>
    >,
    StaticContainerWidget<
        Reference::Widgets::BoxFrame<2, true>,
        Vertical,
        Fixed,
        BoxFrameTitle<Reference::Colors::blue, kTextFILTERS>,
        NumberBoxWidget<kFaustParameterLowcut>,
        NumberBoxWidget<kFaustParameterHighcut>
    >,
    StaticContainerWidget<
        Reference::Widgets::BoxFrame<2, true>,
        Vertical,
        Fixed,
        BoxFrameTitle<Reference::Colors::pink, kTextDYNAMICS>,
        NumberBoxWithMeterWidget<kFaustParameterDeess_amount, kFaustParameterDeess_meter>,
        NumberBoxWithMeterWidget<kFaustParameterDuck_amount, kFaustParameterDuck_meter>
    >,
    StaticContainerWidget<
        Reference::Widgets::BoxFrame<2, true>,
        Vertical,
        Fixed,
        BoxFrameTitle<Reference::Colors::babyblue, kTextMIXER>,
        NumberBoxWidget<kFaustParameterEr_level>,
        NumberBoxWidget<kFaustParameterTail_level>
    >,
    StaticContainerWidget<
        Reference::Widgets::BoxFrame<2, true>,
        Vertical,
        Fixed,
        BoxFrameTitle<Reference::Colors::babyblue, kTextOUTPUT>,
        NumberBoxWidget<kFaustParameterStereo_width>,
        NumberBoxWidget<kFaustParameterDrywet>
    >
>;

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
        addWidget<ReverbBottomGroupWidget>(),
    };
};

// --------------------------------------------------------------------------------------------------------------------

using ReverbCentralAreaWidget = CentralAreaWidget<EasyPageWidget, ReverbExpertPageWidget>;
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
