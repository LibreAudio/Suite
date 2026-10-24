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

template <Orientation orientation, FaustParameterIndex i1, class W1, FaustParameterIndex i2, class W2>
class LimiterGroupWidget final : public ReferenceContainerWidget<LimiterGroupReference, orientation>
{
    const std::array<std::shared_ptr<LabWidget>, 2> widgets {
        BaseWidget::template addWidget<W1, Expanding>(i1),
        BaseWidget::template addWidget<W2, Expanding>(i2),
    };

public:
    using BaseWidget = ReferenceContainerWidget<LimiterGroupReference, orientation>;

    explicit LimiterGroupWidget(LabWidget* const parent)
        : BaseWidget(parent) {}
};

// --------------------------------------------------------------------------------------------------------------------

struct LimiterGroupsContainerReference : Reference::Zero {
    static constexpr const uint margin = kCommonMargin;
    static constexpr const uint padding = kCommonPadding;
    static constexpr const uint height = LimiterGroupReference::height + (border + margin) * 2;
};

class LimiterGroupsContainerWidget final : public ReferenceContainerWidget<LimiterGroupsContainerReference, kHorizontal>
{
    using Group1 = LimiterGroupWidget<kHorizontal,
                                      kFaustParameterTrue_peak,
                                      NumberBoxWidget<Reference::Colors::acc1>,
                                      kFaustParameterUnity_gain,
                                      NumberBoxWidget<Reference::Colors::acc2>>;
    using Group2 = LimiterGroupWidget<kHorizontal,
                                      kFaustParameterThreshold,
                                      NumberBoxWidget<Reference::Colors::acc1>,
                                      kFaustParameterCeiling,
                                      NumberBoxWidget<Reference::Colors::acc2>>;
    using Group3 = LimiterGroupWidget<kHorizontal,
                                      kFaustParameterLookahead,
                                      NumberBoxWidget<Reference::Colors::acc3>,
                                      kFaustParameterLookahead,
                                      NumberBoxWidget<Reference::Colors::acc3>>;
    using Group4 = LimiterGroupWidget<kHorizontal,
                                      kFaustParameterRelease,
                                      NumberBoxWidget<Reference::Colors::acc4>,
                                      kFaustParameterRelease,
                                      NumberBoxWidget<Reference::Colors::acc4>>;
    using Group5 = LimiterGroupWidget<kHorizontal,
                                      kFaustParameterDither,
                                      NumberBoxWidget<Reference::Colors::acc1>,
                                      kFaustParameterNoise_shaping,
                                      NumberBoxWidget<Reference::Colors::acc2>>;

    const std::array<std::shared_ptr<LabWidget>, 5> widgets {
        addWidget<Group1, Expanding>(),
        addWidget<Group2, Expanding>(),
        addWidget<Group3, Expanding>(),
        addWidget<Group4, Expanding>(),
        addWidget<Group5, Expanding>(),
    };

public:
    using R = LimiterGroupsContainerReference;
    using BaseWidget = ReferenceContainerWidget<R, kHorizontal>;

    explicit LimiterGroupsContainerWidget(LabWidget* const parent)
        : BaseWidget(parent) {}
};

// --------------------------------------------------------------------------------------------------------------------

struct LimiterExpertPageReference : Reference::Zero {
    static constexpr const uint margin = 12;
    static constexpr const uint padding = 8;
};

class LimterExpertPageWidget final : public ReferenceContainerWidget<LimiterExpertPageReference, kVertical>
{
    const std::array<std::shared_ptr<LabWidget>, 2> widgets {
        addSpacer(),
        addWidget<LimiterGroupsContainerWidget>(),
    };

public:
    using BaseWidget = ReferenceContainerWidget<LimiterExpertPageReference, kVertical>;

    explicit LimterExpertPageWidget(LabWidget* const parent)
        : BaseWidget(parent) {}
};

// --------------------------------------------------------------------------------------------------------------------

class LimterStageWidget : public StageWidget<EasyStageWidget, LimterExpertPageWidget>
{
public:
    // using R = LimterStageReference;
    using BaseWidget = StageWidget<EasyStageWidget, LimterExpertPageWidget>;

    explicit LimterStageWidget(LabWidget* const parent)
        : BaseWidget(parent)
    {
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
