// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/containers/container.hpp"
#include "ui/containers/main-area.hpp"
#include "ui/containers/top-bar.hpp"
#include "ui/containers/ui.hpp"
#include "ui/widgets/number-box.hpp"

#include "LibreAudioParameters.hpp"

// --------------------------------------------------------------------------------------------------------------------

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

namespace widgets {
    using TruePeak = ParameterWidgetPair<kFaustParameterTrue_peak, NumberBoxWidget<Reference::Colors::acc6>>;
    using UnityGain = ParameterWidgetPair<kFaustParameterUnity_gain, NumberBoxWidget<Reference::Colors::acc4>>;

    using Threshold = ParameterWidgetPair<kFaustParameterThreshold, NumberBoxWidget<Reference::Colors::acc1>>;
    using Ceiling = ParameterWidgetPair<kFaustParameterCeiling, NumberBoxWidget<Reference::Colors::acc6>>;

    using Attack = ParameterWidgetPair<kFaustParameterAttack, NumberBoxWidget<Reference::Colors::acc5>>;
    using AttackShape = ParameterWidgetPair<kFaustParameterAttack_shape, NumberBoxWidget<Reference::Colors::acc5>>;

    using Release = ParameterWidgetPair<kFaustParameterRelease, NumberBoxWidget<Reference::Colors::acc3>>;
    using AutoRelease = ParameterWidgetPair<kFaustParameterAuto_release, NumberBoxWidget<Reference::Colors::acc3>>;

    using Dither = ParameterWidgetPair<kFaustParameterDither, NumberBoxWidget<Reference::Colors::acc2>>;
    using NoiseShaping = ParameterWidgetPair<kFaustParameterNoise_shaping, NumberBoxWidget<Reference::Colors::acc2>>;
}

// --------------------------------------------------------------------------------------------------------------------

static constexpr const uint kCommonMargin = 8;
static constexpr const uint kCommonPadding = 6;

struct LimiterGroupReference : Reference::Zero {
    static constexpr const Color backgroundColor { 0x30, 0x32, 0x3c, 0.7f };
    static constexpr const Color borderColor = backgroundColor;
    static constexpr const Color borderHighlightColor { 0x40, 0x42, 0x4c, 0.8f };
    static constexpr const Color borderShadowColor { 0, 0, 0, 0.12f };
    static constexpr const uint border = 1;
    static constexpr const uint borderRadius = 8;
    static constexpr const uint margin = kCommonMargin;
    static constexpr const uint padding = kCommonPadding;
    static constexpr const uint height = Reference::Widgets::NumberBox<>::height + (border + margin) * 2;

    // TESTING
    static constexpr const Color shadowColor { 0, 0, 0, 0.6f };
    static constexpr const Point<int> shadowOffset = { 0, 5 };
    // static constexpr const Size<int> shadowSize = { -6, -6 };
    static constexpr const Size<int> shadowSize = { -12, -12 };
    static constexpr const uint shadowFeather = 14;
};

template <Orientation orientation, class W1, class W2>
using LimiterGroupWidget = StaticParameterContainerWidget<LimiterGroupReference, orientation, Expanding, W1, W2>;

// --------------------------------------------------------------------------------------------------------------------

struct LimiterGroupsContainerReference : Reference::Zero {
    static constexpr const uint margin = kCommonMargin;
    static constexpr const uint padding = kCommonPadding;
    static constexpr const uint height = LimiterGroupReference::height + (border + margin) * 2;
};

using LimiterGroupsContainerWidget = StaticContainerWidget<
    LimiterGroupsContainerReference,
    kHorizontal,
    Expanding,
    LimiterGroupWidget<kHorizontal, widgets::TruePeak, widgets::UnityGain>,
    LimiterGroupWidget<kHorizontal, widgets::Threshold, widgets::Ceiling>,
    LimiterGroupWidget<kHorizontal, widgets::Attack, widgets::AttackShape>,
    LimiterGroupWidget<kHorizontal, widgets::Release, widgets::AutoRelease>,
    LimiterGroupWidget<kHorizontal, widgets::Dither, widgets::NoiseShaping>
>;

// --------------------------------------------------------------------------------------------------------------------

struct LimiterExpertPageReference : Reference::Zero {
    static constexpr const uint margin = 12;
    static constexpr const uint padding = 8;
};

using LimterExpertPageWidget = StaticBottomAlignedContainerWidget<LimiterExpertPageReference, LimiterGroupsContainerWidget>;

// --------------------------------------------------------------------------------------------------------------------

using LimterStageWidget = StageWidget<EasyStageWidget, LimterExpertPageWidget>;
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
