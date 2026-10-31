// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/containers/container.hpp"
#include "ui/containers/main-area.hpp"
#include "ui/containers/top-bar.hpp"
#include "ui/containers/ui.hpp"
#include "ui/widgets/grid.hpp"
#include "ui/widgets/number-box.hpp"
#include "ui/widgets/threshold-meter.hpp"
#include "ui/_lab/text.hpp"

#include "mbComp5-ui.hpp"

#include "LibreAudioParameters.hpp"

// --------------------------------------------------------------------------------------------------------------------

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

inline bool isBypassed(const LabUIWidgetInterface* const iface) noexcept
{
    return d_isNotZero(iface->getParameterValue(kCommonParameterBypass));
}

inline float getParameterValue(const LabUIWidgetInterface* const iface, const FaustParameterIndex index) noexcept
{
    return iface->getParameterValue(kParametersMainStart + index);
}

// --------------------------------------------------------------------------------------------------------------------

static constexpr const uint kCommonMargin = 8;
static constexpr const uint kCommonPadding = 6;

template<uint8_t band>
struct MbComp5BandGroupReference : Reference::Zero {
    static constexpr const Color backgroundColor { 0x30, 0x32, 0x3c, 0.7f };
    static constexpr const Color borderColor = backgroundColor;
    static constexpr const Color borderHighlightColor { 0x40, 0x42, 0x4c, 0.8f };
    static constexpr const Color borderShadowColor { 0, 0, 0, 0.12f };
    static constexpr const uint border = 1;
    static constexpr const uint borderRadius = 8;
    static constexpr const uint margin = kCommonMargin;
    static constexpr const uint padding = kCommonPadding;

    // TESTING
    static constexpr const Color shadowColor { 0, 0, 0, 0.6f };
    static constexpr const Point<int> shadowOffset = { 0, 5 };
    // static constexpr const Size<int> shadowSize = { -6, -6 };
    static constexpr const Size<int> shadowSize = { -12, -12 };
    static constexpr const uint shadowFeather = 14;

    struct Header : Reference::Zero {
        static constexpr const uint height = 18;

        struct Title : Reference::Zero {
            static constexpr const Color color = Reference::kAccentColors[band + 1];
            static constexpr const float fontSize = Header::height * 0.75f;
            static constexpr const float height = Header::height;
        };

        struct Buttons : Reference::Zero {
            static constexpr const Color backgroundColor = { 0xff, 0xff, 0xff, 0.07f };
            static constexpr const Color borderColor = backgroundColor;
            static constexpr const Color borderHighlightColor { borderColor, 0.16f };
            static constexpr const Color borderShadowColor { 0, 0, 0, 0.16f };
            static constexpr const uint border = 1;
            static constexpr const uint borderRadius = 6;
            static constexpr const uint margin = 2;
            static constexpr const uint padding = 2;
            static constexpr const uint height = Header::height - Header::margin * 2;

            static constexpr const Color shadowColor { 0, 0, 0, 0.6f };
            static constexpr const Point<int> shadowOffset = { 0, 2 };
            static constexpr const Size<int> shadowSize = { -8, -8 };
            static constexpr const uint shadowFeather = 12;

            struct Solo : Reference::Zero {
                static constexpr const Color backgroundColor〡checked = Reference::kAccentColors[band + 1];
                static constexpr const Color color = Reference::Colors::ink2;
                static constexpr const Color color〡checked = Color(0x17, 0x17, 0x1b);
                static constexpr const Color color〡hover = Reference::kAccentColors[band + 1];
                static constexpr const MouseCursor cursor〡hover = kMouseCursorHand;
                static constexpr const uint borderRadius = 4;
                static constexpr const uint width = Header::height;
            };

            struct Bypass : Solo {
                static constexpr const Color backgroundColor〡checked = Reference::Widgets::Button〡Bypass::backgroundColorBypass;
                static constexpr const Color color〡checked = Reference::Widgets::Button〡Bypass::colorBypass;
                static constexpr const Color color〡hover = Reference::Widgets::Button〡Bypass::colorBypass;
            };
        };
    };

