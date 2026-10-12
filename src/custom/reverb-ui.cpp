// Libre Audio Suite
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/containers/container.hpp"
#include "ui/containers/main-area.hpp"
#include "ui/containers/top-bar.hpp"
#include "ui/containers/ui.hpp"
#include "ui/widgets/number-box.hpp"

#include "LibreAudioParameters.hpp"
#include "reverb/room.hpp"

namespace LibreAudio {

static constexpr Color kInputColor = Reference::kAccentColors[1];
static constexpr Color kOutputColor = Reference::kAccentColors[4];
static constexpr Color kEarlyColor = Reference::kAccentColors[2];
static constexpr Color kTailColor = Reference::kAccentColors[3];

static constexpr uint kSectionMargin = 8;
static constexpr uint kControlPadding = 4;

// Match mbComp5's panels, spacing, and parameter widgets.
struct ReverbSectionReference : Reference::Zero {
    static constexpr Color backgroundColor { 0xff, 0xff, 0xff, 0.05f };
    static constexpr uint borderRadius = 8;
    static constexpr uint margin = kSectionMargin;
    static constexpr uint padding = kControlPadding;
    static constexpr uint width = 0;
    static constexpr uint height = margin * 2 + Reference::Widgets::NumberBox<>::height;
};

template<Orientation orientation, const Color& accent, FaustParameterIndex... parameters>
class ReverbControls final : public TransparentPaddedContainerWidget<orientation, kControlPadding>
{
    using BaseWidget = TransparentPaddedContainerWidget<orientation, kControlPadding>;

    const std::array<std::shared_ptr<LabWidget>, sizeof...(parameters)> fControls {
        this->template addWidget<NumberBoxWidget<accent>, orientation == kHorizontal ? Expanding : Fixed>(parameters)...,
    };

public:
    explicit ReverbControls(LabWidget* const parent)
        : BaseWidget(parent) {}
};

template<class Controls, class R = ReverbSectionReference>
class ReverbSection final : public ReferenceContainerWidget<R, kVertical>
{
    using BaseWidget = ReferenceContainerWidget<R, kVertical>;

    std::shared_ptr<Controls> fControls = this->template addWidget<Controls, Expanding>();

public:
    explicit ReverbSection(LabWidget* const parent)
        : BaseWidget(parent) {}
};

using InputControls = ReverbControls<kHorizontal, kInputColor,
    kFaustParameterDeess_amount, kFaustParameterLowcut, kFaustParameterHighcut,
    kFaustParameterPredelay, kFaustParameterDim>;

using EarlyControls = ReverbControls<kHorizontal, kEarlyColor,
    kFaustParameterEr_size, kFaustParameterEr_diffusion,
    kFaustParameterEr_damping, kFaustParameterEr_spread, kFaustParameterEr_level>;

using TailTimeControls = ReverbControls<kHorizontal, kTailColor,
    kFaustParameterDecay, kFaustParameterTail_size,
    kFaustParameterTail_shape, kFaustParameterTail_spread,
    kFaustParameterDensity, kFaustParameterTail_level>;

using TailToneControls = ReverbControls<kHorizontal, kTailColor,
    kFaustParameterBass_mult, kFaustParameterBass_freq, kFaustParameterHf_damp,
    kFaustParameterMod_rate, kFaustParameterMod_depth>;

struct ReverbTailReference : ReverbSectionReference {
    static constexpr uint height = ReverbSectionReference::height
        + kControlPadding + Reference::Widgets::NumberBox<>::height;
};

class TailControls final : public TransparentPaddedContainerWidget<kVertical, kControlPadding>
{
    using BaseWidget = TransparentPaddedContainerWidget<kVertical, kControlPadding>;

