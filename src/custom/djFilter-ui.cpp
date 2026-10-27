// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/containers/container.hpp"
#include "ui/containers/main-area.hpp"
#include "ui/containers/top-bar.hpp"
#include "ui/containers/ui.hpp"
#include "ui/widgets/card.hpp"
#include "ui/widgets/knob.hpp"
#include "ui/widgets/number-box.hpp"

#include "LibreAudioParameters.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>

// --------------------------------------------------------------------------------------------------------------------

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

static constexpr const uint kCommonMargin = 8;
static constexpr const uint kCommonPadding = 6;
static constexpr const uint kNumberBoxHeight = 50;

// --------------------------------------------------------------------------------------------------------------------

namespace DjFilterResponse {
    // maximum Q, mirrored from djFilter.dsp
    inline constexpr const double kQMax = 8;

    // display only: the DSP's minimum Q of 1 has a small peak, Butterworth Q draws 0% resonance flat
    inline constexpr const double kQMin = M_SQRT1_2;

    // display only: the corner sweeps from this far beyond one display edge to as far beyond the other,
    // so the filter enters from off-screen as the knob leaves center and leaves at full travel
    inline constexpr const double kOutsideRatio = 4;
    inline constexpr const double kSweepLow = kFreqMin / kOutsideRatio;
    inline constexpr const double kSweepHigh = kFreqMax * kOutsideRatio;

    // Illustrative response of the DSP's highpass and lowpass stages, in dB: analog 2-pole magnitudes
    // with the full filter shape, swept over the whole knob travel (the DSP's neutral zone, frequency
    // range and dry/filtered crossfade are not reproduced).
    // Overdrive, reverb and the volume fadeout at the knob extremes are not included.
    // Everything that depends only on the knobs is computed once, so db() is cheap per frequency.
    class Curve
    {
        struct Stage {
            bool active = false;
            double invFc = 0;  // 1 / corner frequency
            double invQ2 = 0;  // 1 / Q^2
        };

        Stage fHighpass, fLowpass;

        static Stage stage(const double t, const double fc, const double emphasizeQ)
        {
            const double q = kQMin * std::pow(kQMax / kQMin, emphasizeQ * t);
            return { true, 1 / fc, 1 / (q * q) };
        }

        // 10*log10 of the shared 2-pole denominator, (1-w^2)^2 + (w/Q)^2
        static double denDb(const double w2, const Stage& s)
        {
            const double a = 1 - w2;
            return 10 * std::log10(a * a + w2 * s.invQ2);
        }

    public:
        Curve(const double knob, const double emphasizeQ)
        {
            const double t = std::abs(knob);

            if (knob > 0)
                fHighpass = stage(t, kSweepLow * std::pow(kSweepHigh / kSweepLow, t), emphasizeQ);
            else if (knob < 0)
                fLowpass = stage(t, kSweepHigh * std::pow(kSweepLow / kSweepHigh, t), emphasizeQ);
        }

        double db(const double f) const
        {
            double db = 0;

            if (fHighpass.active)
            {
                const double w = f * fHighpass.invFc;
                const double w2 = w * w;
                db += 20 * std::log10(w2) - denDb(w2, fHighpass);
            }

            if (fLowpass.active)
            {
                const double w = f * fLowpass.invFc;
                db -= denDb(w * w, fLowpass);
            }

            return db;
        }
    };
} // namespace DjFilterResponse

// --------------------------------------------------------------------------------------------------------------------

// filter response curve, drawn in the same rainbow style as the mbComp5 gain-reduction curve
class DjFilterGraphWidget : public LabWidget
{
    static constexpr const float kDbMax = 6;
    static constexpr const float kDbMin = -12;
    // depth over which the curve and fill fade out toward the bottom edge
    static constexpr const float kFadeDb = 6;

    static constexpr const std::array<Color, 5> kColors {{
        {255,191,203}, {255,223,173}, {210,253,211}, {190,241,255}, {218,193,243}
    }};

    Rectangle<float> fBox;

public:
    explicit DjFilterGraphWidget(LabWidget* const parent)
        : LabWidget(parent) {}

    void setBox(const float x, const float y, const float width, const float height)
    {
        fBox = Rectangle<float>(x, y, width, height);
    }

private:
    bool bypassed() const { return fInterface->getParameterValue(kCommonParameterBypass) > .5f; }
    float value(int p) const { return fInterface->getParameterValue(kParametersMainStart + p); }

    // dB outside the window is pushed just past the top or bottom edge, where the scissor cuts it
    float yDb(const float db) const
    {
        return fBox.getY() + (kDbMax - std::clamp(db, kDbMin - 1, kDbMax + 1)) / (kDbMax - kDbMin) * fBox.getHeight();
    }