    struct Body : Reference::Zero {
        static constexpr const uint padding = kCommonPadding;
        static constexpr const uint height = Reference::Widgets::NumberBox<>::height * 5 + padding * 4;

        struct NumberBoxes : Reference::Zero {
            static constexpr const uint padding = kCommonPadding;
        };
    };

    static constexpr const uint height = margin * 2 + Header::height + padding + Body::height;
};

inline constexpr const uint8_t kNumBands = 5;

inline constexpr const char* kBandLabels[kNumBands] = {
    "LOW",
    "LOW MID",
    "MID",
    "HIGH MID",
    "HIGH"
};

template<uint8_t band>
class MbComp5BandGroupWidget final : public ReferenceContainerWidget<MbComp5BandGroupReference<band>, Vertical>
{
    using R = MbComp5BandGroupReference<band>;
    using BaseWidget = ReferenceContainerWidget<R, Vertical>;

    static constexpr const Color kAccentColor = Reference::kAccentColors[band + 1];

    static constexpr const uint32_t kFirstIdInBand = kParametersMainStart + kFaustParameterBand1_threshold;
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

    const std::array<std::shared_ptr<LabWidget>, 2> widgets {
        this->template addWidget<HeaderWidget>(),
        this->template addWidget<BodyWidget, Expanding>(),
    };

    // ----------------------------------------------------------------------------------------------------------------

public:
    using MeterWidget = ThresholdMeterWidget<Reference::Widgets::ThresholdMeter<kAccentColor>>;

    explicit MbComp5BandGroupWidget(LabWidget* const parent)
        : BaseWidget(parent) {}

    // ----------------------------------------------------------------------------------------------------------------
    // header: text + buttons

    class HeaderWidget final : public ReferenceContainerWidget<typename R::Header, Horizontal>
    {
        using HR = typename R::Header;
        using BaseWidget = ReferenceContainerWidget<HR, Horizontal>;

        const std::array<std::shared_ptr<LabWidget>, 2> widgets {
            this->template addWidget<ReferenceTextWidget<typename HR::Title>, Expanding>(kBandLabels[band]),
            this->template addWidget<SoloMuteGroupWidget>(),
        };

    public:
        explicit HeaderWidget(LabWidget* const parent)
            : BaseWidget(parent) {}

        class SoloMuteGroupWidget : public ReferenceButtonGroupWidget<typename HR::Buttons>
        {
            using SMGR = typename HR::Buttons;
            using BaseWidget = ReferenceButtonGroupWidget<SMGR>;

            template<class SMBR, const uchar* imageData, uint imageDataSize>
            class SoloMuteButton : public ImageButtonWidget<kCornerBoth, imageData, imageDataSize, SMBR>
            {
            public:
                using BaseWidget = ImageButtonWidget<kCornerBoth, imageData, imageDataSize, SMBR>;

                explicit SoloMuteButton(LabWidget* const parent, const uint id)
                    : BaseWidget(parent, id) {}

            private:
                void onNanoDisplay() override
                {
                    if (BaseWidget::isChecked())
                        BaseWidget::template drawReferenceBackground<SMBR, SMBR::backgroundColor〡checked, kCornerBoth>();
                    else
                        BaseWidget::template drawReferenceBackground<SMBR, SMBR::backgroundColor, kCornerBoth>();

                    BaseWidget::template drawReferenceBorder<SMBR, SMBR::borderColor>();

                    if (BaseWidget::isChecked())
                        BaseWidget::drawColoredImage(SMBR::color〡checked);
                    else if (BaseWidget::isHovered())
                        BaseWidget::drawColoredImage(SMBR::color〡hover);
                    else
                        BaseWidget::drawColoredImage(SMBR::color);
                }
            };

