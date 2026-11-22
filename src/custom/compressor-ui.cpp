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

// --------------------------------------------------------------------------------------------------------------------

static constexpr const char kTextCOMPRESSION_RESPONSE[] = "COMPRESSION RESPONSE";
static constexpr const char kTextDETECTOR[] = "DETECTOR";
static constexpr const char kTextSIDECHAIN[] = "SIDECHAIN";

// --------------------------------------------------------------------------------------------------------------------

class CompressorBottomRightWidget final : public BoxFrameWidget<Horizontal, 1, true>,
                                          private IdleCallback
{
    using R = Reference::Widgets::BoxFrame<1, true>;
    using BaseWidget = BoxFrameWidget<Horizontal, 1, true>;

    using HeaderWidget = StaticContainerWidget<
        R::Header,
        Horizontal,
        Fixed,
        LabReferenceStaticTextWidget<R::Header::Title<Reference::Colors::yellow>, kTextSIDECHAIN>,
        LabEmptyWidget,
        ParameterScalePointsButtonGroupWidget<R::Header::ButtonGroup, Reference::Colors::yellow, kFaustParameterSc_filter>,
        StaticParameterButtonGroupWidget<
            R::Header::ButtonGroup,
            ParameterWidgetPair<
                kFaustParameterSc_listen,
                ImageButtonWidget<
                    R::Header::ButtonGroup::Button<Reference::Colors::yellow>,
                    kCornerBoth,
                    IMAGES_SOLO_28_PNG_DATA,
                    IMAGES_SOLO_28_PNG_LEN
                >
            >
        >
    >;

    std::unordered_map<FaustParameterIndex, std::shared_ptr<LabParameterWidget>> fWidgets;

public:
    CompressorBottomRightWidget(LabWidget* const parent)
        : BaseWidget(parent)
    {
        addHeader<HeaderWidget>();
        addWidget<kFaustParameterSc_mix>();
        addWidget<kFaustParameterSc_tilt>();
        addWidget<kFaustParameterSc_hp>();
        addWidget<kFaustParameterSc_lp>();
        addWidget<kFaustParameterSc_band_freq>();
        addWidget<kFaustParameterSc_band_q>();
        addWidget<kFaustParameterSc_bell_freq>();
        addWidget<kFaustParameterSc_bell_gain>();
        updateItemVisibility();
        addIdleCallback(this);
    }

private:
    uint fFilterValue = getFilterValue();

    template<FaustParameterIndex kParameterIndex>
    void addWidget()
    {
        fWidgets[kParameterIndex] = BaseWidget::addWidget<NumberBoxWidget<kParameterIndex>, Expanding>();
    }

    uint getFilterValue() const noexcept
    {
        return d_roundToUnsignedInt(fInterface->getParameterValue(kParametersMainStart + kFaustParameterSc_filter));
    }

    void idleCallback() final
    {
        if (const uint filter = getFilterValue(); fFilterValue != filter)
        {
            fFilterValue = filter;
            updateItemVisibility();
        }
    }

    void updateItemVisibility()
    {
        fWidgets[kFaustParameterSc_hp]->setVisible(fFilterValue == 0);
        fWidgets[kFaustParameterSc_lp]->setVisible(fFilterValue == 0);
        fWidgets[kFaustParameterSc_band_freq]->setVisible(fFilterValue == 1);
        fWidgets[kFaustParameterSc_band_q]->setVisible(fFilterValue == 1);
        fWidgets[kFaustParameterSc_bell_freq]->setVisible(fFilterValue == 2);
        fWidgets[kFaustParameterSc_bell_gain]->setVisible(fFilterValue == 2);
        updateSize(true);
    }
};

// --------------------------------------------------------------------------------------------------------------------

using CompressorEnvelopeWidget = LabReferenceWidget<
    Reference::Height<
        Reference::Widgets::BoxFrame<1>::Row::height * 2 + Reference::Widgets::BoxFrame<1>::Row::padding,
        Reference::Widgets::NumberBox<>
    >
>;

// --------------------------------------------------------------------------------------------------------------------

using CompressorVerticalBoxFrameReference = Reference::Width<100, Reference::Widgets::BoxFrame<0>>;

template<SizeHint sizeHint, typename... Args>
using StaticVerticalCompressorBoxFrameWidget = StaticContainerWidget<
    CompressorVerticalBoxFrameReference,
    Vertical,
    sizeHint,
    Args...
>;

// --------------------------------------------------------------------------------------------------------------------

