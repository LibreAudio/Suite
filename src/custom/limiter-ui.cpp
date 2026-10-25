// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/containers/container.hpp"
#include "ui/containers/main-area.hpp"
#include "ui/containers/top-bar.hpp"
#include "ui/containers/ui.hpp"
#include "ui/widgets/check-box.hpp"
#include "ui/widgets/combo-box.hpp"
#include "ui/widgets/number-box.hpp"

#include "LibreAudioParameters.hpp"

// --------------------------------------------------------------------------------------------------------------------

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

static constexpr const uint kCommonMargin = 8;
static constexpr const uint kCommonPadding = 6;
static constexpr const uint kNumberBoxHeight = 50;
static constexpr const uint kGroupsWidth[5] = { 115, 0, 0, 0, 140 };

template<const Color& kAccentColor>
using LimiterNumberBoxWidget = NumberBoxWidget<kAccentColor, kNumberBoxHeight>;

// --------------------------------------------------------------------------------------------------------------------

namespace widgets {
    using TruePeak = ParameterWidgetPair<kFaustParameterTrue_peak, CheckBoxWidget<Reference::Colors::acc6>>;
    using UnityGain = ParameterWidgetPair<kFaustParameterUnity_gain, CheckBoxWidget<Reference::Colors::acc4>>;

    using Threshold = ParameterWidgetPair<kFaustParameterThreshold, LimiterNumberBoxWidget<Reference::Colors::acc1>>;
    using Ceiling = ParameterWidgetPair<kFaustParameterCeiling, LimiterNumberBoxWidget<Reference::Colors::acc6>>;

    using Attack = ParameterWidgetPair<kFaustParameterAttack, LimiterNumberBoxWidget<Reference::Colors::acc5>>;
    using AttackShape = ParameterWidgetPair<kFaustParameterAttack_shape, LimiterNumberBoxWidget<Reference::Colors::acc5>>;

    using Release = ParameterWidgetPair<kFaustParameterRelease, LimiterNumberBoxWidget<Reference::Colors::acc3>>;
    using AutoRelease = ParameterWidgetPair<kFaustParameterAuto_release, LimiterNumberBoxWidget<Reference::Colors::acc3>>;

    using Dither = ParameterWidgetPair<kFaustParameterDither, ComboBoxWidget<Reference::Colors::acc2>>;
    using NoiseShaping = ParameterWidgetPair<kFaustParameterNoise_shaping, ComboBoxWidget<Reference::Colors::acc2>>;
}

// --------------------------------------------------------------------------------------------------------------------

struct LimiterGroupReference : Reference::Zero {
    static constexpr const Color backgroundColor { 0x30, 0x32, 0x3c, 0.7f };
    static constexpr const Color borderColor = backgroundColor;
    static constexpr const Color borderHighlightColor { 0x40, 0x42, 0x4c, 0.8f };
    static constexpr const Color borderShadowColor { 0, 0, 0, 0.12f };
    static constexpr const uint border = 1;
    static constexpr const uint borderRadius = 8;
    static constexpr const uint margin = kCommonMargin;
    static constexpr const uint padding = kCommonPadding;
    static constexpr const uint height = kNumberBoxHeight + (border + margin) * 2;

    // TESTING
    static constexpr const Color shadowColor { 0, 0, 0, 0.6f };
    static constexpr const Point<int> shadowOffset = { 0, 5 };
    // static constexpr const Size<int> shadowSize = { -6, -6 };
    static constexpr const Size<int> shadowSize = { -12, -12 };
    static constexpr const uint shadowFeather = 14;
};

template <uint8_t pos, Orientation orientation, typename... Args>
using LimiterGroupWidget = StaticParameterContainerWidget<
    Reference::Width<kGroupsWidth[pos], LimiterGroupReference>,
    orientation,
    Expanding,
    Args...
>;

// --------------------------------------------------------------------------------------------------------------------

struct LimiterGroupsContainerReference : Reference::Zero {
    static constexpr const uint padding = kCommonPadding;
    static constexpr const uint height = LimiterGroupReference::height + (border + margin) * 2;
};

using LimiterGroupsContainerWidget = StaticContainerPairWidget<
    LimiterGroupsContainerReference,
    Horizontal,
    SizeHintWidgetPair<Fixed, LimiterGroupWidget<__COUNTER__, Vertical, widgets::TruePeak, widgets::UnityGain>>,
    SizeHintWidgetPair<Expanding, LimiterGroupWidget<__COUNTER__, Horizontal, widgets::Threshold, widgets::Ceiling>>,
    SizeHintWidgetPair<Expanding, LimiterGroupWidget<__COUNTER__, Horizontal, widgets::Attack, widgets::AttackShape>>,
    SizeHintWidgetPair<Expanding, LimiterGroupWidget<__COUNTER__, Horizontal, widgets::Release, widgets::AutoRelease>>,
    SizeHintWidgetPair<Fixed, LimiterGroupWidget<__COUNTER__, Vertical, widgets::Dither, widgets::NoiseShaping>>
>;

// --------------------------------------------------------------------------------------------------------------------

class LimiterPresetCardsWidget : public CardGroupWidget
{
    const std::array<std::shared_ptr<LabCardWidget>, 4> cards = {
        addCard(kWidgetPresetA, "NATURAL DOUBLE", "Subtle ADT thickening"),
        addCard(kWidgetPresetB, "TIGHT SLAP", "Short doubled slap"),
        addCard(kWidgetPresetC, "WIDE TRIPLE", "Three-voice spread"),
        addCard(kWidgetPresetD, "LUSH STACK", "Full ensemble stack"),
    };

public:
    explicit LimiterPresetCardsWidget(LabWidget* const parent)
        : CardGroupWidget(parent)
    {
        done();
    }
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

class LimterStageWidget final : public StageWidget<LimterEasyPageWidget, LimterExpertPageWidget>
{
    using BaseWidget = StageWidget<LimterEasyPageWidget, LimterExpertPageWidget>;

public:
    LimterStageWidget(LabWidget* const parent)
        : BaseWidget(parent)
    {
    }

private:
    void onNanoDisplay() final
    {
        BaseWidget::onNanoDisplay();
    }

    void onResize(const ResizeEvent& ev) override
    {
        BaseWidget::onResize(ev);
    }
};

// --------------------------------------------------------------------------------------------------------------------

using LimiterMainArea = MainAreaContainerWidget<LimterStageWidget>;
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
