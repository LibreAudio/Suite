// Libre Audio Suite
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/containers/container.hpp"
#include "ui/containers/main-area.hpp"
#include "ui/containers/top-bar.hpp"
#include "ui/containers/ui.hpp"
#include "ui/widgets/number-box.hpp"
#include "ui/_lab/text.hpp"

#include "LibreAudioParameters.hpp"
#include "reverb/room.hpp"

namespace LibreAudio {

static constexpr Color kInputColor = Reference::kAccentColors[1];
static constexpr Color kOutputColor = Reference::kAccentColors[4];
static constexpr Color kEarlyColor = Reference::kAccentColors[2];
static constexpr Color kTailColor = Reference::kAccentColors[3];
static constexpr Color kFiltersColor = Reference::kAccentColors[0];
static constexpr Color kDynamicsColor = Reference::kAccentColors[5];

static constexpr uint kSectionMargin = 8;
static constexpr uint kControlPadding = 4;
static constexpr uint kHeaderHeight = 20;

// Match mbComp5's panels, spacing, and parameter widgets.
struct ReverbSectionReference : Reference::Zero {
    static constexpr Color backgroundColor { 0x30, 0x32, 0x3c, 0.7f };
    static constexpr uint border = 1;
    static constexpr uint borderRadius = 8;
    static constexpr uint margin = kSectionMargin;
    static constexpr uint padding = kControlPadding;
    static constexpr uint width = 0;
    static constexpr uint height = (border + margin) * 2 + kHeaderHeight + padding + Reference::Widgets::NumberBox<>::height;
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

template<class Controls, class R = ReverbSectionReference, const Color& accent = Reference::Colors::acc1>
class ReverbSection final : public ReferenceContainerWidget<R, kVertical>
{
    using BaseWidget = ReferenceContainerWidget<R, kVertical>;
    using Header = TransparentSizedContainerWidget<kHorizontal, kHeaderHeight>;
    struct HeaderReference : Reference::Zero {
        static constexpr Color color = accent;
        static constexpr float fontSize = 12;
        static constexpr float letterSpacing = 0.4f;
    };

    std::shared_ptr<Header> fHeader = this->template addWidget<Header>();
    std::shared_ptr<LabWidget> fTitle;
    std::shared_ptr<Controls> fControls = this->template addWidget<Controls, Expanding>();

public:
    explicit ReverbSection(LabWidget* const parent, const char* title)
        : BaseWidget(parent),
          fTitle(new ReferenceTextWidget<HeaderReference>(fHeader.get(), title))
    {
        fHeader->widgets.push_back({ fTitle.get(), Expanding });
    }

private:
    void onNanoDisplay() override
    {
        this->template drawReferenceBackground<R>();
        const float w = this->getWidth(), h = this->getHeight();
        const float px = this->fScaleFactor;
        const float radius = R::borderRadius * px;

        // A soft surface sheen and a light upper rim give the panel depth.
        this->beginPath();
        this->roundedRect(0, 0, w, h, radius);
        this->fillPaint(this->linearGradient(0, 0, 0, h,
            Color(1.f, 1.f, 1.f, 0.03f), Color(0.f, 0.f, 0.f, 0.08f)));
        this->fill();

        this->beginPath();
        this->roundedRect(px * 0.5f, px * 0.5f, w - px, h - px, radius - px * 0.5f);
        this->strokePaint(this->linearGradient(0, 0, 0, h,
            Color(0.82f, 0.86f, 0.96f, 0.20f), Color(0.f, 0.f, 0.f, 0.25f)));
        this->strokeWidth(px * 0.8f);
        this->stroke();

        this->save();
        this->scissor(0, 0, w, 16.f * px);
        this->beginPath();
        this->roundedRect(1.5f * px, 1.5f * px, w - 3.f * px, h - 3.f * px, radius - px);
        this->strokePaint(this->linearGradient(0, 0, 0, 16.f * px,
            Color(accent.red, accent.green, accent.blue, 0.08f), Reference::Colors::transparent));
        this->strokeWidth(px * 0.8f);
        this->stroke();
        this->restore();
    }
};

using InputControls = ReverbControls<kVertical, kInputColor,
    kFaustParameterPredelay, kFaustParameterDim>;

using FilterControls = ReverbControls<kVertical, kFiltersColor,
    kFaustParameterLowcut, kFaustParameterHighcut>;

using DynamicsControls = ReverbControls<kVertical, kDynamicsColor,
    kFaustParameterDeess_amount, kFaustParameterDuck_amount>;

using EarlyControls = ReverbControls<kHorizontal, kEarlyColor,
    kFaustParameterEr_size, kFaustParameterEr_diffusion,
    kFaustParameterEr_damping, kFaustParameterEr_spread, kFaustParameterTail_feed>;

using TailTimeControls = ReverbControls<kHorizontal, kTailColor,
    kFaustParameterDecay, kFaustParameterTail_size,
    kFaustParameterTail_shape, kFaustParameterTail_spread, kFaustParameterDensity>;

using TailToneControls = ReverbControls<kHorizontal, kTailColor,
    kFaustParameterBass_mult, kFaustParameterBass_freq, kFaustParameterHf_damp,
    kFaustParameterMod_rate, kFaustParameterMod_depth>;

struct ReverbTwoRowReference : ReverbSectionReference {
    static constexpr uint height = ReverbSectionReference::height
        + Reference::Widgets::NumberBox<>::height + kControlPadding;
};

template<class Top, class Bottom>
class ReverbTwoRows final : public TransparentPaddedContainerWidget<kVertical, kControlPadding>
{
    using BaseWidget = TransparentPaddedContainerWidget<kVertical, kControlPadding>;
    const std::array<std::shared_ptr<LabWidget>, 2> fRows {
        this->template addWidget<Top, Expanding>(),
        this->template addWidget<Bottom, Expanding>(),
    };

public:
    explicit ReverbTwoRows(LabWidget* const parent)
        : BaseWidget(parent) {}
};

using TailControls = ReverbTwoRows<TailTimeControls, TailToneControls>;
using MixerControls = ReverbControls<kVertical, kOutputColor,
    kFaustParameterEr_level, kFaustParameterTail_level>;
using OutputControls = ReverbControls<kVertical, kOutputColor,
    kFaustParameterStereo_width, kFaustParameterDrywet>;

class ReverbBottomRow final : public TransparentPaddedContainerWidget<kHorizontal, 8>
{
    using BaseWidget = TransparentPaddedContainerWidget<kHorizontal, 8>;
    const std::array<std::shared_ptr<LabWidget>, 5> fGroups {
        addWidget<ReverbSection<InputControls, ReverbTwoRowReference, kInputColor>, Expanding>("INPUT"),
        addWidget<ReverbSection<FilterControls, ReverbTwoRowReference, kFiltersColor>, Expanding>("FILTERS"),
        addWidget<ReverbSection<DynamicsControls, ReverbTwoRowReference, kDynamicsColor>, Expanding>("DYNAMICS"),
        addWidget<ReverbSection<MixerControls, ReverbTwoRowReference, kOutputColor>, Expanding>("MIXER"),
        addWidget<ReverbSection<OutputControls, ReverbTwoRowReference, kOutputColor>, Expanding>("OUTPUT"),
    };

public:
    explicit ReverbBottomRow(LabWidget* const parent)
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