using CompressorExpertPageWidget = StaticContainerSizeHintPairWidget<
    Reference::Transparent<Reference::Margin<0, Reference::CentralArea>>,
    Vertical,
    SizeHintWidgetPair<Expanding,
        StaticContainerWidget<
            Reference::CentralArea,
            Horizontal,
            Fixed,
            StaticVerticalCompressorBoxFrameWidget<
                Expanding,
                ThresholdMeterWidget<
                    kFaustParameterThreshold,
                    kFaustParameterLevel_1,
                    kFaustParameterLevel_2
                >
            >,
            StaticVerticalCompressorBoxFrameWidget<
                Fixed,
                NumberBoxWidget<kFaustParameterThreshold>,
                NumberBoxWidget<kFaustParameterRatio>,
                LabEmptyWidget,
                NumberBoxWidget<kFaustParameterKnee>,
                NumberBoxWidget<kFaustParameterRange>
            >,
            LabEmptyWidget,
            StaticVerticalCompressorBoxFrameWidget<
                Fixed,
                NumberBoxWidget<kFaustParameterDrive>,
                SwitchBoxWidget<kFaustParameterMid_side>,
                LabEmptyWidget,
                NumberBoxWidget<kFaustParameterMakeup>,
                NumberBoxWidget<kFaustParameterDrywet>
            >,
            StaticVerticalCompressorBoxFrameWidget<
                Expanding,
                ThresholdMeterWidget<
                    kFaustParameterGr_1,
                    kFaustParameterGr_1,
                    kFaustParameterGr_2
                >
            >
        >
    >,
    SizeHintWidgetPair<Fixed,
        StaticContainerWidget<
            Reference::Widgets::BoxFrame<3, true>,
            Vertical,
            Fixed,
            StaticBoxFrameTitleWidget<Reference::Colors::green, kTextCOMPRESSION_RESPONSE>,
            CompressorEnvelopeWidget,
            StaticBoxFrameRowWidget<
                NumberBoxWidget<kFaustParameterPeak_rms>,
                NumberBoxWidget<kFaustParameterRms_time>,
                NumberBoxWidget<kFaustParameterFeedback>,
                NumberBoxWidget<kFaustParameterLink>
            >
        >
    >,
    SizeHintWidgetPair<Fixed,
        StaticContainerWidget<
            Reference::BoxFrameArea<1, true>,
            Horizontal,
            Expanding,
            StaticHorizontalTitledBoxFrameWidget<
                Reference::Colors::babyblue,
                kTextDETECTOR,
                NumberBoxWidget<kFaustParameterPeak_rms>,
                NumberBoxWidget<kFaustParameterRms_time>,
                NumberBoxWidget<kFaustParameterFeedback>,
                NumberBoxWidget<kFaustParameterLink>
            >,
            CompressorBottomRightWidget
        >
    >
>;

// --------------------------------------------------------------------------------------------------------------------

class CompressorCentralAreaWidget final : public CentralAreaWidget<EasyPageWidget, CompressorExpertPageWidget>
{
    using BaseWidget = CentralAreaWidget<EasyPageWidget, CompressorExpertPageWidget>;
    using BaseWidget::BaseWidget;

public:
    [[nodiscard]] Size<uint> getCentralAreaSize() const noexcept
    {
        if (getCurrentPage(fInterface) == kPageExpert)
            return getGraphSize();

        return getSize();
    }

    [[nodiscard]] Size<uint> getGraphSize() const noexcept final
    {
        if (getCurrentPage(fInterface) == kPageExpert)
            return Size<uint>(getWidth(), getExpertWidget()->getFirstWidgetHeight());

        const LabMetrics<R> m { this };

        return Size<uint>(getWidth() - m.border * 2,
                          getHeight() - getEasyWidget()->getLastWidgetHeight() - m.margin + m.border + m.padding);
    }

private:
    void onNanoDisplay() final
    {
    }
};

// --------------------------------------------------------------------------------------------------------------------

class CompressorMainArea : public MainAreaContainerWidget<CompressorCentralAreaWidget>
{
    using BaseWidget = MainAreaContainerWidget<CompressorCentralAreaWidget>;
    using BaseWidget::BaseWidget;

public:
    [[nodiscard]] Size<uint> getCentralAreaSize() const noexcept
    {
        return getCentralAreaWidget()->getCentralAreaSize();
    }
};

// --------------------------------------------------------------------------------------------------------------------

class CompressorRootWidget : public RootWidget<TopBar<>, CompressorMainArea>
{
    using BaseWidget = RootWidget<TopBar<>, CompressorMainArea>;
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