        public:
            using SoloButtonWidget = SoloMuteButton<typename SMGR::Solo, IMAGES_SOLO_28_PNG_DATA, IMAGES_SOLO_28_PNG_LEN>;
            using MuteButtonWidget = SoloMuteButton<typename SMGR::Bypass, IMAGES_POWER_24_PNG_DATA, IMAGES_POWER_24_PNG_LEN>;

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
        };
    };

    // ----------------------------------------------------------------------------------------------------------------
    // body: number-boxes stack + meter

    class BodyWidget final : public ReferenceContainerWidget<typename R::Body, Horizontal>
    {
        using BR = typename R::Body;
        using BaseWidget = ReferenceContainerWidget<BR, Horizontal>;

        const std::array<std::shared_ptr<LabWidget>, 2> widgets {
            this->template addWidget<NumberBoxStackWidget, Expanding>(),
            this->template addWidget<MeterWidget>(getParameterIndexForBand(kFaustParameterBand1_threshold),
                                                                           getLevelParameterIndex(false),
                                                                           getLevelParameterIndex(true)),
        };

    public:
        explicit BodyWidget(LabWidget* const parent)
            : BaseWidget(parent)
        {
            std::vector<uint32_t> linked;
            linked.reserve(kNumBands);

            for (uint8_t b = 0; b < kNumBands; ++b)
                if (band != b)
                    linked.emplace_back(kFirstIdInBand + b * kNumParamsPerBand);

            static_cast<MeterWidget*>(widgets.back().get())->setLinkedParameters(linked);
        }

        class NumberBoxStackWidget final : public ReferenceContainerWidget<typename BR::NumberBoxes, Vertical>
        {
            const std::array<std::shared_ptr<LabKnobWidget>, 5> widgets {
                this->template addWidget<NumberBoxWidgetR>(getParameterIndexForBand(kFaustParameterBand1_threshold)),
                this->template addWidget<NumberBoxWidgetR>(getParameterIndexForBand(kFaustParameterBand1_ratio)),
                this->template addWidget<NumberBoxWidgetR>(getParameterIndexForBand(kFaustParameterBand1_attack)),
                this->template addWidget<NumberBoxWidgetR>(getParameterIndexForBand(kFaustParameterBand1_release)),
                this->template addWidget<NumberBoxWidgetR>(getParameterIndexForBand(kFaustParameterBand1_makeup)),
            };

        public:
            using NBR = typename BR::NumberBoxes;
            using BaseWidget = ReferenceContainerWidget<NBR, Vertical>;
            using NumberBoxWidgetR = NumberBoxWidget<kAccentColor>;

            explicit NumberBoxStackWidget(LabWidget* const parent)
                : BaseWidget(parent)
            {
                std::vector<uint32_t> linked;
                linked.reserve(kNumBands);

                for (std::shared_ptr<LabKnobWidget> knob : widgets)
                {
                    const uint32_t offset = (knob->getId() - kFirstIdInBand) % kNumParamsPerBand;
                    linked.clear();

                    for (uint8_t b = 0; b < kNumBands; ++b)
                        if (band != b)
                            linked.emplace_back(kFirstIdInBand + b * kNumParamsPerBand + offset);

                    knob->setLinkedParameters(linked);
                }
            }
        };
    };
};

// --------------------------------------------------------------------------------------------------------------------

struct MbComp5BandsReference : Reference::Zero {
    static constexpr const uint padding = kCommonPadding;
    static constexpr const uint height = MbComp5BandGroupReference<0>::height + margin * 2;
};

using MbComp5BandsWidget = StaticContainerWidget<
    MbComp5BandsReference,
    Horizontal,
    Expanding,
    MbComp5BandGroupWidget<__COUNTER__>,
    MbComp5BandGroupWidget<__COUNTER__>,
    MbComp5BandGroupWidget<__COUNTER__>,
    MbComp5BandGroupWidget<__COUNTER__>,
    MbComp5BandGroupWidget<__COUNTER__>
