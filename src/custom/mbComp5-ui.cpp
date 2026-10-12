// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/containers/container.hpp"
#include "ui/containers/main-area.hpp"
#include "ui/containers/top-bar.hpp"
#include "ui/containers/ui.hpp"
#include "ui/widgets/number-box.hpp"
#include "ui/widgets/threshold-meter.hpp"
#include "ui/_lab/text.hpp"

#include "mbComp5-ui.hpp"

#include "LibreAudioParameters.hpp"

// --------------------------------------------------------------------------------------------------------------------

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

static constexpr const uint kCommonMargin = 8;
static constexpr const uint kCommonPadding = 6;

static constexpr const uint kHeaderHeight = 20;
static constexpr const uint kBodyHeight = Reference::Widgets::NumberBox<>::height * 5 + kCommonPadding * 5;

struct MbComp5BandGroupReference : Reference::Zero {
    static constexpr const Color backgroundColor { 0xff, 0xff, 0xff, 0.05f };
    static constexpr const uint borderRadius = 8;
    static constexpr const uint margin = kCommonMargin;
    static constexpr const uint padding = kCommonPadding;
    static constexpr const uint height = kCommonMargin * 2 + kHeaderHeight + kBodyHeight;
};

inline constexpr const char* bandLabels[] = {
    "LOW",
    "LOW MID",
    "MID",
    "HIGH MID",
    "HIGH"
};

template<uint8_t band>
class MbComp5BandGroupWidget final : public ReferenceContainerWidget<MbComp5BandGroupReference, kVertical>
{
    using BaseWidget = ReferenceContainerWidget<MbComp5BandGroupReference, kVertical>;

    static constexpr const uint32_t kNumParamsPerBand = kFaustParameterBand2_threshold - kFaustParameterBand1_threshold;

    static constexpr FaustParameterIndex getParameterIndexForBand(const FaustParameterIndex index)
    {
        return static_cast<FaustParameterIndex>(index + kNumParamsPerBand * band);
    }

