// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/containers/container.hpp"
#include "ui/containers/knob-group.hpp"
#include "ui/containers/main-area.hpp"
#include "ui/containers/top-bar.hpp"
#include "ui/containers/ui.hpp"
#include "ui/widgets/card.hpp"
#include "ui/widgets/knob.hpp"

#include "LibreAudioParameters.hpp"

#include <functional>

// --------------------------------------------------------------------------------------------------------------------

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

static constexpr const uint kCommonMargin = 8;
static constexpr const uint kCommonPadding = 6;
static constexpr const uint kKnobsGroupWidth = 330;

// --------------------------------------------------------------------------------------------------------------------

struct DjFilterGroupReference : Reference::Zero {
    static constexpr const Color backgroundColor { 0x30, 0x32, 0x3c, 0.7f };
    static constexpr const Color borderColor = backgroundColor;
    static constexpr const Color borderHighlightColor { 0x40, 0x42, 0x4c, 0.8f };
    static constexpr const Color borderShadowColor { 0, 0, 0, 0.12f };
    static constexpr const uint border = 1;
    static constexpr const uint borderRadius = 8;
    static constexpr const uint margin = kCommonMargin;
    static constexpr const uint padding = kCommonPadding;
    static constexpr const uint height = Reference::Widgets::Knob::height + (border + margin) * 2;

    static constexpr const Color shadowColor { 0, 0, 0, 0.6f };
    static constexpr const Point<int> shadowOffset = { 0, 5 };
    static constexpr const Size<int> shadowSize = { -12, -12 };
    static constexpr const uint shadowFeather = 14;
};

// reverb, resonance and overdrive knobs, evenly spaced inside a group well
class DjFilterKnobsGroupWidget final : public ReferenceContainerWidget<Reference::Width<kKnobsGroupWidth, DjFilterGroupReference>, Horizontal>
{
    using BaseWidget = ReferenceContainerWidget<Reference::Width<kKnobsGroupWidth, DjFilterGroupReference>, Horizontal>;

    const std::array<std::shared_ptr<LabWidget>, 7> widgets {
        addSpacer(),
        addKnob(kFaustParameterReverb_send),
        addSpacer(),
        addKnob(kFaustParameterEmphasize_q),
        addSpacer(),
        addKnob(kFaustParameterOverdrive),
        addSpacer(),
    };

public:
    explicit DjFilterKnobsGroupWidget(LabWidget* const parent)
        : BaseWidget(parent) {}

private:
    std::shared_ptr<LabWidget> addKnob(const FaustParameterIndex index)
    {
        // the knob keeps a reference to its parameter, so pass the static entry and not a copy
        return addWidget<SmallKnobWidget>(std::cref(kFaustParameters[index]), kParametersMainStart + index);
    }
};

// --------------------------------------------------------------------------------------------------------------------

struct DjFilterGroupsContainerReference : Reference::Zero {
    static constexpr const uint padding = kCommonPadding;
    static constexpr const uint height = DjFilterGroupReference::height + (border + margin) * 2;
};

using DjFilterGroupsContainerWidget = StaticContainerPairWidget<
    DjFilterGroupsContainerReference,
    Horizontal,
    SizeHintWidgetPair<Expanding, LabEmptyWidget>,
    SizeHintWidgetPair<Fixed, DjFilterKnobsGroupWidget>,
    SizeHintWidgetPair<Expanding, LabEmptyWidget>
>;

// --------------------------------------------------------------------------------------------------------------------

class DjFilterPresetCardsWidget : public CardGroupWidget
{
    const std::array<std::shared_ptr<LabCardWidget>, 4> cards = {
        addCard(kWidgetPresetA, "CLEAN SWEEP", "Pure filter, nothing added"),
        addCard(kWidgetPresetB, "RESONANT", "Squelchy peak at the cutoff"),
        addCard(kWidgetPresetC, "DUB SPACE", "Reverb tail on the sweep"),
        addCard(kWidgetPresetD, "DIRTY", "Driven, resonant sweep"),
    };

public:
    explicit DjFilterPresetCardsWidget(LabWidget* const parent)
        : CardGroupWidget(parent)
    {
        done();
    }
};

// --------------------------------------------------------------------------------------------------------------------

struct DjFilterEasyExpertPageReference : Reference::Zero {
    static constexpr const uint margin = 12;
    static constexpr const uint padding = 8;
};

// big filter knob centered in the display, preset cards at the bottom
using DjFilterEasyPageWidget = StaticContainerPairWidget<
    DjFilterEasyExpertPageReference,
    Vertical,
    SizeHintWidgetPair<Expanding, LabEmptyWidget>,
    SizeHintWidgetPair<Fixed, EasyKnobsGroupWidget>,
    SizeHintWidgetPair<Expanding, LabEmptyWidget>,
    SizeHintWidgetPair<Fixed, DjFilterPresetCardsWidget>
>;

// big filter knob centered in the display, knobs group at the bottom
using DjFilterExpertPageWidget = StaticContainerPairWidget<
    DjFilterEasyExpertPageReference,
    Vertical,
    SizeHintWidgetPair<Expanding, LabEmptyWidget>,
    SizeHintWidgetPair<Fixed, EasyKnobsGroupWidget>,
    SizeHintWidgetPair<Expanding, LabEmptyWidget>,
    SizeHintWidgetPair<Fixed, DjFilterGroupsContainerWidget>
>;

// --------------------------------------------------------------------------------------------------------------------

using DjFilterStageWidget = StageWidget<DjFilterEasyPageWidget, DjFilterExpertPageWidget>;
using DjFilterMainArea = MainAreaContainerWidget<DjFilterStageWidget>;
using DjFilterRootWidget = RootWidget<TopBar, DjFilterMainArea>;

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

UI* createUI()
{
    return new LibreAudio::UI<LibreAudio::DjFilterRootWidget,
                              SHADERS_ANALYSER_FFT_FRAG_DATA, SHADERS_ANALYSER_FFT_FRAG_LEN,
                              SHADERS_CURVE_DJ_FILTER_FRAG_DATA, SHADERS_CURVE_DJ_FILTER_FRAG_LEN>();
}

END_NAMESPACE_DISTRHO
