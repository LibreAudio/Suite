// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/containers/box-frame.hpp"
#include "ui/containers/central-area.hpp"
#include "ui/containers/main-area.hpp"
#include "ui/containers/top-bar.hpp"
#include "ui/containers/ui.hpp"
#include "ui/widgets/number-box.hpp"
#include "ui/widgets/threshold-meter.hpp"
#include "ui/widgets/switch-box.hpp"

#include "LibreAudioParameters.hpp"

namespace LibreAudio {

// kFaustParameterThreshold,
// kFaustParameterRatio,
// kFaustParameterKnee,
// kFaustParameterRange,
// kFaustParameterScale_lm,
// kFaustParameterScale_rs,
// kFaustParameterLookahead,
// kFaustParameterAttack,
// kFaustParameterAttack_curve,
// kFaustParameterHold,
// kFaustParameterRelease,
// kFaustParameterRelease_curve,
// kFaustParameterRelease_s,
// kFaustParameterAuto_release,
// ,
// kFaustParameterMid_side,
// ,
// kFaustParameterDrive,
// kFaustParameterMakeup,
// kFaustParameterDrywet,
// kFaustParameterGr_1,
// kFaustParameterGr_2,

// --------------------------------------------------------------------------------------------------------------------

static constexpr const char kTextCOMPRESSION_RESPONSE[] = "COMPRESSION RESPONSE";
static constexpr const char kTextDETECTOR[] = "DETECTOR";
static constexpr const char kTextSIDECHAIN[] = "SIDECHAIN";

// --------------------------------------------------------------------------------------------------------------------

using CompressorExpertPageWidget = StaticContainerSizeHintPairWidget<
    Reference::Transparent<Reference::CentralArea>,
    Vertical,
    SizeHintWidgetPair<
        Expanding,
        StaticContainerWidget<
            Reference::Height<0, Reference::Widgets::BoxFrame<1>>,
            Horizontal,
            Expanding
    //         // StaticContainerWidget<
    //         //     Reference::Widgets::BoxFrame<6>>::Row,
    //         //     Vertical,
    //         //     Fixed,
    //         //     NumberBoxWidget<kFaustParameterThreshold>
    //         // >
    //         // NumberBoxWithMeterWidget<kFaustParameterThreshold, kFaustParameterGr>,
    //         // NumberBoxWidget<kFaustParameterCeiling>
        >
    >,
    SizeHintWidgetPair<
        Fixed,
        StaticContainerWidget<
            Reference::Widgets::BoxFrame<3, true>,
            Vertical,
            Fixed,
            BoxFrameTitle<Reference::Colors::green, kTextCOMPRESSION_RESPONSE>,
            LabReferenceWidget<Reference::Height<
                Reference::Widgets::BoxFrame<1>::Row::height * 2 + Reference::Widgets::BoxFrame<1>::Row::padding,
                Reference::Widgets::NumberBox<>
            >>,
            StaticContainerWidget<
                Reference::Widgets::BoxFrame<1>::Row,
                Horizontal,
                Expanding,
                NumberBoxWidget<kFaustParameterPeak_rms>,
                NumberBoxWidget<kFaustParameterRms_time>,
                NumberBoxWidget<kFaustParameterFeedback>,
                NumberBoxWidget<kFaustParameterLink>
            >
        >
    >,
    SizeHintWidgetPair<
        Fixed,
        StaticContainerWidget<
            Reference::Transparent<Reference::Margin<0, Reference::Widgets::BoxFrame<1, true>>>,
            Horizontal,
            Expanding,
            StaticContainerWidget<
                Reference::Widgets::BoxFrame<1, true>,
                Vertical,
                Fixed,
                BoxFrameTitle<Reference::Colors::babyblue, kTextDETECTOR>,
                StaticContainerWidget<
                    Reference::Widgets::BoxFrame<1, true>::Row,
                    Horizontal,
                    Expanding,
                    NumberBoxWidget<kFaustParameterPeak_rms>,
                    NumberBoxWidget<kFaustParameterRms_time>,
                    NumberBoxWidget<kFaustParameterFeedback>,
                    NumberBoxWidget<kFaustParameterLink>
                >
            >,
            StaticContainerWidget<
                Reference::Widgets::BoxFrame<1, true>,
                Vertical,
                Fixed,
                BoxFrameTitle<Reference::Colors::yellow, kTextSIDECHAIN>,
                StaticContainerWidget<
                    Reference::Widgets::BoxFrame<1, true>::Row,
                    Horizontal,
                    Expanding,
                    // TODO needs to be dynamic
                    NumberBoxWidget<kFaustParameterSc_mix>,
                    NumberBoxWidget<kFaustParameterSc_hp>,
                    NumberBoxWidget<kFaustParameterSc_lp>,
                    NumberBoxWidget<kFaustParameterSc_tilt>
                >
            >
        >
    >
>;

// --------------------------------------------------------------------------------------------------------------------

class CompressorCentralAreaWidget final : public CentralAreaWidget<EasyPageWidget, CompressorExpertPageWidget>
{
    using BaseWidget = CentralAreaWidget<EasyPageWidget, CompressorExpertPageWidget>;
    using BaseWidget::BaseWidget;

public:
    Size<uint> getGraphSize() const noexcept final
    {
        const LabMetrics<R> m { this };

        const float space = m.margin + m.border + m.padding + (
            fLastPage == kPageEasy ? getEasyWidget()->getLastWidgetHeight()
                                   : 0 // getExpertWidget()->getLastWidgetHeight()
        );

        return Size<uint>(getWidth() - m.border * 2, getHeight() - space);
    }
};

// --------------------------------------------------------------------------------------------------------------------

using CompressorMainArea = MainAreaContainerWidget<CompressorCentralAreaWidget>;

// --------------------------------------------------------------------------------------------------------------------

class CompressorRootWidget : public RootWidget<TopBar, CompressorMainArea>
{
    using BaseWidget = RootWidget<TopBar, CompressorMainArea>;
    using BaseWidget::BaseWidget;
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

UI* createUI()
{
    return new LibreAudio::UI<LibreAudio::CompressorRootWidget>();
}

END_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------
