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

    struct SoloButtonReference : Reference::Zero {
        static constexpr const Color backgroundColor〡checked = Reference::kAccentColors[band + 1];
        static constexpr const Color color = Reference::Colors::ink2;
        static constexpr const Color color〡checked = Color(0x17, 0x17, 0x1b);
        static constexpr const Color color〡hover = Reference::kAccentColors[band + 1];
        static constexpr const MouseCursor cursor〡hover = kMouseCursorHand;
        static constexpr const uint borderRadius = 4;
        static constexpr const uint width = 18;
    };

    struct BypassButtonReference : SoloButtonReference {
        static constexpr const Color backgroundColor〡checked = Reference::Widgets::Button〡Bypass::backgroundColorBypass;
        static constexpr const Color color〡checked = Reference::Widgets::Button〡Bypass::colorBypass;
        static constexpr const Color color〡hover = Reference::Widgets::Button〡Bypass::colorBypass;
    };

    struct ButtonGroupReference : Reference::Zero {
        static constexpr const Color backgroundColor = { 0xff, 0xff, 0xff, 0.07f };
        static constexpr const uint borderRadius = 6;
        static constexpr const uint margin = 2;
        static constexpr const uint padding = 2;
    };

    template<class R, const uchar* imageData, uint imageDataSize>
    class SoloMuteButton : public ImageButtonWidget<kCornerBoth, imageData, imageDataSize, R>
    {
        using BaseWidget = ImageButtonWidget<kCornerBoth, imageData, imageDataSize, R>;

    public:
        explicit SoloMuteButton(LabWidget* const parent)
            : BaseWidget(parent) {}

    private:
        void onNanoDisplay() override
        {
            if (BaseWidget::isChecked())
                BaseWidget::template drawReferenceBackground<R, R::backgroundColor〡checked, kCornerBoth>();
            else
                BaseWidget::template drawReferenceBackground<R, R::backgroundColor, kCornerBoth>();

            BaseWidget::template drawReferenceBorder<R, R::borderColor>();

            if (BaseWidget::isChecked())
                BaseWidget::drawColoredImage(R::color〡checked);
            else if (BaseWidget::isHovered())
                BaseWidget::drawColoredImage(R::color〡hover);
            else
                BaseWidget::drawColoredImage(R::color);
        }
    };

    class SoloMuteGroupWidget : public ReferenceButtonGroupWidget<ButtonGroupReference>
    {
        using R = ButtonGroupReference;
        using BaseWidget = ReferenceButtonGroupWidget<R>;
        using SoloButtonWidget = SoloMuteButton<SoloButtonReference, IMAGES_SOLO_28_PNG_DATA, IMAGES_SOLO_28_PNG_LEN>;
        using MuteButtonWidget = SoloMuteButton<BypassButtonReference, IMAGES_POWER_28_PNG_DATA, IMAGES_POWER_28_PNG_LEN>;

    public:
        explicit SoloMuteGroupWidget(LabWidget* const parent)
            : BaseWidget(parent)
        {
            addButton<SoloButtonWidget>(getParameterIndexForBand(kFaustParameterBand1_listen));
            addButton<MuteButtonWidget>(getParameterIndexForBand(kFaustParameterBand1_bypass));
            BaseWidget::done();
        }

    private:
        template<class B>
        std::shared_ptr<ButtonBaseWidget> addButton(const FaustParameterIndex index)
        {
            const FaustParameter& parameter = kFaustParameters[index];
            DISTRHO_SAFE_ASSERT_RETURN(d_isEqual(parameter.min, 0.f), nullptr);
            DISTRHO_SAFE_ASSERT_RETURN(d_isEqual(parameter.max, 1.f), nullptr);

            return BaseWidget::template addButton<B>(kParametersMainStart + index, getFaustParameterLabel(parameter));
        }

        void onNanoDisplay() final
        {
            BaseWidget::template drawReferenceBackground<R>();
            BaseWidget::template drawReferenceBorder<R>();
        }
    };

    class HeaderWidget final : public TransparentSizedContainerWidget<kHorizontal, kHeaderHeight>
    {
        using BaseWidget = TransparentSizedContainerWidget<kHorizontal, kHeaderHeight>;

        struct TextReference : Reference::Zero {
            static constexpr const Color color = Reference::kAccentColors[band + 1];
            static constexpr const float fontSize = 18;
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
    static constexpr const Color backgroundColor { 0x30, 0x32, 0x3c, 0.7f };
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

struct MbComp5StageReference : Reference::Stage {
    struct GridLabel {
        static constexpr const Color color { 140,142,150 };
        static constexpr const uint fontSize = 10;
        static constexpr const uint letterSpacing = 0;
    };
    struct Line {
        static constexpr const Color color { 0x3b, 0x3b, 0x44 };
        static constexpr const Color color〡main { color, 0.7f };
        static constexpr const Color color〡mid { color, 0.45f };
        static constexpr const Color color〡other { color, 0.22f };
        static constexpr const uint width = 1;
    };
};

class MbComp5StageWidget : public StageWidget<EasyStageWidget, MbComp5ExpertPageWidget>
{
    using R = MbComp5StageReference;
    using BaseWidget = StageWidget<EasyStageWidget, MbComp5ExpertPageWidget>;

    static constexpr const float kCrossoverFreqMin = kFaustParameters[kFaustParameterXover1].min;
    static constexpr const float kCrossoverFreqMax = kFaustParameters[kFaustParameterXover4].max;
    static constexpr const float kCrossoverRatio = 2.f; // 2x previous freq for 1 octave between crossovers

    static constexpr const int kGridDbMin = -18.f;
    static constexpr const int kGridDbMax = 6.f;
    static constexpr const int kGridDbRange = kGridDbMax - kGridDbMin;

    std::shared_ptr<MbComp5GraphWidget> graph { new MbComp5GraphWidget(this) };

public:
    explicit MbComp5StageWidget(LabWidget* const parent)
        : BaseWidget(parent)
    {
        graph->toBottom();
    }

private:
    bool bypassed() const { return fInterface->getParameterValue(kCommonParameterBypass) > .5f; }

    float value(int p) const { return fInterface->getParameterValue(kParametersMainStart + p); }

    bool anyListen() const { for (const auto& b : bands) if (value(b[5]) > .5f) return true; return false; }

    void roundedRect(float x,
                     float y,
                     float width,
                     float height,
                     float radiusTopLeft,
                     float radiusTopRight,
                     float radiusBottomRight,
                     float radiusBottomLeft)
    {
        moveTo(x + radiusTopLeft, y);
        lineTo(x + width - radiusTopRight, y);
        arcTo(x + width, y, x + width, y + radiusTopRight, radiusTopRight);
        lineTo(x + width, y + height - radiusBottomRight);
        arcTo(x + width, y + height, x + width - radiusBottomRight, y + height, radiusBottomRight);
        lineTo(x + radiusBottomLeft, y + height);
        arcTo(x, y + height, x, y + height - radiusBottomLeft, radiusBottomLeft);
        lineTo(x, y + radiusTopLeft);
        arcTo(x, y, x + radiusTopLeft, y, radiusTopLeft);
        closePath();
    }

    bool live(int b) const
    {
        return !bypassed() && value(bands[b][6]) < .5f && (!anyListen() || value(bands[b][5]) > .5f);
    }

    float nextFreq(const float freq, const float previous) const noexcept
    {
        const float minFreq = previous * kCrossoverRatio;
        return std::max(freq, minFreq);
    }

    void onNanoDisplay() final
    {
        BaseWidget::onNanoDisplay();
        // return;

        const float w = getWidth();
        const float h = getHeight();
        const float borderRadius = R::borderRadius * fScaleFactor;
        const float margin = R::margin * fScaleFactor;

        const float freq1 = fInterface->getParameterValue(kParametersMainStart + kFaustParameterXover1);
        const float freq2 = nextFreq(fInterface->getParameterValue(kParametersMainStart + kFaustParameterXover2), freq1);
        const float freq3 = nextFreq(fInterface->getParameterValue(kParametersMainStart + kFaustParameterXover3), freq2);
        const float freq4 = nextFreq(fInterface->getParameterValue(kParametersMainStart + kFaustParameterXover4), freq3);

        const std::array<double, 6> freqs = { kFreqMin, freq1, freq2, freq3, freq4, kFreqMax };

        // background coloring matching each band dedicated frequency
        for (uint8_t b = 0; b < 5; ++b)
        {
            const float x1 = freq2norm(freqs[b]) * w;
            const float x2 = freq2norm(freqs[b + 1]) * w;
            const float rl = b == 0 ? borderRadius : 0;
            const float rr = b == 4 ? borderRadius : 0;

            beginPath();
            roundedRect(x1, 0, x2 - x1, h, rl, rr, rr, rl);
            fillColor(Color(Reference::kAccentColors[b + 1], live(b) ? .045f : .012f));
            // fillColor(Color(Reference::kAccentColors[b + 1]));
            fill();
        }

        // grid and labels
        fillColor(R::GridLabel::color);
        fontFace("regular");
        fontSize(R::GridLabel::fontSize * fScaleFactor);
        strokeWidth(fScaleFactor);
        textLetterSpacing(R::GridLabel::letterSpacing * fScaleFactor);

        const float gfontsize = R::GridLabel::fontSize * fScaleFactor;
        const float gmargin = margin + gfontsize * 0.5f;
        const float gh = h - (R::margin * 2 + R::padding + R::GridLabel::fontSize +
            (fLastPage == kPageExpert
                ? MbComp5BandsReference::height + MbComp5NonBandParamsGroupReference::height + R::padding
                : Reference::Widgets::CardGroup::height
            )) * fScaleFactor;

        graph->setBox(gmargin, w, gh);

        // horizontal grid: 9 lines for each power of 10
        {
            char s[16];
            textAlign(ALIGN_CENTER | ALIGN_MIDDLE);

            for (int dec = 10; dec < kFreqMax; dec *= 10)
            {
                for (uint8_t m = 0; m < 9; ++m)
                {
                    const int freq = (m + 1) * dec;
                    if (freq <= kFreqMin || freq >= kFreqMax)
                        continue;

                    const float x = freq2norm(freq) * w;

                    switch (m)
                    {
                    case 0:
                        strokeColor(R::Line::color〡main);
                        break;
                    case 4:
                        strokeColor(R::Line::color〡mid);
                        break;
                    default:
                        strokeColor(R::Line::color〡other);
                        break;
                    }

                    beginPath();
                    moveTo(x, gmargin);
                    lineTo(x, h);
                    stroke();

                    if ((m == 0 || m == 1 || m == 4) && freq >= 50)
                    {
                        if (freq >= 1000.f)
                            std::snprintf(s, sizeof(s), "%dk", freq / 1000);
                        else
                            std::snprintf(s, sizeof(s), "%d", freq);

                        text(x, gmargin * 0.5f, s);
                    }
                }
            }
        }

        // vertical grid, 1 line per each 6 dB
        {
            char s[24];

            for (int db = kGridDbMin; db <= kGridDbMax; db += 6)
            {
                const float y = gmargin + static_cast<float>(kGridDbMax - db) / kGridDbRange * gh;

                strokeColor(Color(0x64, 0x64, 0x70, db == 0 ? .5f : .22f));

                for (float x = 0; x < w; x += 7 * fScaleFactor)
                {
                    beginPath();
                    moveTo(x, y);
                    lineTo(x + 3 * fScaleFactor, y);
                    stroke();
                }

                std::snprintf(s, sizeof(s), "%c%d%s", db > 0 ? '+' : '-', std::abs(db), db == kGridDbMin ? " dB" : "");

                textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
                text(gfontsize, y, s);

                textAlign(ALIGN_RIGHT | ALIGN_MIDDLE);
                text(w - gfontsize, y, s);
            }
        }
    }

    void onPositionChanged(const PositionChangedEvent& ev) override
    {
        BaseWidget::onPositionChanged(ev);

        graph->setAbsolutePos(ev.pos);
    }

    void updateSize(const bool updateChildren) override
    {
        graph->setSize(getSize());

        BaseWidget::updateSize(updateChildren);
    }
};

// --------------------------------------------------------------------------------------------------------------------

using MbComp5MainArea = MainAreaContainerWidget<MbComp5StageWidget>;
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