    const std::array<std::shared_ptr<LabWidget>, 2> fRows {
        addWidget<TailTimeControls, Expanding>(),
        addWidget<TailToneControls, Expanding>(),
    };

public:
    explicit TailControls(LabWidget* const parent)
        : BaseWidget(parent) {}
};

using OutputControls = ReverbControls<kHorizontal, kOutputColor,
    kFaustParameterDuck_amount, kFaustParameterStereo_width, kFaustParameterDrywet>;

struct ReverbIOReference : ReverbSectionReference {
    static constexpr uint width = 0;
    static constexpr uint height = margin * 2 + Reference::Widgets::NumberBox<>::height;
};

template<class Controls, class R = ReverbIOReference>
class ReverbIOGroup final : public ReferenceContainerWidget<R, kHorizontal>
{
    using BaseWidget = ReferenceContainerWidget<R, kHorizontal>;

    std::shared_ptr<Controls> fControls = this->template addWidget<Controls, Expanding>();

public:
    explicit ReverbIOGroup(LabWidget* const parent)
        : BaseWidget(parent) {}
};

struct ReverbInputReference : ReverbIOReference {
    // Give the five input controls more room than the three output controls.
    static constexpr uint width = 490;
};

struct ReverbIORowReference : Reference::Zero {
    static constexpr uint padding = 8;
    static constexpr uint height = ReverbIOReference::height;
};

class ReverbIORow final : public ReferenceContainerWidget<ReverbIORowReference, kHorizontal>
{
    using BaseWidget = ReferenceContainerWidget<ReverbIORowReference, kHorizontal>;

    const std::array<std::shared_ptr<LabWidget>, 2> fGroups {
        addWidget<ReverbIOGroup<InputControls, ReverbInputReference>>(),
        addWidget<ReverbIOGroup<OutputControls>, Expanding>(),
    };

public:
    explicit ReverbIORow(LabWidget* const parent)
        : BaseWidget(parent) {}
};

struct ReverbPageReference : Reference::Zero {
    static constexpr uint margin = 0;
    static constexpr uint padding = 8;
};

// Inset the control frames without reducing the visualization's width.
template<class Content, uint height>
class ReverbInsetRow final : public TransparentSizedContainerWidget<kHorizontal, height>
{
    using BaseWidget = TransparentSizedContainerWidget<kHorizontal, height>;
    using SideMargin = TransparentSizedContainerWidget<kVertical, 12>;

    const std::array<std::shared_ptr<LabWidget>, 3> fWidgets {
        this->template addWidget<SideMargin>(),
        this->template addWidget<Content, Expanding>(),
        this->template addWidget<SideMargin>(),
    };

public:
    explicit ReverbInsetRow(LabWidget* const parent)
        : BaseWidget(parent) {}
};

class ReverbPage final : public ReferenceContainerWidget<ReverbPageReference, kVertical>
{
    using BaseWidget = ReferenceContainerWidget<ReverbPageReference, kVertical>;
    // The layout adds its 8 px gap next to each spacer: 4 + 8 = 12 px.
    using EdgeMargin = TransparentSizedContainerWidget<kHorizontal, 12 - ReverbPageReference::padding>;

    const std::array<std::shared_ptr<LabWidget>, 6> fWidgets {
        addWidget<EdgeMargin>(),
        addWidget<ReverbInsetRow<ReverbSection<EarlyControls>, ReverbSectionReference::height>>(),
        addWidget<ReverbRoomWidget, Expanding>(),
        addWidget<ReverbInsetRow<ReverbSection<TailControls, ReverbTailReference>, ReverbTailReference::height>>(),
        addWidget<ReverbInsetRow<ReverbIORow, ReverbIORowReference::height>>(),
        addWidget<EdgeMargin>(),
    };

public:
    explicit ReverbPage(LabWidget* const parent)
        : BaseWidget(parent) {}
};

// There are no Easy macro controls in reverb.dsp yet; expose the arranged
// parameters on both pages instead of the generic preset cards.
using ReverbMainArea = MainAreaContainerWidget<StageWidget<ReverbPage, ReverbPage>>;
using ReverbRootWidget = RootWidget<TopBar, ReverbMainArea>;

} /* namespace LibreAudio */

START_NAMESPACE_DISTRHO

UI* createUI()
{
    return new LibreAudio::UI<LibreAudio::ReverbRootWidget>();
}

END_NAMESPACE_DISTRHO
