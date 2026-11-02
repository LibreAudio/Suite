// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

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

static constexpr const uint kCommonMargin = 8;
static constexpr const uint kCommonPadding = 6;
static constexpr const uint kNumberBoxHeight = 50;
static constexpr const uint kGroupsWidth[5] = { 115, 0, 0, 0, 140 };

template<const Color& kAccentColor>
using LimiterCheckBoxWidget = CheckBoxWidget<kAccentColor, (kNumberBoxHeight - kCommonPadding) / 2>;

// template<const Color& kAccentColor>
using LimiterComboBoxWidget = ComboBoxWidget<Reference::Colors::acc2, (kNumberBoxHeight - kCommonPadding) / 2>;

template<const Color& kAccentColor>
using LimiterNumberBoxWidget = NumberBoxWidget<kAccentColor, kNumberBoxHeight>;

// --------------------------------------------------------------------------------------------------------------------

namespace widgets {
    using TruePeak = ParameterWidgetPair<kFaustParameterTrue_peak, LimiterCheckBoxWidget<Reference::Colors::acc6>>;
    using UnityGain = ParameterWidgetPair<kFaustParameterUnity_gain, LimiterCheckBoxWidget<Reference::Colors::acc4>>;

    using Threshold = ParameterWidgetPair<kFaustParameterThreshold, LimiterNumberBoxWidget<Reference::Colors::acc1>>;
    using Ceiling = ParameterWidgetPair<kFaustParameterCeiling, LimiterNumberBoxWidget<Reference::Colors::acc6>>;

    using Attack = ParameterWidgetPair<kFaustParameterAttack, LimiterNumberBoxWidget<Reference::Colors::acc5>>;
    using AttackShape = ParameterWidgetPair<kFaustParameterAttack_shape, LimiterNumberBoxWidget<Reference::Colors::acc5>>;