    Color rainbow(const float t, const float alpha = 1) const
    {
        const float p = std::clamp(t, 0.f, 1.f) * 4;
        const int i = std::min(3, static_cast<int>(p));
        const float f = p - i;
        Color c = kColors[i];
        for (int k = 0; k < 3; ++k)
            c.rgba[k] = kColors[i].rgba[k] * (1 - f) + kColors[i + 1].rgba[k] * f;
        c.alpha = alpha;
        return c;
    }

    void onNanoDisplay() final
    {
        if (fBox.isInvalid())
            return;

        constexpr int steps = 256;

        const float x0 = fBox.getX();
        const float w = fBox.getWidth();
        const bool isBypassed = bypassed();
        const DjFilterResponse::Curve curve(isBypassed ? 0.f : value(kFaustParameterKnob),
                                            value(kFaustParameterEmphasize_q) / kFaustParameters[kFaustParameterEmphasize_q].max);

        std::array<float, steps + 1> ys {};
        for (int i = 0; i <= steps; ++i)
            ys[i] = yDb(curve.db(norm2freq(i / static_cast<float>(steps))));

        // 1 above the fade zone, 0 at the bottom edge
        const float bottom = fBox.getY() + fBox.getHeight();
        const float fadeHeight = kFadeDb / (kDbMax - kDbMin) * fBox.getHeight();
        const auto fadeAt = [&](const float y) { return std::clamp((bottom - y) / fadeHeight, 0.f, 1.f); };

        save();
        scissor(fBox.getX(), fBox.getY(), fBox.getWidth(), fBox.getHeight());

        // Fill between 0 dB and the curve, colored along the curve. Its opacity depends only on the height in
        // the display: transparent at 0 dB, strongest at the top edge and at the start of the bottom fade zone,
        // then transparent again at the bottom edge. Each zone is a static vertical gradient under its own scissor.
        constexpr float fillAlpha = .24f;
        const float top = fBox.getY();
        const float zero = yDb(0);
        const float fadeTop = bottom - fadeHeight;

        struct FillZone {
            float y1, y2;     // scissor range, top to bottom
            float g1, g2;     // gradient start/end y
            float a1, a2;     // gradient start/end alpha
        };
        const FillZone zones[] = {
            { top, zero, zero, top, 0, fillAlpha },
            { zero, fadeTop, zero, fadeTop, 0, fillAlpha },
            { fadeTop, bottom, fadeTop, bottom, fillAlpha, 0 },
        };

        const auto fillSegment = [&](const FillZone& zone, float left, float y1, float right, float y2)
        {
            if (std::max({ y1, y2, zero }) <= zone.y1 || std::min({ y1, y2, zero }) >= zone.y2)
                return;
            const float t = ((left + right) / 2 - x0) / w;
            beginPath();
            moveTo(left, zero);
            lineTo(left, y1);
            lineTo(right, y2);
            lineTo(right, zero);
            closePath();
            fillPaint(linearGradient(0, zone.g1, 0, zone.g2, rainbow(t, zone.a1), rainbow(t, zone.a2)));
            fill();
        };

        for (const FillZone& zone : zones)
        {
            if (zone.y2 - zone.y1 < .5f)
                continue;

            save();
            intersectScissor(fBox.getX(), zone.y1, fBox.getWidth(), zone.y2 - zone.y1);

            for (int i = 0; i < steps; ++i)
            {
                const float left = x0 + i * w / steps;
                const float right = x0 + (i + 1) * w / steps;
                if ((ys[i] - zero) * (ys[i + 1] - zero) < 0)
                {
                    const float crossing = left + (right - left) * (zero - ys[i]) / (ys[i + 1] - ys[i]);
                    fillSegment(zone, left, ys[i], crossing, zero);
                    fillSegment(zone, crossing, zero, right, ys[i + 1]);
                }
                else
                {
                    fillSegment(zone, left, ys[i], right, ys[i + 1]);
                }
            }

            restore();
        }

        // wide glow, then the crisp line, stroked in short runs so color and depth fade can follow the curve
        constexpr int runLength = 8;
        const auto strokeRuns = [&](const float alpha, const float width)
        {
            strokeWidth(width * fScaleFactor);

            for (int i0 = 0; i0 < steps; i0 += runLength)
            {
                const int i1 = i0 + runLength;

                float runFade = 0;
                for (int i = i0; i <= i1; ++i)
                    runFade = std::max(runFade, fadeAt(ys[i]));
                if (runFade <= 0)
                    continue;

                const float left = x0 + i0 * w / steps;
                const float right = x0 + i1 * w / steps;

                beginPath();
                moveTo(left, ys[i0]);
                for (int i = i0 + 1; i <= i1; ++i)
                    lineTo(x0 + i * w / steps, ys[i]);
                strokePaint(linearGradient(left, 0, right, 0,
                                           rainbow((left - x0) / w, alpha * fadeAt(ys[i0])),
                                           rainbow((right - x0) / w, alpha * fadeAt(ys[i1]))));
                stroke();
            }
        };
        strokeRuns(isBypassed ? .04f : .13f, 6);
        strokeRuns(isBypassed ? .25f : .95f, 2);

        restore();
    }
};

