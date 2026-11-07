// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/containers/container.hpp"
#include "ui/containers/main-area.hpp"
#include "ui/containers/top-bar.hpp"
#include "ui/containers/ui.hpp"
#include "ui/widgets/check-box.hpp"
#include "ui/widgets/number-box.hpp"

#include "LibreAudioParameters.hpp"

// --------------------------------------------------------------------------------------------------------------------

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

static constexpr const uint kCommonMargin = 8;
static constexpr const uint kCommonPadding = 6;
static constexpr const uint kNumberBoxHeight = 50;
static constexpr const uint kCheckBoxGroupWidth = 115;

// two stacked in a number box's height
template<const Color& kAccentColor>
using DeEsserCheckBoxWidget = CheckBoxWidget<kAccentColor, (kNumberBoxHeight - kCommonPadding) / 2>;

template<const Color& kAccentColor>
using DeEsserNumberBoxWidget = NumberBoxWidget<kAccentColor, kNumberBoxHeight>;

// --------------------------------------------------------------------------------------------------------------------

namespace widgets {
    using Crossover = ParameterWidgetPair<kFaustParameterCrossover_frequency, DeEsserNumberBoxWidget<Reference::Colors::acc2>>;
    using Channel = ParameterWidgetPair<kFaustParameterChannel, DeEsserNumberBoxWidget<Reference::Colors::acc5>>;

    using Ratio = ParameterWidgetPair<kFaustParameterRatio, DeEsserNumberBoxWidget<Reference::Colors::acc3>>;
    using Range = ParameterWidgetPair<kFaustParameterRange, DeEsserNumberBoxWidget<Reference::Colors::acc4>>;

    using Lookahead = ParameterWidgetPair<kFaustParameterLookahead, DeEsserNumberBoxWidget<Reference::Colors::acc4>>;
    using DryWet = ParameterWidgetPair<kFaustParameterDrywet, DeEsserNumberBoxWidget<Reference::Colors::acc2>>;

    using Delta = ParameterWidgetPair<kFaustParameterDelta, DeEsserCheckBoxWidget<Reference::Colors::acc6>>;
    using Listen = ParameterWidgetPair<kFaustParameterListen, DeEsserCheckBoxWidget<Reference::Colors::acc4>>;
}

// --------------------------------------------------------------------------------------------------------------------

struct DeEsserGroupReference : Reference::Zero {
    static constexpr const Color backgroundColor { 0x30, 0x32, 0x3c, 0.7f };
    static constexpr const Color borderColor = backgroundColor;
    static constexpr const Color borderHighlightColor { 0x40, 0x42, 0x4c, 0.8f };
    static constexpr const Color borderShadowColor { 0, 0, 0, 0.12f };
    static constexpr const uint border = 1;
    static constexpr const uint borderRadius = 8;
    static constexpr const uint margin = kCommonMargin;
    static constexpr const uint padding = kCommonPadding;
    static constexpr const uint height = kNumberBoxHeight + (border + margin) * 2;

    static constexpr const Color shadowColor { 0, 0, 0, 0.6f };
    static constexpr const Point<int> shadowOffset = { 0, 5 };
    static constexpr const Size<int> shadowSize = { -12, -12 };
    static constexpr const uint shadowFeather = 14;
};

template <typename... Args>
using DeEsserGroupWidget = StaticParameterContainerWidget<DeEsserGroupReference, Horizontal, Expanding, Args...>;

// the monitoring switches, stacked
using DeEsserCheckBoxGroupWidget = StaticParameterContainerWidget<
    Reference::Width<kCheckBoxGroupWidth, DeEsserGroupReference>,
    Vertical,
    Expanding,
    widgets::Delta, widgets::Listen
>;

// --------------------------------------------------------------------------------------------------------------------

struct DeEsserGroupsContainerReference : Reference::Zero {
    static constexpr const uint padding = kCommonPadding;
    static constexpr const uint height = DeEsserGroupReference::height + (border + margin) * 2;
};

// Band, amount, timing and mix, then the monitoring switches, side by side.
using DeEsserMainGroupsWidget = StaticContainerPairWidget<
    DeEsserGroupsContainerReference,
    Horizontal,
    SizeHintWidgetPair<Expanding, DeEsserGroupWidget<widgets::Crossover, widgets::Channel>>,
    SizeHintWidgetPair<Expanding, DeEsserGroupWidget<widgets::Ratio, widgets::Range>>,
    SizeHintWidgetPair<Expanding, DeEsserGroupWidget<widgets::Lookahead, widgets::DryWet>>,
    SizeHintWidgetPair<Fixed, DeEsserCheckBoxGroupWidget>
>;

// --------------------------------------------------------------------------------------------------------------------

struct DeEsserPageReference : Reference::Zero {
    static constexpr const uint margin = 12;
    static constexpr const uint padding = 8;
};

// The control row at the bottom of the stage, the space above it left to the display.
using DeEsserPageWidget = StaticBottomAlignedContainerWidget<DeEsserPageReference, DeEsserMainGroupsWidget>;

// --------------------------------------------------------------------------------------------------------------------
// The stage: the starfield and the control row at the bottom. Easy and Expert pages show the same controls for now.

class DeEsserStageWidget final : public StageWidget<DeEsserPageWidget, DeEsserPageWidget>
{
    using BaseWidget = StageWidget<DeEsserPageWidget, DeEsserPageWidget>;

public:
    explicit DeEsserStageWidget(LabWidget* const parent)
        : BaseWidget(parent) {}
};

using DeEsserMainArea = MainAreaContainerWidget<DeEsserStageWidget>;
using DeEsserRootWidget = RootWidget<TopBar, DeEsserMainArea>;

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

UI* createUI()
{
    return new LibreAudio::UI<LibreAudio::DeEsserRootWidget>();
}

END_NAMESPACE_DISTRHO