>;

// --------------------------------------------------------------------------------------------------------------------

struct MbComp5NonBandParamsGroupReference : MbComp5BandGroupReference<0> {
    static constexpr const uint height = Reference::Widgets::NumberBox<>::height + (border + margin) * 2;
};

using MbComp5NonBandParamsGroupWidget = StaticParameterContainerWidget<
    MbComp5NonBandParamsGroupReference,
    Horizontal,
    Expanding,
    ParameterWidgetPair<kFaustParameterSlope, NumberBoxWidget<>>,
    ParameterWidgetPair<kFaustParameterKnee, NumberBoxWidget<>>,
    ParameterWidgetPair<kFaustParameterRange, NumberBoxWidget<>>,
    ParameterWidgetPair<kFaustParameterRms_time, NumberBoxWidget<>>,
    ParameterWidgetPair<kFaustParameterShape, NumberBoxWidget<>>,
    ParameterWidgetPair<kFaustParameterChan_link, NumberBoxWidget<>>,
    ParameterWidgetPair<kFaustParameterBand_link, NumberBoxWidget<>>,
    ParameterWidgetPair<kFaustParameterLookahead, NumberBoxWidget<>>,
    ParameterWidgetPair<kFaustParameterDrywet, NumberBoxWidget<>>
>;

// --------------------------------------------------------------------------------------------------------------------

struct MbComp5ExpertPageReference : Reference::Zero {
    static constexpr const uint margin = 12;
    static constexpr const uint padding = 8;
};

using MbComp5ExpertPageWidget = StaticBottomAlignedContainerWidget<
    MbComp5ExpertPageReference,
    MbComp5BandsWidget,
    MbComp5NonBandParamsGroupWidget
>;

// --------------------------------------------------------------------------------------------------------------------

struct MbComp5StageReference : Reference::Stage {
    struct GridLabel {
        static constexpr const Color color { 140, 142, 150 };
        static constexpr const float fontSize = 10.f;
        static constexpr const float letterSpacing = 0.f;
    };
    struct Line {
        static constexpr const Color color { 0x3b, 0x3b, 0x44 };
        static constexpr const Color color〡main { color, 0.7f };
        static constexpr const Color color〡mid { color, 0.45f };
        static constexpr const Color color〡other { color, 0.22f };
        static constexpr const uint width = 1;
    };
    struct FreqLine {
        // font mono
        static constexpr const Color backgroundColor { Line::color, 0.5f };
        static constexpr const float fontSize = 11.f;
        static constexpr const float letterSpacing = 0.f;
        static constexpr const uint borderRadius = 4;
    };
};

class MbComp5StageWidget : public StageWidget<EasyStageWidget, MbComp5ExpertPageWidget>
{
    static constexpr const float kCrossoverFreqMin = kFaustParameters[kFaustParameterXover1].min;
    static constexpr const float kCrossoverFreqMax = kFaustParameters[kFaustParameterXover4].max;
    static constexpr const float kCrossoverRatio = 2.f; // 2x previous freq for 1 octave between crossovers

    static constexpr const float kCrossoverFreqsMin[6] = {
        kFreqMinf,
        kFaustParameters[kFaustParameterXover1].min,
        kFaustParameters[kFaustParameterXover2].min,
        kFaustParameters[kFaustParameterXover3].min,
        kFaustParameters[kFaustParameterXover4].min,
        kFreqMaxf,
    };
    static constexpr const float kCrossoverFreqsMax[6] = {
        kFreqMinf,
        kFaustParameters[kFaustParameterXover1].max,
        kFaustParameters[kFaustParameterXover2].max,
        kFaustParameters[kFaustParameterXover3].max,
        kFaustParameters[kFaustParameterXover4].max,
        kFreqMaxf,
    };

public:
    using R = MbComp5StageReference;
    using BaseWidget = StageWidget<EasyStageWidget, MbComp5ExpertPageWidget>;