// --------------------------------------------------------------------------------------------------------------------

// big filter knob without name/value text, centered horizontally
struct DjFilterBigKnobReference : Reference::Widgets::EasyKnob {
    static constexpr const uint height = width;
    static constexpr const bool showText = false;
};

using DjFilterBigKnobWidget = DrawableKnobWidget<DjFilterBigKnobReference>;

struct DjFilterBigKnobRowReference : Reference::Zero {
    static constexpr const uint height = DjFilterBigKnobReference::height;
};

class DjFilterBigKnobRowWidget final : public ReferenceContainerWidget<DjFilterBigKnobRowReference, Horizontal>
{
    using BaseWidget = ReferenceContainerWidget<DjFilterBigKnobRowReference, Horizontal>;

    const std::array<std::shared_ptr<LabWidget>, 3> widgets {
        addSpacer(),
        addWidget<DjFilterBigKnobWidget>(std::cref(kFaustParameters[kFaustParameterKnob]),
                                         kParametersMainStart + kFaustParameterKnob),
        addSpacer(),
    };

public:
    explicit DjFilterBigKnobRowWidget(LabWidget* const parent)
        : BaseWidget(parent) {}
};

// --------------------------------------------------------------------------------------------------------------------

// number boxes take their accent colors from the dsp file
namespace widgets {
    using DjFilterNumberBoxWidget = NumberBoxWidget<Reference::Colors::acc1, kNumberBoxHeight>;

    using ReverbSend = ParameterWidgetPair<kFaustParameterReverb_send, DjFilterNumberBoxWidget>;
    using EmphasizeQ = ParameterWidgetPair<kFaustParameterEmphasize_q, DjFilterNumberBoxWidget>;
    using Overdrive = ParameterWidgetPair<kFaustParameterOverdrive, DjFilterNumberBoxWidget>;
}

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
    static constexpr const uint height = kNumberBoxHeight + (border + margin) * 2;

    static constexpr const Color shadowColor { 0, 0, 0, 0.6f };
    static constexpr const Point<int> shadowOffset = { 0, 5 };
    static constexpr const Size<int> shadowSize = { -12, -12 };
    static constexpr const uint shadowFeather = 14;
};

template <typename... Args>
using DjFilterGroupWidget = StaticParameterContainerWidget<
    DjFilterGroupReference,
    Horizontal,
    Expanding,
    Args...
>;

// --------------------------------------------------------------------------------------------------------------------

struct DjFilterGroupsContainerReference : Reference::Zero {
    static constexpr const uint padding = kCommonPadding;
    static constexpr const uint height = DjFilterGroupReference::height + (border + margin) * 2;
};

// all number boxes in one group well, 80% of the display width and centered
class DjFilterGroupsContainerWidget final : public ReferenceContainerWidget<DjFilterGroupsContainerReference, Horizontal>
{
    using BaseWidget = ReferenceContainerWidget<DjFilterGroupsContainerReference, Horizontal>;
    using GroupWidget = DjFilterGroupWidget<widgets::ReverbSend, widgets::EmphasizeQ, widgets::Overdrive>;

    static constexpr const float kGroupWidthRatio = 0.8f;

    std::shared_ptr<LabWidget> fLeftSpacer = addSpacer();
    std::shared_ptr<GroupWidget> fGroup = addWidget<GroupWidget, Fixed>();
    std::shared_ptr<LabWidget> fRightSpacer = addSpacer();

public:
    explicit DjFilterGroupsContainerWidget(LabWidget* const parent)
        : BaseWidget(parent) {}

private:
    void updateSize(const bool updateChildren) override
    {
        static_cast<LabWidget*>(fGroup.get())->setWidth(d_roundToUnsignedInt(getParentWidget()->getWidth() * kGroupWidthRatio));
        BaseWidget::updateSize(updateChildren);
    }
};

// --------------------------------------------------------------------------------------------------------------------