    const std::array<std::shared_ptr<LabWidget>, 3> fWidgets;

public:
    template<typename... Args>
    explicit ReverbInsetRow(LabWidget* const parent, Args... args)
        : BaseWidget(parent),
          fWidgets {
              this->template addWidget<SideMargin>(),
              this->template addWidget<Content, Expanding>(args...),
              this->template addWidget<SideMargin>(),
          } {}
};

class ReverbPage final : public ReferenceContainerWidget<ReverbPageReference, kVertical>
{
    using BaseWidget = ReferenceContainerWidget<ReverbPageReference, kVertical>;
    using EdgeMargin = TransparentSizedContainerWidget<kHorizontal, 12 - ReverbPageReference::padding>;

    std::shared_ptr<ReverbRoomWidget> fRoom { new ReverbRoomWidget(this) };
    const std::array<std::shared_ptr<LabWidget>, 6> fWidgets {
        addWidget<EdgeMargin>(),
        addWidget<ReverbInsetRow<ReverbSection<EarlyControls, ReverbSectionReference, kEarlyColor>, ReverbSectionReference::height>>("EARLY"),
        addWidget<ReverbInsetRow<ReverbSection<TailControls, ReverbTwoRowReference, kTailColor>, ReverbTwoRowReference::height>>("TAIL"),
        addSpacer(),
        addWidget<ReverbInsetRow<ReverbBottomRow, ReverbTwoRowReference::height>>(),
        addWidget<EdgeMargin>(),
    };

public:
    explicit ReverbPage(LabWidget* const parent)
        : BaseWidget(parent) {}

private:
    void updateSize(const bool updateChildren) override
    {
        BaseWidget::updateSize(updateChildren);
        fRoom->setAbsolutePos(getAbsolutePos());
        fRoom->setSize(getSize());

        const auto& display = fWidgets[3];
        fRoom->setSceneArea(Point<int>(0, display->getAbsoluteY() - getAbsoluteY()),
                            Size<uint>(getWidth(), display->getHeight()));
    }
};

// Share Chorus's large Easy knob and four placeholder preset cards.
using ReverbMainArea = MainAreaContainerWidget<StageWidget<EasyStageWidget, ReverbPage>>;
using ReverbRootWidget = RootWidget<TopBar, ReverbMainArea>;

} /* namespace LibreAudio */

START_NAMESPACE_DISTRHO

UI* createUI()
{
    return new LibreAudio::UI<LibreAudio::ReverbRootWidget>();
}

END_NAMESPACE_DISTRHO