    explicit MbComp5StageWidget(LabWidget* const parent)
        : BaseWidget(parent)
    {
    }

private:
    std::array<float, 6> fFreqs = {
        kFreqMin,
        kFaustParameters[kFaustParameterXover1].init,
        kFaustParameters[kFaustParameterXover2].init,
        kFaustParameters[kFaustParameterXover3].init,
        kFaustParameters[kFaustParameterXover4].init,
        kFreqMax,
    };
    std::array<double, 6> fFreqPos = {};
    int fFreqDraggingIndex = -1;
    int fFreqDraggingHz = 0.f;
    bool fFreqsDragging[4] = {};

    const std::shared_ptr<MbComp5GraphWidget> fGraph { addGraphWidget<MbComp5GraphWidget>() };
    const std::shared_ptr<LabWidget> fGrid { addGraphWidget<GridWidget>(-18.f, 7.f, 6.f, "dB") };

    void idleCallback() final
    {
        BaseWidget::idleCallback();

        // ignore when dragging, to prevent flicker
        if (fFreqDraggingIndex != -1)
            return;

        const double dw = getWidth();

        if (const float f1 = getParameterValue(fInterface, kFaustParameterXover1); d_isNotEqual(fFreqs[1], f1))
        {
            fFreqs[1] = f1;
            fFreqPos[1] = d_roundToUnsignedInt(freq2norm(f1) * dw);
            repaint();
        }

        if (const float f2 = nextFreq(getParameterValue(fInterface, kFaustParameterXover2), fFreqs[1]); d_isNotEqual(fFreqs[2], f2))
        {
            fFreqs[2] = f2;
            fFreqPos[2] = d_roundToUnsignedInt(freq2norm(f2) * dw);
            repaint();
        }

        if (const float f3 = nextFreq(getParameterValue(fInterface, kFaustParameterXover3), fFreqs[2]); d_isNotEqual(fFreqs[3], f3))
        {
            fFreqs[3] = f3;
            fFreqPos[3] = d_roundToUnsignedInt(freq2norm(f3) * dw);
            repaint();
        }

        if (const float f4 = nextFreq(getParameterValue(fInterface, kFaustParameterXover4), fFreqs[3]); d_isNotEqual(fFreqs[4], f4))
        {
            fFreqs[4] = f4;
            fFreqPos[4] = d_roundToUnsignedInt(freq2norm(f4) * dw);
            repaint();
        }
    }

    bool bypassed() const { return isBypassed(fInterface); }

    float value(FaustParameterIndex p) const { return getParameterValue(fInterface, p); }

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

    Size<uint> getGraphSize() const noexcept final
    {
        const LabMetrics<R> m { this };

        const float gh = m.height - (R::border + R::margin * 2 +
            (fLastPage == kPageExpert
                ? MbComp5BandsReference::height + MbComp5NonBandParamsGroupReference::height + R::GridLabel::fontSize * 0.5f + R::padding
                : Reference::Widgets::CardGroup::height + R::margin * 0.5f
            )) * fScaleFactor;

        return { getWidth(), d_roundToUnsignedInt(gh) };
    }