class DjFilterPresetCardsWidget : public CardGroupWidget
{
    const std::array<std::shared_ptr<LabCardWidget>, 4> cards = {
        addCard(kWidgetPresetA, "NATURAL DOUBLE", "Subtle ADT thickening"),
        addCard(kWidgetPresetB, "TIGHT SLAP", "Short doubled slap"),
        addCard(kWidgetPresetC, "WIDE TRIPLE", "Three-voice spread"),
        addCard(kWidgetPresetD, "LUSH STACK", "Full ensemble stack"),
    };

public:
    explicit DjFilterPresetCardsWidget(LabWidget* const parent)
        : CardGroupWidget(parent)
    {
        done();
    }
};

// --------------------------------------------------------------------------------------------------------------------

// bottom area of both pages has the same height, so the big knob sits at the same place
static constexpr const uint kBottomAreaHeight = std::max(Reference::Widgets::CardGroup::height,
                                                         DjFilterGroupsContainerReference::height);

template <class W>
using DjFilterBottomAreaWidget = StaticBottomAlignedContainerWidget<Reference::Height<kBottomAreaHeight>, W>;

// --------------------------------------------------------------------------------------------------------------------

struct DjFilterEasyExpertPageReference : Reference::Zero {
    static constexpr const uint margin = 12;
    static constexpr const uint padding = 8;
};

// big filter knob at seven eighths down the display, preset cards at the bottom
using DjFilterEasyPageWidget = StaticContainerPairWidget<
    DjFilterEasyExpertPageReference,
    Vertical,
    SizeHintWidgetPair<Expanding, LabEmptyWidget>,
    SizeHintWidgetPair<Expanding, LabEmptyWidget>,
    SizeHintWidgetPair<Expanding, LabEmptyWidget>,
    SizeHintWidgetPair<Expanding, LabEmptyWidget>,
    SizeHintWidgetPair<Expanding, LabEmptyWidget>,
    SizeHintWidgetPair<Expanding, LabEmptyWidget>,
    SizeHintWidgetPair<Expanding, LabEmptyWidget>,
    SizeHintWidgetPair<Fixed, DjFilterBigKnobRowWidget>,
    SizeHintWidgetPair<Expanding, LabEmptyWidget>,
    SizeHintWidgetPair<Fixed, DjFilterBottomAreaWidget<DjFilterPresetCardsWidget>>
>;

// big filter knob at seven eighths down the display, knobs group at the bottom
using DjFilterExpertPageWidget = StaticContainerPairWidget<
    DjFilterEasyExpertPageReference,
    Vertical,
    SizeHintWidgetPair<Expanding, LabEmptyWidget>,
    SizeHintWidgetPair<Expanding, LabEmptyWidget>,
    SizeHintWidgetPair<Expanding, LabEmptyWidget>,
    SizeHintWidgetPair<Expanding, LabEmptyWidget>,
    SizeHintWidgetPair<Expanding, LabEmptyWidget>,
    SizeHintWidgetPair<Expanding, LabEmptyWidget>,
    SizeHintWidgetPair<Expanding, LabEmptyWidget>,
    SizeHintWidgetPair<Fixed, DjFilterBigKnobRowWidget>,
    SizeHintWidgetPair<Expanding, LabEmptyWidget>,
    SizeHintWidgetPair<Fixed, DjFilterBottomAreaWidget<DjFilterGroupsContainerWidget>>
>;

// --------------------------------------------------------------------------------------------------------------------

class DjFilterStageWidget final : public StageWidget<DjFilterEasyPageWidget, DjFilterExpertPageWidget>
{
    using BaseWidget = StageWidget<DjFilterEasyPageWidget, DjFilterExpertPageWidget>;

    std::shared_ptr<DjFilterGraphWidget> fGraph { new DjFilterGraphWidget(this) };

public:
    explicit DjFilterStageWidget(LabWidget* const parent)
        : BaseWidget(parent)
    {
        fGraph->toBottom();
    }

private:
    // the curve spans the whole well, behind the widgets of the current page
    void onNanoDisplay() final
    {
        BaseWidget::onNanoDisplay();

        fGraph->setBox(0, 0, getWidth(), getHeight());
    }

    void onPositionChanged(const PositionChangedEvent& ev) final
    {
        BaseWidget::onPositionChanged(ev);

        fGraph->setAbsolutePos(ev.pos);
    }

    void updateSize(const bool updateChildren) final
    {
        fGraph->setSize(getSize());

        BaseWidget::updateSize(updateChildren);
    }
};

using DjFilterMainArea = MainAreaContainerWidget<DjFilterStageWidget>;
using DjFilterRootWidget = RootWidget<TopBar, DjFilterMainArea>;

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

UI* createUI()
{
    return new LibreAudio::UI<LibreAudio::DjFilterRootWidget>();
}

END_NAMESPACE_DISTRHO
