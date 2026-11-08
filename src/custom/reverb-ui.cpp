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
// kFaustParameterDeess_meter
// kFaustParameterDuck_meter

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
        StaticNumberBoxWidget<kFaustParameterEr_size, Reference::Colors::yellow>,
        StaticNumberBoxWidget<kFaustParameterEr_diffusion, Reference::Colors::yellow>,
        StaticNumberBoxWidget<kFaustParameterEr_damping, Reference::Colors::yellow>,
        StaticNumberBoxWidget<kFaustParameterEr_spread, Reference::Colors::yellow>,
        StaticNumberBoxWidget<kFaustParameterTail_feed, Reference::Colors::yellow>
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
        StaticNumberBoxWidget<kFaustParameterDecay, Reference::Colors::green>,
        StaticNumberBoxWidget<kFaustParameterTail_size, Reference::Colors::green>,
        StaticNumberBoxWidget<kFaustParameterTail_shape, Reference::Colors::green>,
        StaticNumberBoxWidget<kFaustParameterTail_spread, Reference::Colors::green>,
        StaticNumberBoxWidget<kFaustParameterDensity, Reference::Colors::green>
    >,
    StaticContainerWidget<
        Reference::Widgets::BoxFrame<2, true>::Row,
        Horizontal,
        Expanding,
        StaticNumberBoxWidget<kFaustParameterBass_mult, Reference::Colors::green>,
        StaticNumberBoxWidget<kFaustParameterBass_freq, Reference::Colors::green>,
        StaticNumberBoxWidget<kFaustParameterHf_damp, Reference::Colors::green>,
        StaticNumberBoxWidget<kFaustParameterMod_rate, Reference::Colors::green>,
        StaticNumberBoxWidget<kFaustParameterMod_depth, Reference::Colors::green>
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
        StaticNumberBoxWidget<kFaustParameterPredelay, Reference::Colors::red>,
        StaticNumberBoxWidget<kFaustParameterDim, Reference::Colors::red>
    >,
    StaticContainerWidget<
        Reference::Widgets::BoxFrame<2, true>,
        Vertical,
        Fixed,
        BoxFrameTitle<Reference::Colors::blue, kTextFILTERS>,
        StaticNumberBoxWidget<kFaustParameterLowcut, Reference::Colors::blue>,
        StaticNumberBoxWidget<kFaustParameterHighcut, Reference::Colors::blue>
    >,
    StaticContainerWidget<
        Reference::Widgets::BoxFrame<2, true>,
        Vertical,
        Fixed,
        BoxFrameTitle<Reference::Colors::pink, kTextDYNAMICS>,
        StaticNumberBoxWidget<kFaustParameterDeess_amount, Reference::Colors::pink>,
        StaticNumberBoxWidget<kFaustParameterDuck_amount, Reference::Colors::pink>
    >,
    StaticContainerWidget<
        Reference::Widgets::BoxFrame<2, true>,
        Vertical,
        Fixed,
        BoxFrameTitle<Reference::Colors::babyblue, kTextMIXER>,
        StaticNumberBoxWidget<kFaustParameterEr_level, Reference::Colors::babyblue>,
        StaticNumberBoxWidget<kFaustParameterTail_level, Reference::Colors::babyblue>
    >,
    StaticContainerWidget<
        Reference::Widgets::BoxFrame<2, true>,
        Vertical,
        Fixed,
        BoxFrameTitle<Reference::Colors::babyblue, kTextOUTPUT>,
        StaticNumberBoxWidget<kFaustParameterStereo_width, Reference::Colors::babyblue>,
        StaticNumberBoxWidget<kFaustParameterDrywet, Reference::Colors::babyblue>
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