    void onNanoDisplay() final
    {
        const LabMetrics<R> m { this };

        char valuestr[24];

        // background coloring matching each band dedicated frequency
        for (uint8_t b = 0; b < fFreqs.size() - 1; ++b)
        {
            const float x1 = fFreqPos[b];
            const float x2 = fFreqPos[b + 1];
            const float rl = b == 0 ? m.borderRadius : 0;
            const float rr = b == 4 ? m.borderRadius : 0;

            beginPath();
            roundedRect(x1, 0, x2 - x1, m.height, rl, rr, rr, rl);
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

        const float gh = getGraphSize().getHeight();

        fGraph->setFreqs({ fFreqs[1], fFreqs[2], fFreqs[3], fFreqs[4] });

        if (fLastPage == kPageExpert)
        {
            fontFace("mono");
            fontSize(R::FreqLine::fontSize * fScaleFactor);
            strokeWidth(1.2f * fScaleFactor);
            textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
            textLetterSpacing(R::FreqLine::letterSpacing * fScaleFactor);

            Rectangle<float> bounds;

            const float sy = m.border + m.margin;
            const float ey = gh - m.border - m.margin;
            const float gfontsize = R::FreqLine::fontSize * fScaleFactor;
            const float fade = gh * .18f;

            for (uint8_t i = 1; i < fFreqs.size() - 1; ++i)
            {
                const float freq = fFreqs[i];
                const float x = fFreqPos[i];

                const Color& color = Reference::kAccentColors[i + 1];
                const Color colorHalf(color, .5f);
                const Color colorTrans(color, 0.f);

                beginPath();
                moveTo(x, sy);
                lineTo(x, sy + fade);
                strokePaint(linearGradient(x, sy, x, sy + fade, colorTrans, colorHalf));
                stroke();

                beginPath();
                moveTo(x, sy + fade);
                lineTo(x, ey - fade);
                strokeColor(colorHalf);
                stroke();

                beginPath();
                moveTo(x, ey - fade);
                lineTo(x, ey);
                strokePaint(linearGradient(x, ey - fade, x, ey, colorHalf, colorTrans));
                stroke();

                if (freq >= 1000.f)
                    std::snprintf(valuestr, sizeof(valuestr), "%.1f kHz", static_cast<float>(freq) * 0.001f);
                else
                    std::snprintf(valuestr, sizeof(valuestr), "%.0f Hz", freq);

                textBounds(x, gh, valuestr, nullptr, bounds);

                beginPath();
                BaseWidget::roundedRect(bounds.getX() - gfontsize * 0.25f,
                                        bounds.getY() - gfontsize * 0.25f,
                                        bounds.getWidth() + gfontsize * 0.5f,
                                        bounds.getHeight() + gfontsize * 0.5f,
                                        R::FreqLine::borderRadius * fScaleFactor);
                fillColor(R::FreqLine::backgroundColor);
                fill();

                fillColor(color);
                text(x, gh, valuestr);
            }
        }

        drawReferenceBorder<R>();
    }

    bool onMouse(const MouseEvent& ev) final
    {
        if (BaseWidget::onMouse(ev))
            return true;

        if (! ev.press)
        {
            if (fFreqDraggingIndex != -1)
            {
                const float orig = fFreqDraggingHz;

                // value going up, release bands from highest to lowest
                if (fFreqs[fFreqDraggingIndex] > orig)
                {
                    for (uint8_t i = 3; i != UINT8_MAX; --i)
                    {
                        if (fFreqsDragging[i])
                        {
                            fFreqsDragging[i] = false;
                            fInterface->parameterControlReleased(kParametersMainStart + kFaustParameterXover1 + i);
                        }
                    }
                }
                // value going down, release bands from lowest to highest
                else
                {
                    for (uint8_t i = 0; i < 4; ++i)
                    {
                        if (fFreqsDragging[i])
                        {
                            fFreqsDragging[i] = false;
                            fInterface->parameterControlReleased(kParametersMainStart + kFaustParameterXover1 + i);
                        }
                    }
                }

                fFreqDraggingIndex = -1;
            }
            return false;
        }

        const int hit = d_roundToIntPositive(6 * fScaleFactor);
        const double x = ev.pos.getX();
        const double y = ev.pos.getY();

        const float gfontsize = R::GridLabel::fontSize * fScaleFactor;
        const float gmargin = R::margin * fScaleFactor + gfontsize * 0.5f;
        const float gh = getHeight() - (R::margin * 2 + R::padding + R::GridLabel::fontSize +
            (fLastPage == kPageExpert
                ? MbComp5BandsReference::height + MbComp5NonBandParamsGroupReference::height + R::padding
                : Reference::Widgets::CardGroup::height
            )) * fScaleFactor;

        if (y < gmargin)
            return false;
        if (y > gmargin + gh)
            return false;

        for (uint i = 1; i < fFreqPos.size() - 1; ++i)
        {
            const double fx = fFreqPos[i];

            if (std::abs(fx - x) <= hit)
            {
                fFreqDraggingIndex = i;
                fFreqDraggingHz = fFreqs[i];
                fFreqsDragging[i - 1] = true;
                fInterface->parameterControlPressed(kParametersMainStart + kFaustParameterXover1 + i - 1);
                return true;
            }
        }

        return false;
    }

    bool onMotion(const MotionEvent& ev) final
    {
        if (fFreqDraggingIndex != -1)
        {
            const int fd = fFreqDraggingIndex;
            DISTRHO_SAFE_ASSERT_RETURN(fd >= 1 && fd <= 4, false);

            const double x = ev.pos.getX();
            const double w = getWidth();

            if (const float freq = std::clamp(norm2freq(x / w),
                                              kCrossoverFreqsMin[fd],
                                              kCrossoverFreqsMax[fd]); d_isNotEqual(fFreqs[fd], freq))
            {
                int fd2 = fd;
                fFreqs[fd] = freq;
                fFreqPos[fd] = freq2norm(freq) * w;

                if (freq < fFreqs[fd - 1] * kCrossoverRatio)
                {
                    float freq2;
                    do
                    {
                        freq2 = fFreqs[fd2] / kCrossoverRatio;
                        --fd2;
                        fFreqs[fd2] = freq2;
                        fFreqPos[fd2] = freq2norm(freq2) * w;
                    }
                    while (fd2 >= 2 && freq2 < fFreqs[fd2 - 1] * kCrossoverRatio);
                }
                else if (freq > fFreqs[fd + 1] / kCrossoverRatio)
                {
                    float freq2;
                    do
                    {
                        freq2 = fFreqs[fd2] * kCrossoverRatio;
                        ++fd2;
                        fFreqs[fd2] = freq2;
                        fFreqPos[fd2] = freq2norm(freq2) * w;
                    }
                    while (fd2 < 4 && freq2 > fFreqs[fd2 + 1] / kCrossoverRatio);
                }

                for (int fdc = fd2; fdc > fd; --fdc)
                {
                    const uint8_t i = fdc - 1;
                    if (! fFreqsDragging[i])
                    {
                        fFreqsDragging[i] = true;
                        fInterface->parameterControlPressed(kParametersMainStart + kFaustParameterXover1 + i);
                    }
                    fInterface->parameterControlModified(kParametersMainStart + kFaustParameterXover1 + i, fFreqs[fdc]);
                }

                fInterface->parameterControlModified(kParametersMainStart + kFaustParameterXover1 + fd - 1, freq);

                for (int fdc = fd - 1; fdc >= fd2; --fdc)
                {
                    const uint8_t i = fdc - 1;
                    if (! fFreqsDragging[i])
                    {
                        fFreqsDragging[i] = true;
                        fInterface->parameterControlPressed(kParametersMainStart + kFaustParameterXover1 + i);
                    }
                    fInterface->parameterControlModified(kParametersMainStart + kFaustParameterXover1 + i, fFreqs[fdc]);
                }

                repaint();
            }

            return true;
        }

        if (BaseWidget::onMotion(ev))
            return true;

        return false;
    }

    void onResize(const ResizeEvent& ev) final
    {
        BaseWidget::onResize(ev);

        const uint w = ev.size.getWidth();

        fFreqPos[1] = d_roundToUnsignedInt(freq2norm(fFreqs[1]) * w);
        fFreqPos[2] = d_roundToUnsignedInt(freq2norm(fFreqs[2]) * w);
        fFreqPos[3] = d_roundToUnsignedInt(freq2norm(fFreqs[3]) * w);
        fFreqPos[4] = d_roundToUnsignedInt(freq2norm(fFreqs[4]) * w);
        fFreqPos[5] = w;
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