    using Release = ParameterWidgetPair<kFaustParameterRelease, LimiterNumberBoxWidget<Reference::Colors::acc3>>;
    using AutoRelease = ParameterWidgetPair<kFaustParameterAuto_release, LimiterNumberBoxWidget<Reference::Colors::acc3>>;
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
    SizeHintWidgetPair<Fixed, ComboBoxGroupWidget<LimiterGroupReference, LimiterComboBoxWidget, kFaustParameterDither, kFaustParameterNoise_shaping>>
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

class LimiterStageWidget final : public StageWidget<LimterEasyPageWidget, LimterExpertPageWidget>,
                                 private ButtonEventHandler::Callback
{
    using BaseWidget = StageWidget<LimterEasyPageWidget, LimterExpertPageWidget>;
    using ButtonWidget = TextButtonWidget<kCornerNone, Reference::Height<30, Reference::Widgets::Button>>;

    static constexpr const FaustParameter& kParamThreshold = kFaustParameters[kFaustParameterThreshold];

    static constexpr const float kPaddedMinimumScale = -48.f;

    const std::shared_ptr<GridWidget> fGrid {
        addGraphWidget<GridWidget>(kParamThreshold.min - kParamThreshold.min / kPaddedMinimumScale,
                                   kParamThreshold.min,
                                   kParamThreshold.max,
                                   6.f,
                                   kParamThreshold.unit)
    };

    const std::shared_ptr<FloatingLineSliderWidget> fSlider {
        addGraphWidget<FloatingLineSliderWidget>(kFaustParameterThreshold, kParamThreshold.min / kPaddedMinimumScale)
    };

    const std::shared_ptr<ButtonWidget> fRangeButton {
        new ButtonWidget(this, 0, "Range Test Button")
    };

public:
    LimiterStageWidget(LabWidget* const parent)
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

        return Size<uint>(getWidth(), getHeight() - space);
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
        fGrid->setRange(min, start, kParamThreshold.max);
        fSlider->setRange(min, start, kParamThreshold.max);
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

using LimiterMainArea = MainAreaContainerWidget<LimiterStageWidget>;
using LimiterRootWidget = RootWidget<TopBar, LimiterMainArea>;

// --------------------------------------------------------------------------------------------------------------------

class LimiterUI : public LibreAudioBaseUI
{
    // using ShaderGainReductionW = BotShaderWidget<SHADERS_CURVE_LIMITER_FRAG_DATA, SHADERS_CURVE_LIMITER_FRAG_LEN, kNumSamplePointsForWaveform>;
    // using ShaderWaveformW = BotShaderWidget<SHADERS_WAVEFORM_SCROLL_FRAG_DATA, SHADERS_WAVEFORM_SCROLL_FRAG_LEN, kNumSamplePointsForWaveform>;
    using ShaderWaveformW = WaveformWidget;

    static constexpr const float kDisplayDbMax = 0.f;
    static constexpr const float kDisplayDbFloor = -24.f;
    static constexpr const float kDisplayDbStep = 6.f;

public:
    LimiterUI()
    {
        const std::list<std::shared_ptr<ShaderWidgetInterface>> shaders {
            std::make_shared<ShaderBackgroundWidget>(this, this),
            std::make_shared<BotShaderWidget<SHADER_CLOUDSTARFIELD>>(this, this),
            makeShaderWaveform(),
            // makeShaderGainReduction(),
        };

        createRootWidget<LimiterRootWidget>();

        static_cast<LimiterRootWidget*>(fRootWidget.get())->enableShaders(shaders);

        // fShaderGainReduction->setCustomUniform("uHead", [this] { return waveformHead(); });
        // fShaderGainReduction->setCustomUniform("uDbMin", [this] { return displayDbMin(); });
        // fShaderGainReduction->setCustomUniform("uDbMax", [] { return kDisplayDbMax; });
        //
        // fShaderWaveform->setCustomUniform("uHead", [this] { return waveformHead(); });
        // fShaderWaveform->setCustomUniform("uDbMin", [this] { return displayDbMin(); });
        // fShaderWaveform->setCustomUniform("uDbMax", [] { return kDisplayDbMax; });
    }

protected:
    // std::shared_ptr<ShaderGainReductionW> fShaderGainReduction;
    std::shared_ptr<ShaderWaveformW> fShaderWaveform;

    void audioPeaksReceived(const LibreAudioWaveformIPC<LIBREAUDIO_WANT_GRAPH_IO_COUNT>::ValueType& value) final
    {
        fShaderWaveform->push(value.ptr[0]);
        // fShaderGainReduction->push(-displayDbMin() * (1.f - ((value.ptr[1] + 24.f) / 24.f))); // FIXME ugly stuff
        // fPointsPushed += 1.0;
    }

private:
    // std::shared_ptr<ShaderGainReductionW> makeShaderGainReduction()
    // {
    //     fShaderGainReduction.reset(new ShaderGainReductionW(this, this));
    //     return fShaderGainReduction;
    // }

    std::shared_ptr<ShaderWaveformW> makeShaderWaveform()
    {
        fShaderWaveform.reset(new ShaderWaveformW(this, this));
        return fShaderWaveform;
    }

    // points pushed so far, and the point the waveform is drawn up to (fractional, moving at kWaveformRate)
    // double fPointsPushed = 0;
    // double fPointsShown = 0;
    // double fLastTime = 0;
    // float fHead = 0;

    // dB at the bottom edge of a display this tall
    // float displayDbMin() noexcept
    // {
    //     LimiterRootWidget* const root = static_cast<LimiterRootWidget*>(fRootWidget.get());
    //     LimiterMainArea* const mainArea = root->getMainAreaWidget().get();
    //     LimiterStageWidget* const stage = mainArea->getStageWidget().get();
    //
    //     const float height = mainArea->getMainAreaSize().getHeight();
    //     const float floorY = std::max(1.f, height - stage->getGraphSize().getHeight());
    //     return kDisplayDbMax + (kDisplayDbFloor - kDisplayDbMax) * height / floorY;
    // }

    // Peaks arrive in bursts, so the shown point runs at the steady rate and is eased towards kWaveformLag behind
    // the newest point, never past it. Returns its ring position: push number n lands at (size - 2 + n) mod size.
    // Both shaders ask for it each frame; the second ask within a millisecond gets the same head.
    // float waveformHead()
    // {
    //     static constexpr const double kWaveformSize = kNumSamplePointsForWaveform;
    //     static constexpr const double kWaveformRate = kWaveformSize / kNumSecondsForWaveform; // points per second
    //     static constexpr const double kWaveformLag = kWaveformRate * 0.05; // points shown behind the newest one
    //
    //     const double now = getApp().getTime();
    //     if (fLastTime != 0 && now - fLastTime < 0.001)
    //         return fHead;
    //
    //     const double dt = fLastTime != 0 ? std::min<double>(now - fLastTime, 0.1) : 0;
    //     fLastTime = now;
    //
    //     fPointsShown += dt * kWaveformRate;
    //
    //     const double error = fPointsPushed - kWaveformLag - fPointsShown;
    //
    //     if (std::abs(error) > kWaveformRate * 0.5)
    //         fPointsShown += error;
    //     else
    //         fPointsShown += error * std::min<double>(1.0, dt * 2);
    //
    //     fPointsShown = std::min(fPointsShown, fPointsPushed);
    //
    //     double head = std::fmod(fPointsShown - 2, kWaveformSize);
    //     if (head < 0)
    //         head += kWaveformSize;
    //     fHead = static_cast<float>(head);
    //     return fHead;
    // }
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

UI* createUI()
{
    return new LibreAudio::LimiterUI();
}

END_NAMESPACE_DISTRHO
