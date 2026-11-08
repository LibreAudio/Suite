// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/containers/combo-box-group.hpp"
#include "ui/containers/container.hpp"
#include "ui/containers/main-area.hpp"
#include "ui/containers/top-bar.hpp"
#include "ui/containers/ui.hpp"
#include "ui/widgets/button.hpp"
#include "ui/widgets/check-box.hpp"
#include "ui/widgets/combo-box.hpp"
#include "ui/widgets/floating-line-slider.hpp"
#include "ui/widgets/fft-waveform.hpp"
#include "ui/widgets/grid.hpp"
#include "ui/widgets/number-box.hpp"

#include "LibreAudioParameters.hpp"

// --------------------------------------------------------------------------------------------------------------------

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

static constexpr const uint kCommonMargin = 8;
static constexpr const uint kCommonPadding = 6;
static constexpr const uint kNumberBoxHeight = 40;

template<const Color& kAccentColor>
using LimiterCheckBoxWidget = CheckBoxWidget<kAccentColor>;

// template<const Color& kAccentColor>
using LimiterComboBoxWidget = ComboBoxWidget<Reference::Colors::acc2>;

template<const Color& kAccentColor>
using LimiterNumberBoxWidget = NumberBoxWidget<kAccentColor>;

// --------------------------------------------------------------------------------------------------------------------

namespace widgets {
    using TruePeak = ParameterWidgetPair<kFaustParameterTrue_peak, LimiterCheckBoxWidget<Reference::Colors::acc6>>;
    using UnityGain = ParameterWidgetPair<kFaustParameterUnity_gain, LimiterCheckBoxWidget<Reference::Colors::acc4>>;

    using Attack = ParameterWidgetPair<kFaustParameterAttack, LimiterNumberBoxWidget<Reference::Colors::acc5>>;
    using AttackShape = ParameterWidgetPair<kFaustParameterAttack_shape, LimiterNumberBoxWidget<Reference::Colors::acc5>>;

    using Release = ParameterWidgetPair<kFaustParameterRelease, LimiterNumberBoxWidget<Reference::Colors::acc3>>;
    using AutoRelease = ParameterWidgetPair<kFaustParameterAuto_release, LimiterNumberBoxWidget<Reference::Colors::acc3>>;
}

// --------------------------------------------------------------------------------------------------------------------

template <Orientation orientation, SizeHint sizeHint, typename... Args>
using LimiterGroupWidget = StaticParameterContainerWidget<
    Reference::Widgets::BoxFrame<1>,
    orientation,
    sizeHint,
    Args...
>;

template <Orientation orientation, SizeHint sizeHint, typename... Args>
using LimiterGroupWidget2 = StaticContainerWidget<
    Reference::Widgets::BoxFrame<1>,
    orientation,
    sizeHint,
    Args...
>;

// --------------------------------------------------------------------------------------------------------------------

struct LimiterGroupsContainerReference : Reference::Zero {
    static constexpr const uint padding = kCommonPadding;
    static constexpr const uint height = Reference::Widgets::BoxFrame<1>::height + (border + margin) * 2;
};

using LimiterGroupsContainerWidget = StaticContainerPairWidget<
    LimiterGroupsContainerReference,
    Horizontal,
    SizeHintWidgetPair<Fixed, LimiterGroupWidget<Vertical, Expanding, widgets::TruePeak, widgets::UnityGain>>,
    SizeHintWidgetPair<
        Expanding,
        LimiterGroupWidget2<
            Horizontal,
            Expanding,
            // StaticNumberBoxWithMeterWidget<kFaustParameterThreshold, kFaustParameterGr, Reference::Colors::acc1>,
            StaticNumberBoxWidget<kFaustParameterThreshold, Reference::Colors::acc1>,
            StaticNumberBoxWidget<kFaustParameterCeiling, Reference::Colors::acc6>
        >
    >,
    SizeHintWidgetPair<Expanding, LimiterGroupWidget<Horizontal, Expanding, widgets::Attack, widgets::AttackShape>>,
    SizeHintWidgetPair<Expanding, LimiterGroupWidget<Horizontal, Expanding, widgets::Release, widgets::AutoRelease>>,
    SizeHintWidgetPair<Fixed, ComboBoxGroupWidget<Reference::Widgets::BoxFrame<1>, LimiterComboBoxWidget, kFaustParameterDither, kFaustParameterNoise_shaping>>