    static constexpr FaustParameterIndex getLevelParameterIndex(const bool right)
    {
        return static_cast<FaustParameterIndex>(
            (right ? kFaustParameterLevel_band1_ch1 : kFaustParameterLevel_band1_ch0) + band);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // header: text + buttons

    class SoloMuteGroupWidget : public ButtonGroupWidget
    {
        using SoloButtonWidget = ImageButtonWidget<kCornerLeft, IMAGES_UNDO_PNG_DATA, IMAGES_UNDO_PNG_LEN>;
        using MuteButtonWidget = ImageButtonWidget<kCornerRight, IMAGES_POWER_PNG_DATA, IMAGES_POWER_PNG_LEN>;

    public:
        explicit SoloMuteGroupWidget(LabWidget* const parent)
            : ButtonGroupWidget(parent)
        {
            addButton<SoloButtonWidget>(getParameterIndexForBand(kFaustParameterBand1_listen));
            addButton<MuteButtonWidget>(getParameterIndexForBand(kFaustParameterBand1_bypass));
            done();
        }
    };

    class HeaderWidget final : public TransparentSizedContainerWidget<kHorizontal, kHeaderHeight>
    {
        using BaseWidget = TransparentSizedContainerWidget<kHorizontal, kHeaderHeight>;

        struct TextReference : Reference::Zero {
            static constexpr const Color color = Reference::kAccentColors[band + 1];
            static constexpr const float fontSize = 15;
        };

        const std::array<std::shared_ptr<LabWidget>, 2> widgets {
            addWidget<ReferenceTextWidget<TextReference>, Expanding>(bandLabels[band]),
            addWidget<SoloMuteGroupWidget>(),
        };

    public:
        explicit HeaderWidget(LabWidget* const parent)
            : BaseWidget(parent) {}
    };

    // ----------------------------------------------------------------------------------------------------------------
    // body: number-boxes stack + meter

    class BodyWidget final : public TransparentPaddedContainerWidget<kHorizontal, kCommonPadding>
    {
        using BaseWidget = TransparentPaddedContainerWidget<kHorizontal, kCommonPadding>;

        static constexpr const Color kMeterColor = Reference::kAccentColors[band + 1];
        using MeterWidget = ThresholdMeterWidget<Reference::Widgets::ThresholdMeter<kMeterColor>>;

        const std::array<std::shared_ptr<LabWidget>, 2> widgets {
            addWidget<NumberBoxStackWidget, Expanding>(),
            addWidget<MeterWidget>(getParameterIndexForBand(kFaustParameterBand1_threshold),
                                   getLevelParameterIndex(false),
                                   getLevelParameterIndex(true)),
        };

    public:
        explicit BodyWidget(LabWidget* const parent)
            : BaseWidget(parent) {}
    };

    // ----------------------------------------------------------------------------------------------------------------
    // stack of number-boxes

    static constexpr const Color kNumberBoxColor = Reference::kAccentColors[band + 1];

    class NumberBoxStackWidget final : public TransparentPaddedContainerWidget<kVertical, kCommonPadding>
    {
        using BaseWidget = TransparentPaddedContainerWidget<kVertical, kCommonPadding>;

        const std::array<std::shared_ptr<LabWidget>, 5> widgets {
            addWidget<NumberBoxWidget<kNumberBoxColor>>(getParameterIndexForBand(kFaustParameterBand1_threshold)),
            addWidget<NumberBoxWidget<kNumberBoxColor>>(getParameterIndexForBand(kFaustParameterBand1_ratio)),
            addWidget<NumberBoxWidget<kNumberBoxColor>>(getParameterIndexForBand(kFaustParameterBand1_attack)),
            addWidget<NumberBoxWidget<kNumberBoxColor>>(getParameterIndexForBand(kFaustParameterBand1_release)),
            addWidget<NumberBoxWidget<kNumberBoxColor>>(getParameterIndexForBand(kFaustParameterBand1_makeup)),
        };

    public:
        explicit NumberBoxStackWidget(LabWidget* const parent)
            : BaseWidget(parent) {}
    };

    // ----------------------------------------------------------------------------------------------------------------

    const std::array<std::shared_ptr<LabWidget>, 2> widgets {
        addWidget<HeaderWidget>(),
        addWidget<BodyWidget, Expanding>(),
    };

    // ----------------------------------------------------------------------------------------------------------------

public:
    explicit MbComp5BandGroupWidget(LabWidget* const parent)
        : BaseWidget(parent) {}
};

// --------------------------------------------------------------------------------------------------------------------

struct MbComp5BandsReference : Reference::Zero {
    static constexpr const uint padding = kCommonPadding;
    static constexpr const uint height = MbComp5BandGroupReference::height + margin * 2;
};

class MbComp5BandsWidget final : public ReferenceContainerWidget<MbComp5BandsReference, kHorizontal>
{
    using BaseWidget = ReferenceContainerWidget<MbComp5BandsReference, kHorizontal>;

    const std::array<std::shared_ptr<LabWidget>, 5> widgets {
        addWidget<MbComp5BandGroupWidget<__COUNTER__>, Expanding>(),
        addWidget<MbComp5BandGroupWidget<__COUNTER__>, Expanding>(),
        addWidget<MbComp5BandGroupWidget<__COUNTER__>, Expanding>(),
        addWidget<MbComp5BandGroupWidget<__COUNTER__>, Expanding>(),
        addWidget<MbComp5BandGroupWidget<__COUNTER__>, Expanding>(),
    };

public:
    explicit MbComp5BandsWidget(LabWidget* const parent)
        : BaseWidget(parent) {}
};

// --------------------------------------------------------------------------------------------------------------------

struct MbComp5NonBandParamsGroupReference : Reference::Zero {
    static constexpr const Color backgroundColor { 0xff, 0xff, 0xff, .05f };
    static constexpr const uint borderRadius = 10;
    static constexpr const uint margin = kCommonMargin;
    static constexpr const uint padding = kCommonPadding;
    static constexpr const uint height = Reference::Widgets::NumberBox<>::height + (border + margin) * 2;
};

class MbComp5NonBandParamsGroupWidget final : public ReferenceContainerWidget<MbComp5NonBandParamsGroupReference, kHorizontal>
{
    using BaseWidget = ReferenceContainerWidget<MbComp5NonBandParamsGroupReference, kHorizontal>;

