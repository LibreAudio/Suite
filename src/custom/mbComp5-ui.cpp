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

#include "LibreAudioParameters.hpp"

// --------------------------------------------------------------------------------------------------------------------

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

class SoloMuteGroupWidget : public ButtonGroupWidget
{
    using SoloButtonWidget = ImageButtonWidget<kCornerLeft, IMAGES_UNDO_PNG_DATA, IMAGES_UNDO_PNG_LEN>;
    using MuteButtonWidget = ImageButtonWidget<kCornerRight, IMAGES_POWER_PNG_DATA, IMAGES_POWER_PNG_LEN>;

public:
    explicit SoloMuteGroupWidget(LabWidget* const parent)
        : ButtonGroupWidget(parent)
    {
        addButton<SoloButtonWidget>(kFaustParameterBand1_listen);
        addButton<MuteButtonWidget>(kFaustParameterBand1_bypass);
        done();
    }
};

// --------------------------------------------------------------------------------------------------------------------

struct TextReference : Reference::Zero {
    static constexpr const Color color = Reference::Colors::ink2;
    static constexpr const float fontSize = 18;
    static constexpr const float letterSpacing = fontSize * 0.01;
    static constexpr const uint margin = 0;
    // static constexpr const uint height = 30;
};

// --------------------------------------------------------------------------------------------------------------------

struct MbComp5BandGroupReference : Reference::Zero {
    static constexpr const Color backgroundColor { 0xff, 0xff, 0xff, 0.05f };
    static constexpr const uint borderRadius = 8;
    static constexpr const uint height = 268;
    // static constexpr const uint width = 120;
    static constexpr const uint margin = 8;
    static constexpr const uint padding = 4;
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

    static constexpr const uint kHeaderHeight = 20;

    // ----------------------------------------------------------------------------------------------------------------
    // header: text + buttons

    class HeaderWidget final : public TransparentSizedContainerWidget<kHorizontal, kHeaderHeight>
    {
        using BaseWidget = TransparentSizedContainerWidget<kHorizontal, kHeaderHeight>;

        const std::array<std::shared_ptr<LabWidget>, 2> widgets {
            addWidget<ReferenceTextWidget<TextReference>, Expanding>(bandLabels[band - 1]),
            addWidget<SoloMuteGroupWidget>(),
        };

    public:
        explicit HeaderWidget(LabWidget* const parent)
            : BaseWidget(parent) {}
    };

    // ----------------------------------------------------------------------------------------------------------------
    // body: number-boxes stack + meter

    class BodyWidget final : public TransparentHorizontalContainerWidget
    {
        using BaseWidget = TransparentHorizontalContainerWidget;

        const std::array<std::shared_ptr<LabWidget>, 2> widgets {
            addWidget<NumberBoxStackWidget, Expanding>(),
            addWidget<ThresholdMeterWidget>(kFaustParameterBand1_threshold, kFaustParameterLevel_band1_ch0, kFaustParameterLevel_band1_ch1),
        };

    public:
        explicit BodyWidget(LabWidget* const parent)
            : BaseWidget(parent) {}
    };

    // ----------------------------------------------------------------------------------------------------------------
    // stack of number-boxes

    static constexpr const uint kNumberBoxStackPadding = 4;

    class NumberBoxStackWidget final : public TransparentPaddedContainerWidget<kVertical, kNumberBoxStackPadding>
    {
        using BaseWidget = TransparentPaddedContainerWidget<kVertical, kNumberBoxStackPadding>;

        const std::array<std::shared_ptr<LabWidget>, 5> widgets {
            addWidget<NumberBoxWidget>(kFaustParameterBand1_threshold),
            addWidget<NumberBoxWidget>(kFaustParameterBand1_ratio),
            addWidget<NumberBoxWidget>(kFaustParameterBand1_attack),
            addWidget<NumberBoxWidget>(kFaustParameterBand1_release),
            addWidget<NumberBoxWidget>(kFaustParameterBand1_makeup),
        };

    public:
        explicit NumberBoxStackWidget(LabWidget* const parent)
            : BaseWidget(parent) {}
    };

    // ----------------------------------------------------------------------------------------------------------------

    const std::array<std::shared_ptr<LabWidget>, 2> widgets {
        addWidget<HeaderWidget>(),
        addWidget<BodyWidget>(),
    };

    // ----------------------------------------------------------------------------------------------------------------

public:
    explicit MbComp5BandGroupWidget(LabWidget* const parent)
        : BaseWidget(parent) {}
};

// --------------------------------------------------------------------------------------------------------------------

struct MbComp5BandsReference : Reference::Zero {
    static constexpr const uint margin = 12;
    static constexpr const uint padding = 8;
    static constexpr const uint height = MbComp5BandGroupReference::height + margin * 2;
};

class MbComp5BandsWidget final : public ReferenceContainerWidget<MbComp5BandsReference, kHorizontal>
{
    using BaseWidget = ReferenceContainerWidget<MbComp5BandsReference, kHorizontal>;

    const std::array<std::shared_ptr<LabWidget>, 5> widgets {
        addWidget<MbComp5BandGroupWidget<1>, Expanding>(),
        addWidget<MbComp5BandGroupWidget<2>, Expanding>(),
        addWidget<MbComp5BandGroupWidget<3>, Expanding>(),
        addWidget<MbComp5BandGroupWidget<4>, Expanding>(),
        addWidget<MbComp5BandGroupWidget<5>, Expanding>(),
    };

public:
    explicit MbComp5BandsWidget(LabWidget* const parent)
        : BaseWidget(parent) {}
};

// --------------------------------------------------------------------------------------------------------------------

class MbComp5ExpertPageWidget final : public TransparentVerticalContainerWidget
{
    using BaseWidget = TransparentVerticalContainerWidget;

    const std::array<std::shared_ptr<LabWidget>, 2> widgets {
        addSpacer(),
        addWidget<MbComp5BandsWidget>(),
    };

public:
    explicit MbComp5ExpertPageWidget(LabWidget* const parent)
        : BaseWidget(parent) {}
};

// --------------------------------------------------------------------------------------------------------------------

using MbComp5MainArea = MainAreaContainerWidget<StageWidget<EasyStageWidget, MbComp5ExpertPageWidget>>;
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
