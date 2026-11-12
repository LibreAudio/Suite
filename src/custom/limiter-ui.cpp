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

using LimiterGroupsContainerWidget = StaticContainerSizeHintPairWidget<
    Reference::Transparent<Reference::Margin<0, Reference::Widgets::BoxFrame<1>>>,
    Horizontal,
    SizeHintWidgetPair<
        Fixed,
        StaticAutoSizedContainerWidget<
            Reference::Widgets::BoxFrame<1>,
            Vertical,
            Expanding,
            CheckBoxWidget<kFaustParameterTrue_peak>,
            CheckBoxWidget<kFaustParameterUnity_gain>
        >
    >,
    SizeHintWidgetPair<
        Expanding,
        StaticContainerWidget<
            Reference::Widgets::BoxFrame<1>,
            Horizontal,
            Expanding,
            // NumberBoxWithMeterWidget<kFaustParameterThreshold, kFaustParameterGr>,
            NumberBoxWidget<kFaustParameterThreshold>,
            NumberBoxWidget<kFaustParameterCeiling>
        >
    >,
    SizeHintWidgetPair<
        Expanding,
        StaticContainerWidget<
            Reference::Widgets::BoxFrame<1>,
            Horizontal,
            Expanding,
            NumberBoxWidget<kFaustParameterAttack>,
            NumberBoxWidget<kFaustParameterAttack_shape>
        >
    >,
    SizeHintWidgetPair<
        Expanding,
        StaticContainerWidget<
            Reference::Widgets::BoxFrame<1>,
            Horizontal,
            Expanding,
            NumberBoxWidget<kFaustParameterRelease>,
            NumberBoxWidget<kFaustParameterAuto_release>
        >
    >,
    SizeHintWidgetPair<
        Fixed,
        ComboBoxGroupWidget<
            Reference::Widgets::BoxFrame<1>,
            kFaustParameterDither,
            kFaustParameterNoise_shaping
        >
    >
>;

// --------------------------------------------------------------------------------------------------------------------

struct LimiterEasyExpertPageReference : Reference::Zero {
    static constexpr const uint margin = Reference::Common::margin;
    static constexpr const uint padding = Reference::Common::padding;
};

using LimterExpertPageWidget = StaticBottomAlignedContainerWidget<
    LimiterEasyExpertPageReference,
    LimiterGroupsContainerWidget
>;

// --------------------------------------------------------------------------------------------------------------------

class LimiterCentralAreaWidget final : public CentralAreaWidget<EasyPageWidget, LimterExpertPageWidget>,
                                       private ButtonEventHandler::Callback
{
    using BaseWidget = CentralAreaWidget<EasyPageWidget, LimterExpertPageWidget>;
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