    const std::array<std::shared_ptr<LabWidget>, 9> widgets {
        addWidget<NumberBoxWidget<>, Expanding>(kFaustParameterSlope),
        addWidget<NumberBoxWidget<>, Expanding>(kFaustParameterKnee),
        addWidget<NumberBoxWidget<>, Expanding>(kFaustParameterRange),
        addWidget<NumberBoxWidget<>, Expanding>(kFaustParameterRms_time),
        addWidget<NumberBoxWidget<>, Expanding>(kFaustParameterShape),
        addWidget<NumberBoxWidget<>, Expanding>(kFaustParameterChan_link),
        addWidget<NumberBoxWidget<>, Expanding>(kFaustParameterBand_link),
        addWidget<NumberBoxWidget<>, Expanding>(kFaustParameterLookahead),
        addWidget<NumberBoxWidget<>, Expanding>(kFaustParameterDrywet),
    };

public:
    explicit MbComp5NonBandParamsGroupWidget(LabWidget* const parent)
        : BaseWidget(parent) {}
};

// --------------------------------------------------------------------------------------------------------------------

struct MbComp5ExpertPageReference : Reference::Zero {
    static constexpr const uint margin = 12;
    static constexpr const uint padding = 8;
};

class MbComp5ExpertPageWidget final : public ReferenceContainerWidget<MbComp5ExpertPageReference, kVertical>
{
    using BaseWidget = ReferenceContainerWidget<MbComp5ExpertPageReference, kVertical>;

    const std::array<std::shared_ptr<LabWidget>, 3> widgets {
        addSpacer(),
        addWidget<MbComp5BandsWidget>(),
        addWidget<MbComp5NonBandParamsGroupWidget>(),
    };

public:
    explicit MbComp5ExpertPageWidget(LabWidget* const parent)
        : BaseWidget(parent) {}
};

// --------------------------------------------------------------------------------------------------------------------

class MbComp5MainArea : public MainAreaContainerWidget<StageWidget<EasyStageWidget, MbComp5ExpertPageWidget>>
{
    using BaseWidget = MainAreaContainerWidget<StageWidget<EasyStageWidget, MbComp5ExpertPageWidget>>;

    std::shared_ptr<MbComp5GraphWidget> graph { new MbComp5GraphWidget(this) };

public:
    explicit MbComp5MainArea(LabTopLevelWidget* const parent)
        : BaseWidget(parent)
    {
        graph->toBottom();
    }

private:
    // void onResize(const ResizeEvent& ev) final
    // {
    //     BaseWidget::onResize(ev);
    void updateSize(const bool updateChildren) final
    {
        BaseWidget::updateSize(updateChildren);

        const Point<int> pos = getMainAreaAbsolutePos();
        const Size<uint> size = getMainAreaSize();
        // const float borderRadius = getMainAreaBorderRadius();

        graph->setAbsolutePos(pos);
        graph->setSize(size);
        // graph->setBorderRadius(borderRadius);
    }
};

// --------------------------------------------------------------------------------------------------------------------

// using MbComp5MainArea = MainAreaContainerWidget<StageWidget<EasyStageWidget, MbComp5ExpertPageWidget>>;
using MbComp5RootWidget = RootWidget<TopBar, MbComp5MainArea>;

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

UI* createUI()
{
    return new LibreAudio::UI<LibreAudio::MbComp5RootWidget>();
}

END_NAMESPACE_DISTRHO