>;

// --------------------------------------------------------------------------------------------------------------------

class LimiterPresetCardsWidget : public CardGroupWidget
{
    using CardGroupWidget::CardGroupWidget;

    const std::array<std::shared_ptr<CardWidget>, 4> cards = {
        addCard(kWidgetPresetA, "NATURAL DOUBLE", "Subtle ADT thickening"),
        addCard(kWidgetPresetB, "TIGHT SLAP", "Short doubled slap"),
        addCard(kWidgetPresetC, "WIDE TRIPLE", "Three-voice spread"),
        addCard(kWidgetPresetD, "LUSH STACK", "Full ensemble stack"),
    };
};

// --------------------------------------------------------------------------------------------------------------------

struct LimiterEasyExpertPageReference : Reference::Zero {
    static constexpr const uint margin = 12;
    static constexpr const uint padding = 8;
};

using LimterEasyPageWidget = StaticBottomAlignedContainerWidget<
    LimiterEasyExpertPageReference,
    LimiterPresetCardsWidget
>;

using LimterExpertPageWidget = StaticBottomAlignedContainerWidget<
    LimiterEasyExpertPageReference,
    LimiterGroupsContainerWidget
>;

// --------------------------------------------------------------------------------------------------------------------

class LimiterCentralAreaWidget final : public CentralAreaWidget<LimterEasyPageWidget, LimterExpertPageWidget>,
                                       private ButtonEventHandler::Callback
{
    using BaseWidget = CentralAreaWidget<LimterEasyPageWidget, LimterExpertPageWidget>;
    using ButtonWidget = TextButtonWidget<Reference::Height<30, Reference::Widgets::TopBarButton>, kCornerNone>;

    static constexpr const FaustParameter& kParamThreshold = kFaustParameters[kFaustParameterThreshold];

    static constexpr const float kPaddedMinimumScale = -48.f;

    const std::array<std::shared_ptr<LabWidget>, 3> _{
        addGraphWidget<GridWidget>(kParamThreshold.min - kParamThreshold.min / kPaddedMinimumScale,
                                   kParamThreshold.min,
                                   kParamThreshold.max,
                                   6.f,
                                   kParamThreshold.unit),
        addGraphWidget<WaveformWidget>(kParamThreshold.min - kParamThreshold.min / kPaddedMinimumScale,
                                       kParamThreshold.min,
                                       kParamThreshold.max),
        addGraphWidget<FloatingLineSliderWidget>(kFaustParameterThreshold, kParamThreshold.min / kPaddedMinimumScale),
    };

    const std::shared_ptr<ButtonWidget> fRangeButton {
        new ButtonWidget(this, 0, "Range Test Button")
    };

public:
    LimiterCentralAreaWidget(LabWidget* const parent)
        : BaseWidget(parent)
    {
        fRangeButton->setCallback(this);
    }

    Size<uint> getGraphSize() const noexcept final
    {
        const LabMetrics<R> m { this };

        const float space = m.margin + m.border + m.padding + (
            fLastPage == kPageEasy ? getEasyWidget()->getLastWidgetHeight()
                                   : getExpertWidget()->getLastWidgetHeight()
        );

        return Size<uint>(getWidth() - m.border * 2, getHeight() - space);
    }

private:
    int scale = 0;

    void buttonClicked(SubWidget*, int) final
    {
        scale = (scale + 1) % 3;
        float start;

        switch (scale)
        {
        case 0:
            start = -24.f;
            break;
        case 1:
            start = -12.f;
            break;
        case 2:
            start = -6.f;
            break;
        default:
            return;
        }

        const float min = start - start / kPaddedMinimumScale;
        setGraphRange(min, start, kParamThreshold.max);
    }

    void onPositionChanged(const PositionChangedEvent& ev) final
    {
        BaseWidget::onPositionChanged(ev);

        const LabMetrics<R> m { this };

        fRangeButton->setAbsolutePos(
            ev.pos.getX() + (m.margin + m.border),
            ev.pos.getY() + (m.margin + m.border));
    }
};

// --------------------------------------------------------------------------------------------------------------------

using LimiterMainArea = MainAreaContainerWidget<LimiterCentralAreaWidget>;
using LimiterRootWidget = RootWidget<TopBar, LimiterMainArea>;

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

UI* createUI()
{
    return new LibreAudio::UI<LibreAudio::LimiterRootWidget>();
}

END_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------
