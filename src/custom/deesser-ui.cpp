// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/containers/container.hpp"
#include "ui/containers/main-area.hpp"
#include "ui/containers/top-bar.hpp"
#include "ui/containers/ui.hpp"
#include "ui/widgets/check-box.hpp"
#include "ui/widgets/number-box.hpp"

#include "ui/_lab/knob.hpp"

#include "LibreAudioParameters.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>

// --------------------------------------------------------------------------------------------------------------------

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

static constexpr const uint kCommonMargin = 8;
static constexpr const uint kCommonPadding = 6;
static constexpr const uint kNumberBoxHeight = 50;
static constexpr const uint kCheckBoxGroupWidth = 115;

// two stacked in a number box's height
template<const Color& kAccentColor>
using DeEsserCheckBoxWidget = CheckBoxWidget<kAccentColor, (kNumberBoxHeight - kCommonPadding) / 2>;

template<const Color& kAccentColor>
using DeEsserNumberBoxWidget = NumberBoxWidget<kAccentColor, kNumberBoxHeight>;

// --------------------------------------------------------------------------------------------------------------------

namespace widgets {
    using Crossover = ParameterWidgetPair<kFaustParameterCrossover_frequency, DeEsserNumberBoxWidget<Reference::Colors::acc2>>;
    using Channel = ParameterWidgetPair<kFaustParameterChannel, DeEsserNumberBoxWidget<Reference::Colors::acc5>>;

    using Mode = ParameterWidgetPair<kFaustParameterMode, DeEsserNumberBoxWidget<Reference::Colors::acc5>>;

    using Ratio = ParameterWidgetPair<kFaustParameterRatio, DeEsserNumberBoxWidget<Reference::Colors::acc3>>;
    using Range = ParameterWidgetPair<kFaustParameterRange, DeEsserNumberBoxWidget<Reference::Colors::acc4>>;

    using Lookahead = ParameterWidgetPair<kFaustParameterLookahead, DeEsserNumberBoxWidget<Reference::Colors::acc4>>;
    using DryWet = ParameterWidgetPair<kFaustParameterDrywet, DeEsserNumberBoxWidget<Reference::Colors::acc2>>;

    using Delta = ParameterWidgetPair<kFaustParameterDelta, DeEsserCheckBoxWidget<Reference::Colors::acc6>>;
    using Listen = ParameterWidgetPair<kFaustParameterListen, DeEsserCheckBoxWidget<Reference::Colors::acc4>>;
}

// --------------------------------------------------------------------------------------------------------------------

struct DeEsserGroupReference : Reference::Zero {
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
using DeEsserGroupWidget = StaticParameterContainerWidget<DeEsserGroupReference, Horizontal, Expanding, Args...>;

// the monitoring switches, stacked
using DeEsserCheckBoxGroupWidget = StaticParameterContainerWidget<
    Reference::Width<kCheckBoxGroupWidth, DeEsserGroupReference>,
    Vertical,
    Expanding,
    widgets::Listen, widgets::Delta
>;

// --------------------------------------------------------------------------------------------------------------------

struct DeEsserGroupsContainerReference : Reference::Zero {
    static constexpr const uint padding = kCommonPadding;
    static constexpr const uint height = DeEsserGroupReference::height + (border + margin) * 2;
};

// Band, amount, timing and mix, then the monitoring switches, side by side.
using DeEsserMainGroupsWidget = StaticContainerPairWidget<
    DeEsserGroupsContainerReference,
    Horizontal,
    SizeHintWidgetPair<Expanding, DeEsserGroupWidget<widgets::Crossover, widgets::Channel>>,
    SizeHintWidgetPair<Expanding, DeEsserGroupWidget<widgets::Ratio, widgets::Range>>,
    SizeHintWidgetPair<Expanding, DeEsserGroupWidget<widgets::Lookahead, widgets::DryWet>>,
    SizeHintWidgetPair<Fixed, DeEsserCheckBoxGroupWidget>
>;

// --------------------------------------------------------------------------------------------------------------------

struct DeEsserPageReference : Reference::Zero {
    static constexpr const uint margin = 12;
    static constexpr const uint padding = 8;
};

// The control row at the bottom of the stage, the space above it left to the display.
using DeEsserPageWidget = StaticBottomAlignedContainerWidget<DeEsserPageReference, DeEsserMainGroupsWidget>;

// --------------------------------------------------------------------------------------------------------------------
// The display's columns: group panels like the control row's, as tall as the display gives them.

struct DeEsserColumnReference : DeEsserGroupReference {
    static constexpr const uint height = 0;
};

// of the meter columns
static constexpr const uint kMeterColumnWidth = 110;

// --------------------------------------------------------------------------------------------------------------------
// A sidechain meter: a title on top, a dB scale down the left, a well with a track for mid and one for side, their
// names under the tracks, and the threshold's triangle right of the well with its line across it. The meter's maximum
// is at the top of the well and its minimum at the bottom; the tracks fill up from the bottom. Dragging anywhere on it
// sets the threshold.

struct DeEsserMeterReference : Reference::Zero {
    struct Title {
        static constexpr const Color color = Reference::Colors::ink3;
        static constexpr const float fontSize = 10.f;
        static constexpr const float letterSpacing = fontSize * 0.08f;
        static constexpr const uint height = 12;
        static constexpr const uint gap = 8;
    };
    struct Channel {
        static constexpr const Color color = Reference::Colors::ink3;
        static constexpr const float fontSize = 10.f;
        static constexpr const uint height = 10;
        static constexpr const uint gap = 6;
    };
    struct Scale {
        static constexpr const Color color { Reference::Colors::ink3, 0.8f };
        static constexpr const float fontSize = 9.f;
        static constexpr const uint width = 22;
        static constexpr const uint gap = 4;
    };
    // the threshold's triangle, right of the well
    struct Slider {
        static constexpr const uint width = 8;
        static constexpr const uint gap = 2;
    };
    struct Well {
        static constexpr const Color backgroundColor = Reference::Colors::track;
        static constexpr const Color insetShadingColor { 0.f, 0.f, 0.f, 0.55f };
        static constexpr const uint border = 1;
        static constexpr const uint borderRadius = 5;
    };
    struct Track {
        static constexpr const uint width = 14;
        static constexpr const uint gap = 2;
    };
    struct Tick {
        static constexpr const Color color { Reference::Colors::ink3, 0.15f };
    };
};

// What one meter shows: its title, the threshold it sets, the mid and side levels, and the dB of its scale's labels.
struct DeEsserAbsoluteMeter {
    static constexpr const char* const title = "ABSOLUTE";
    static constexpr const FaustParameterIndex threshold = kFaustParameterThreshold_abs;
    static constexpr const FaustParameterIndex mid = kFaustParameterSc_mid_abs;
    static constexpr const FaustParameterIndex side = kFaustParameterSc_side_abs;
    static constexpr const std::array<float, 6> scale { 0, -12, -24, -36, -48, -60 };
};

struct DeEsserRelativeMeter {
    static constexpr const char* const title = "RELATIVE";
    static constexpr const FaustParameterIndex threshold = kFaustParameterThreshold;
    static constexpr const FaustParameterIndex mid = kFaustParameterSc_mid_rel;
    static constexpr const FaustParameterIndex side = kFaustParameterSc_side_rel;
    static constexpr const std::array<float, 5> scale { 10, 0, -10, -20, -30 };
};

template<class M>
class DeEsserThresholdMeterWidget final : public LabKnobWidget
{
    using R = DeEsserMeterReference;

    static constexpr const FaustParameter& kThreshold = kFaustParameters[M::threshold];
    static constexpr const std::array<FaustParameterIndex, 2> kMeters { M::mid, M::side };
    static constexpr const float kTop = kFaustParameters[M::mid].max;
    static constexpr const float kFloor = kFaustParameters[M::mid].min;

    std::array<float, 2> fValues { kFloor, kFloor };

    // The well, between the title and the channel names, centred across the panel with the scale to its left and the
    // triangle to its right.
    struct Geometry {
        float x, wx, wy, ww, wh;

        Geometry(const float w, const float h, const float s) noexcept
        {
            ww = (R::Well::border * 2 + R::Track::width * 2 + R::Track::gap) * s;
            wx = std::round((w - ww) * 0.5f);
            x = wx - (R::Scale::width + R::Scale::gap) * s;
            wy = (R::Title::height + R::Title::gap) * s;
            wh = h - wy - (R::Channel::height + R::Channel::gap) * s;
        }

        // inside the well's border
        [[nodiscard]] float dbToY(const float db, const float s) const noexcept
        {
            const float b = R::Well::border * s;
            return wy + b + (wh - b * 2) * std::clamp((kTop - db) / (kTop - kFloor), 0.f, 1.f);
        }
    };

public:
    explicit DeEsserThresholdMeterWidget(LabWidget* const parent)
        : LabKnobWidget(parent, kParametersMainStart + M::threshold)
    {
        setName(kThreshold.label);
        setDefault(kThreshold.init);
        setRange(kThreshold.min, kThreshold.max);
        setStep(kThreshold.step);
        setValue(kThreshold.init, false);
    }

private:
    void idleCallback() final
    {
        for (size_t i = 0; i < kMeters.size(); ++i)
        {
            if (const float value = std::clamp(fInterface->getParameterValue(kParametersMainStart + kMeters[i]),
                                               kFloor, kTop); d_isNotEqual(fValues[i], value))
            {
                fValues[i] = value;
                repaint();
            }
        }

        LabKnobWidget::idleCallback();
    }

    void onNanoDisplay() final
    {
        const float s = fScaleFactor;
        const float w = getWidth();
        const float h = getHeight();
        const Color& color = Reference::Colors::acc1;

        const Geometry g(w, h, s);

        if (g.wh <= 0.f || g.ww <= 0.f)
            return;

        const float b = R::Well::border * s;
        const float wr = R::Well::borderRadius * s;
        const float tw = R::Track::width * s;
        const std::array<float, 2> trackX { g.wx + b, g.wx + g.ww - b - tw };
        const float ty = g.wy + b;
        const float tb = g.wy + g.wh - b;

        // the title, shrunk to fit the panel if need be
        {
            const float maxWidth = w;
            Rectangle<float> bounds;

            fontFace("regular");
            fontSize(R::Title::fontSize * s);
            textLetterSpacing(R::Title::letterSpacing * s);

            if (const float titleWidth = textBounds(0, 0, M::title, nullptr, bounds); titleWidth > maxWidth)
                fontSize(R::Title::fontSize * s * maxWidth / titleWidth);

            textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
            fillColor(R::Title::color);
            text(w * 0.5f, R::Title::height * 0.5f * s, M::title, nullptr);
            textLetterSpacing(0.f);
        }

        // the well
        beginPath();
        roundedRect(g.wx, g.wy, g.ww, g.wh, wr);
        fillColor(R::Well::backgroundColor);
        fill();

        // the tracks, each cut out of the well's shape so the ends follow its corners
        for (size_t i = 0; i < fValues.size(); ++i)
        {
            const float y = g.dbToY(fValues[i], s);

            if (tb - y < 0.5f)
                continue;

            save();
            scissor(trackX[i], y, tw, tb - y);
            beginPath();
            roundedRect(g.wx + b, ty, g.ww - b * 2, tb - ty, wr - b);
            fillPaint(linearGradient(0, y, 0, tb, color, Color(color, 0.45f)));
            fill();
            restore();
        }

        // inset shading
        beginPath();
        roundedRect(g.wx, g.wy, g.ww, g.wh, wr);
        fillPaint(linearGradient(0, g.wy, 0, g.wy + 4 * s, R::Well::insetShadingColor, Reference::Colors::transparent));
        fill();

        // the scale: ticks across the well, leaving out its ends, and the dB to their left
        fontSize(R::Scale::fontSize * s);
        textAlign(ALIGN_RIGHT | ALIGN_MIDDLE);
        fillColor(R::Scale::color);

        for (const float db : M::scale)
        {
            const float y = g.dbToY(db, s);

            if (d_isNotEqual(db, kTop) && d_isNotEqual(db, kFloor))
            {
                beginPath();
                moveTo(g.wx + b, std::round(y) + 0.5f);
                lineTo(g.wx + g.ww - b, std::round(y) + 0.5f);
                strokeColor(R::Tick::color);
                strokeWidth(1.f);
                stroke();
            }

            char dbstr[8];
            std::snprintf(dbstr, sizeof(dbstr), db > 0.f ? "+%.0f" : "%.0f", db);
            text(g.x + R::Scale::width * s, y, dbstr, nullptr);
        }

        // the channels' names, under their tracks
        {
            const char* const names[2] = { "M", "S" };

            fontSize(R::Channel::fontSize * s);
            textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
            fillColor(R::Channel::color);

            const float y = g.wy + g.wh + (R::Channel::gap + R::Channel::height * 0.5f) * s;
            for (size_t i = 0; i < trackX.size(); ++i)
                text(trackX[i] + tw * 0.5f, y, names[i], nullptr);
        }

        // the threshold: a triangle pointing at the well, and a line across it
        const float y = g.dbToY(getValue(), s);
        const float sw = R::Slider::width * s;
        const float sx = g.wx + g.ww + R::Slider::gap * s;

        beginPath();
        moveTo(sx + sw, y - sw * 0.5f);
        lineTo(sx, y);
        lineTo(sx + sw, y + sw * 0.5f);
        closePath();
        fillColor(color);
        fill();

        beginPath();
        moveTo(g.wx, std::round(y) + 0.5f);
        lineTo(g.wx + g.ww, std::round(y) + 0.5f);
        strokeColor(color);
        strokeWidth(1.f * s);
        stroke();
    }
};

// --------------------------------------------------------------------------------------------------------------------
// A threshold's column: its meter, and under it its number box.

template<class M>
class DeEsserThresholdColumnWidget final : public ReferenceContainerWidget<DeEsserColumnReference, Vertical>
{
    using BaseWidget = ReferenceContainerWidget<DeEsserColumnReference, Vertical>;

    const std::array<std::shared_ptr<LabWidget>, 2> widgets {
        addWidget<DeEsserThresholdMeterWidget<M>, Expanding>(),
        addWidget<DeEsserNumberBoxWidget<Reference::Colors::acc1>, Fixed>(M::threshold),
    };

public:
    explicit DeEsserThresholdColumnWidget(LabWidget* const parent)
        : BaseWidget(parent) {}
};

// The glassy display in the middle column, as glueBus's XY pad: the input spectrum, as vocalDoubler's analyser,
// mbComp5's frequency grid, dB rows on the shelf's scale and a rainbow EQ line of the high shelf as the de-esser
// applies it, with a node at the crossover dragged across to set it, under a pane of glass over the starfield.
// Frequency runs 20 Hz to 20 kHz logarithmically, left to right.

struct DeEsserGlassReference : Reference::Zero {
    // of the pane's corners
    static constexpr const uint borderRadius = 10;
    // the spectrum's scale, in dB
    static constexpr const float spectrumBottom = FFTAnalysis::kSmallestValue;
    static constexpr const float spectrumTop = 0.f;
    static constexpr const Color spectrumColor = Reference::Colors::acc1;
    // The shelf curve: the high shelf's gain across the pane, 0 dB at zeroY of the height down and minGain at the
    // bottom, in sections each blended between the rainbow's colours at its ends, fading out over fade at either end.
    struct Curve {
        static constexpr const float zeroY = 0.4f;
        static constexpr const float minGain = -24.f;
        // the shelf's Q, as in the DSP
        static constexpr const float q = 0.7f;
        static constexpr const uint sections = 64;
        // points along each section
        static constexpr const uint steps = 4;
        static constexpr const float fade = 24.f;
        static constexpr const float width = 1.5f;
        static constexpr const float glowWidth = 4.f;
        static constexpr const float glowAlpha = 0.2f;
    };
    // The frequency grid, as mbComp5's at half its alphas: a line at each of 1 to 9 times every decade, labelled at 1,
    // 2 and 5 times from 50 Hz, in the grid's colour, fading in below the labels and out at the pane's bottom.
    struct Grid {
        static constexpr const Color color = Reference::Widgets::Grid::color;
        static constexpr const float alphaMain = 0.35f;
        static constexpr const float alphaMid = 0.22f;
        static constexpr const float alphaOther = 0.11f;
        struct Label {
            static constexpr const Color color = Reference::Widgets::Grid::Label::color;
            static constexpr const float fontSize = Reference::Widgets::Grid::Label::fontSize;
            // from the pane's top edge
            static constexpr const float inset = 4.f;
        };
        // over which a line fades in and out
        static constexpr const float fade = 8.f;
        // The dB rows, as mbComp5's: dashed, on the shelf curve's scale, from the curve's minimum up to 0 dB every
        // step, labelled at both ends, fading in from the labels. 0 dB is brighter.
        struct Rows {
            static constexpr const float step = 6.f;
            static constexpr const float alphaZero = 0.45f;
            static constexpr const float alpha = 0.3f;
            // between a label and the pane's side, and between it and its line
            static constexpr const float inset = 6.f;
            static constexpr const float padding = 4.f;
            // over which a line fades in past its label's padding
            static constexpr const float fade = 16.f;
            static constexpr const float dash = 3.f;
            static constexpr const float dashPitch = 7.f;
        };
    };
    // The crossover node, unscaled, as glueBus's XY pad point: on the shelf curve at the crossover, larger while
    // dragged, grabbed within grabRadius and dragged across.
    struct Node {
        static constexpr const float radius = 4.f;
        static constexpr const float glowRadius = 9.f;
        static constexpr const float dragScale = 1.3f;
        static constexpr const float grabRadius = 12.f;
        static constexpr const Color coreColor { 1.f, 1.f, 1.f, 1.f };
        static constexpr const MouseCursor cursor = kMouseCursorHand;
    };
    // glueBus's curve colours, from left to right, blended between
    static constexpr const std::array<Color, 5> curveColors {{
        { 255, 191, 203 }, { 255, 223, 173 }, { 210, 253, 211 }, { 190, 241, 255 }, { 218, 193, 243 }
    }};
};

class DeEsserGlassWidget final : public LabWidget,
                                 private IdleCallback
{
    using R = DeEsserGlassReference;

    static constexpr const uint32_t kNumBins = LibreAudioAnalyzerIPC::kNumBins;
    static constexpr const FaustParameter& kCrossover = kFaustParameters[kFaustParameterCrossover_frequency];
    static constexpr const FaustParameter& kReduction = kFaustParameters[kFaustParameterDeess_meter];

    std::array<float, kNumBins> fSpectrum;
    bool fPendingDisplay = false;
    float fCrossover = kCrossover.init;
    float fReduction = 0.f;
    bool fDragging = false;
    bool fHovering = false;
    // between the pointer and the node across, kept while dragging
    float fGrabOffset = 0.f;
    uint fLastClickTime = 0;

public:
    explicit DeEsserGlassWidget(LabWidget* const parent)
        : LabWidget(parent)
    {
        fSpectrum.fill(R::spectrumBottom);
        addIdleCallback(this);
    }

    void setSpectrum(const float values[kNumBins])
    {
        std::memcpy(fSpectrum.data(), values, sizeof(float) * kNumBins);

        if (! fPendingDisplay)
        {
            fPendingDisplay = true;
            repaint();
        }
    }

private:
    // the node's position in the widget
    [[nodiscard]] Point<float> nodePosition() const noexcept
    {
        const float w = getWidth();
        const float h = getHeight();
        const float zeroY = h * R::Curve::zeroY;
        const float db = std::max(R::Curve::minGain, shelfGainDb(fCrossover));
        return { freq2norm(fCrossover) * w, zeroY + (h - zeroY) * db / R::Curve::minGain };
    }

    [[nodiscard]] bool nearNode(const Point<double>& pos) const noexcept
    {
        const Point<float> node = nodePosition();
        const float dx = pos.getX() - node.getX();
        const float dy = pos.getY() - node.getY();
        const float r = R::Node::grabRadius * fScaleFactor;
        return dx * dx + dy * dy <= r * r;
    }

    void setCrossover(const float value)
    {
        const float crossover = std::clamp(std::round(value / kCrossover.step) * kCrossover.step,
                                           kCrossover.min, kCrossover.max);
        if (d_isEqual(fCrossover, crossover))
            return;

        fCrossover = crossover;
        fInterface->parameterControlModified(kParametersMainStart + kFaustParameterCrossover_frequency, crossover);
        repaint();
    }

    bool onMouse(const MouseEvent& ev) final
    {
        if (ev.button != kMouseButtonLeft)
            return LabWidget::onMouse(ev);

        if (ev.press)
        {
            if (! contains(ev.pos) || ! nearNode(ev.pos))
                return LabWidget::onMouse(ev);

            fInterface->parameterControlPressed(kParametersMainStart + kFaustParameterCrossover_frequency);

            // a second click within 300 ms, as knobs take it
            const bool doubleClick = fLastClickTime != 0 && ev.time > fLastClickTime && ev.time - fLastClickTime <= 300;
            fLastClickTime = doubleClick ? 0 : ev.time;

            if (doubleClick)
            {
                setCrossover(kCrossover.init);
                fInterface->parameterControlReleased(kParametersMainStart + kFaustParameterCrossover_frequency);
                return true;
            }

            fDragging = true;
            fGrabOffset = nodePosition().getX() - ev.pos.getX();
            fInterface->setActiveWidget(this);
            fInterface->setCursor(this, R::Node::cursor);
            repaint();
            return true;
        }

        if (! fDragging)
            return LabWidget::onMouse(ev);

        fDragging = false;
        fInterface->parameterControlReleased(kParametersMainStart + kFaustParameterCrossover_frequency);
        fInterface->setActiveWidget(nullptr);
        fHovering = nearNode(ev.pos);
        fInterface->setCursor(this, fHovering ? R::Node::cursor : kMouseCursorArrow);
        repaint();
        return true;
    }

    bool onMotion(const MotionEvent& ev) final
    {
        if (fDragging)
        {
            setCrossover(norm2freq((ev.pos.getX() + fGrabOffset) / getWidth()));
            return true;
        }

        if (const bool hovering = contains(ev.pos) && nearNode(ev.pos); hovering != fHovering)
        {
            fHovering = hovering;
            fInterface->setCursor(this, hovering ? R::Node::cursor : kMouseCursorArrow);
        }

        return LabWidget::onMotion(ev);
    }

    void idleCallback() final
    {
        const float crossover = fDragging
                              ? fCrossover
                              : fInterface->getParameterValue(kParametersMainStart + kFaustParameterCrossover_frequency);
        const float reduction = std::clamp(fInterface->getParameterValue(kParametersMainStart + kFaustParameterDeess_meter),
                                           kReduction.min, kReduction.max);

        if (d_isEqual(fCrossover, crossover) && d_isEqual(fReduction, reduction))
            return;

        fCrossover = crossover;
        fReduction = reduction;

        if (! fPendingDisplay)
        {
            fPendingDisplay = true;
            repaint();
        }
    }

    void onNanoDisplay() final
    {
        fPendingDisplay = false;

        const float r = R::borderRadius * fScaleFactor;

        drawSpectrum(getWidth(), getHeight());
        drawGrid(getWidth(), getHeight());
        drawCurve(getWidth(), getHeight());
        drawNode();
        drawGlass(getWidth(), getHeight(), r);
    }

    // the rainbow's colour at t, from 0 at the left to 1 at the right
    [[nodiscard]] static Color curveColor(const float t, const float alpha) noexcept
    {
        const float p = std::clamp(t, 0.f, 1.f) * (R::curveColors.size() - 1);
        const size_t i = std::min(R::curveColors.size() - 2, static_cast<size_t>(p));
        return Color(Color(R::curveColors[i], R::curveColors[i + 1], p - i), alpha);
    }

    // The high shelf's gain in dB at frequency f, as fi.svf.hs: H(s) = (A²s² + kAs + 1) / (s² + ks + 1), with
    // A = 10^(gain/40), k = 1/Q and s normalised to the corner times sqrt(A).
    [[nodiscard]] float shelfGainDb(const float f) const noexcept
    {
        if (fReduction <= 0.f)
            return 0.f;

        const float a = std::pow(10.f, -fReduction / 40.f);
        const float k = 1.f / R::Curve::q;
        const float w = f / (fCrossover * std::sqrt(a));
        const float w2 = w * w;

        const float numRe = 1.f - a * a * w2;
        const float numIm = k * a * w;
        const float denRe = 1.f - w2;
        const float denIm = k * w;

        return 10.f * std::log10((numRe * numRe + numIm * numIm) / (denRe * denRe + denIm * denIm));
    }

    // The shelf curve across the pane, frequency 20 Hz to 20 kHz logarithmically as the spectrum, with a glow.
    void drawCurve(const float w, const float h)
    {
        const float s = fScaleFactor;
        const float zeroY = h * R::Curve::zeroY;
        const float fade = R::Curve::fade * s;
        static constexpr const uint kPoints = R::Curve::sections * R::Curve::steps;

        std::array<Point<float>, kPoints + 1> points;

        for (uint i = 0; i <= kPoints; ++i)
        {
            const float t = float(i) / kPoints;
            const float db = std::max(R::Curve::minGain, shelfGainDb(20.f * std::pow(1000.f, t)));
            points[i] = { t * w, zeroY + (h - zeroY) * db / R::Curve::minGain };
        }

        // 1 inside, eased to 0 at either end
        const auto fadeAt = [&](const float x) {
            const float c = std::clamp(std::min(x, w - x) / fade, 0.f, 1.f);
            return c * c * (3.f - 2.f * c);
        };

        lineJoin(ROUND);

        // Each section strokes the curve from a section before it to a section after it, clipped to its own strip,
        // so the strips meet edge to edge and no pixel is painted twice.
        for (const auto& [alpha, width] : { std::make_pair(R::Curve::glowAlpha, R::Curve::glowWidth),
                                            std::make_pair(1.f, R::Curve::width) })
        {
            strokeWidth(width * s);

            for (uint i = 0; i < R::Curve::sections; ++i)
            {
                const float x0 = points[i * R::Curve::steps].getX();
                const float x1 = points[(i + 1) * R::Curve::steps].getX();
                const uint first = i > 0 ? (i - 1) * R::Curve::steps : 0;
                const uint last = std::min(kPoints, (i + 2) * R::Curve::steps);

                save();
                intersectScissor(x0, 0, x1 - x0, h);
                beginPath();
                moveTo(points[first].getX(), points[first].getY());
                for (uint j = first + 1; j <= last; ++j)
                    lineTo(points[j].getX(), points[j].getY());
                strokePaint(linearGradient(x0, 0, x1, 0,
                                           curveColor(x0 / w, alpha * fadeAt(x0)),
                                           curveColor(x1 / w, alpha * fadeAt(x1))));
                stroke();
                restore();
            }
        }
    }

    // The spectrum filled from the bottom, fading from the middle of the pane to clear at the bottom, so its bottom
    // corners need no rounding.
    void drawSpectrum(const float w, const float h)
    {
        static constexpr const float kRange = R::spectrumTop - R::spectrumBottom;

        beginPath();
        moveTo(0, h);

        for (uint32_t i = 0; i < kNumBins; ++i)
        {
            const float norm = std::clamp((fSpectrum[i] - R::spectrumBottom) / kRange, 0.f, 1.f);
            lineTo(float(i) / (kNumBins - 1) * w, h - norm * h);
        }

        lineTo(w, h);
        closePath();

        fillPaint(linearGradient(0, h * 0.5f, 0, h, R::spectrumColor.withAlpha(0.45f), R::spectrumColor.withAlpha(0.f)));
        fill();
    }

    void drawGrid(const float w, const float h)
    {
        const float s = fScaleFactor;
        const float fade = R::Grid::fade * s;
        const float labelY = R::Grid::Label::inset * s;
        const float labelHeight = R::Grid::Label::fontSize * s;

        char label[8];

        fillColor(R::Grid::Label::color);
        fontFace("mono");
        fontSize(labelHeight);
        textAlign(ALIGN_CENTER | ALIGN_TOP);
        strokeWidth(s);

        for (int dec = 10; dec < kFreqMax; dec *= 10)
        {
            for (int i = 1; i <= 9; ++i)
            {
                const int freq = i * dec;
                if (freq <= kFreqMin || freq >= kFreqMax)
                    continue;

                const float x = freq2norm(freq) * w;
                float y = 0.f;

                if ((i == 1 || i == 2 || i == 5) && freq >= 50)
                {
                    if (freq >= 1000)
                        std::snprintf(label, sizeof(label), "%dk", freq / 1000);
                    else
                        std::snprintf(label, sizeof(label), "%d", freq);

                    text(x, labelY, label, nullptr);
                    y = labelY + labelHeight;
                }

                const float alpha = i == 1 ? R::Grid::alphaMain : i == 5 ? R::Grid::alphaMid : R::Grid::alphaOther;
                const Color on = R::Grid::color.withAlpha(alpha);
                const Color off = R::Grid::color.withAlpha(0.f);

                beginPath();
                moveTo(x, y);
                lineTo(x, y + fade);
                strokePaint(linearGradient(x, y, x, y + fade, off, on));
                stroke();

                beginPath();
                moveTo(x, y + fade);
                lineTo(x, h - fade);
                strokeColor(on);
                stroke();

                beginPath();
                moveTo(x, h - fade);
                lineTo(x, h);
                strokePaint(linearGradient(x, h - fade, x, h, on, off));
                stroke();
            }
        }

        drawRows(w, h);
    }

    void drawRows(const float w, const float h)
    {
        using G = R::Grid::Rows;

        const float s = fScaleFactor;
        const float zeroY = h * R::Curve::zeroY;
        const float inset = G::inset * s;
        const float padding = G::padding * s;
        const float fade = G::fade * s;
        const float dash = G::dash * s;
        const float pitch = G::dashPitch * s;

        char label[16];

        fillColor(R::Grid::Label::color);
        fontFace("mono");
        fontSize(R::Grid::Label::fontSize * s);
        strokeWidth(s);

        // the curve's minimum is the pane's bottom edge, so the rows start a step above it
        for (float db = R::Curve::minGain + G::step; db <= 0.f; db += G::step)
        {
            const bool isZero = d_isZero(db);

            if (isZero)
                std::snprintf(label, sizeof(label), "0");
            else
                std::snprintf(label, sizeof(label), "-%d", std::abs(d_roundToInt(db)));

            const float y = zeroY + (h - zeroY) * db / R::Curve::minGain;

            textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
            const float sx = text(inset, y, label, nullptr) + padding;
            const float ex = w - sx;

            textAlign(ALIGN_RIGHT | ALIGN_MIDDLE);
            text(w - inset, y, label, nullptr);

            const Color on = R::Grid::color.withAlpha(isZero ? G::alphaZero : G::alpha);
            const Color off = R::Grid::color.withAlpha(0.f);

            for (float x = sx; x < ex; x += pitch)
            {
                const float x1 = std::min(x + dash, ex);
                const float mid = (x + x1) * 0.5f;

                beginPath();
                moveTo(x, y);
                lineTo(x1, y);
                if (mid < w * 0.5f)
                    strokePaint(linearGradient(sx, y, sx + fade, y, off, on));
                else
                    strokePaint(linearGradient(ex - fade, y, ex, y, on, off));
                stroke();
            }
        }
    }

    // the node, in the curve's colour there with a white core and a glow
    void drawNode()
    {
        const float s = fScaleFactor * (fDragging ? R::Node::dragScale : 1.f);
        const Point<float> node = nodePosition();
        const float x = node.getX();
        const float y = node.getY();
        const Color color = curveColor(x / getWidth(), 1.f);

        beginPath();
        circle(x, y, R::Node::glowRadius * s);
        fillPaint(radialGradient(x, y, R::Node::radius * s, R::Node::glowRadius * s,
                                 Color(color, 0.5f), Color(color, 0.f)));
        fill();

        beginPath();
        circle(x, y, R::Node::radius * s);
        fillColor(color);
        fill();

        beginPath();
        circle(x, y, R::Node::radius * 0.5f * s);
        fillColor(R::Node::coreColor);
        fill();
    }

    // A pane of glass: a gloss over the top whose lower edge sweeps up to the right, a faint diagonal sheen across
    // the middle, kept clear of the corners, a rim lit at the top and fading down, and a bright line just inside the
    // top edge.
    void drawGlass(const float w, const float h, const float r)
    {
        // gloss
        beginPath();
        moveTo(0, h * 0.46f);
        lineTo(0, r);
        arcTo(0, 0, r, 0, r);
        lineTo(w - r, 0);
        arcTo(w, 0, w, r, r);
        lineTo(w, h * 0.26f);
        quadTo(w * 0.45f, h * 0.30f, 0, h * 0.46f);
        closePath();
        fillPaint(linearGradient(0, 0, 0, h * 0.46f, Color(1.f, 1.f, 1.f, 0.09f), Color(1.f, 1.f, 1.f, 0.015f)));
        fill();

        // sheen
        beginPath();
        moveTo(w * 0.60f, 0);
        lineTo(w * 0.70f, 0);
        lineTo(w * 0.56f, h);
        lineTo(w * 0.46f, h);
        closePath();
        fillPaint(linearGradient(0, 0, 0, h, Color(1.f, 1.f, 1.f, 0.045f), Color(1.f, 1.f, 1.f, 0.005f)));
        fill();

        // rim
        beginPath();
        roundedRect(0.5f, 0.5f, w - 1.f, h - 1.f, r - 0.5f);
        strokePaint(linearGradient(0, 0, 0, h, Color(1.f, 1.f, 1.f, 0.22f), Color(1.f, 1.f, 1.f, 0.04f)));
        strokeWidth(1.f);
        stroke();

        // top edge highlight
        beginPath();
        moveTo(r, 1.5f);
        lineTo(w - r, 1.5f);
        strokePaint(linearGradient(r, 0, w - r, 0, Color(1.f, 1.f, 1.f, 0.08f), Color(1.f, 1.f, 1.f, 0.32f)));
        strokeWidth(1.f);
        stroke();
    }
};

// Mode, one box across the middle column's lower group.
using DeEsserModeGroupWidget = DeEsserGroupWidget<widgets::Mode>;

// --------------------------------------------------------------------------------------------------------------------
// The display, above the controls: three columns over the starfield, the absolute threshold at the left, the relative
// threshold at the right and between them the glassy display over Mode's group, which is as tall as a control group.

class DeEsserDisplayWidget final : public LabWidget
{
    // between the columns and the display's sides and top, as the controls below are in from the stage's edges
    static constexpr const uint kPanelMargin = DeEsserPageReference::margin;
    // between the columns side by side, and between them and the controls below
    static constexpr const uint kPanelGap = DeEsserPageReference::padding;

    using AbsoluteColumn = DeEsserThresholdColumnWidget<DeEsserAbsoluteMeter>;
    using RelativeColumn = DeEsserThresholdColumnWidget<DeEsserRelativeMeter>;

    const std::unique_ptr<AbsoluteColumn> fAbsolute { new AbsoluteColumn(this) };
    const std::unique_ptr<DeEsserGlassWidget> fGlass { new DeEsserGlassWidget(this) };
    const std::unique_ptr<DeEsserModeGroupWidget> fMode { new DeEsserModeGroupWidget(this) };
    const std::unique_ptr<RelativeColumn> fRelative { new RelativeColumn(this) };

public:
    explicit DeEsserDisplayWidget(LabWidget* const parent)
        : LabWidget(parent) {}

    void setSpectrum(const float values[LibreAudioAnalyzerIPC::kNumBins])
    {
        fGlass->setSpectrum(values);
    }

private:
    // the columns draw themselves
    void onNanoDisplay() final {}

    // the columns follow the display
    void layout()
    {
        const int margin = d_roundToIntPositive(kPanelMargin * fScaleFactor);
        const int gap = d_roundToIntPositive(kPanelGap * fScaleFactor);
        const int columnWidth = d_roundToIntPositive(kMeterColumnWidth * fScaleFactor);
        const int x = getAbsoluteX();
        const int y = getAbsoluteY() + margin;
        const int w = static_cast<int>(getWidth());
        const int h = std::max(1, static_cast<int>(getHeight()) - margin - gap);

        // as widgets, their layouts having a setSize and setAbsolutePos of their own
        LabWidget& absolute = *fAbsolute;
        absolute.setSize(columnWidth, h);
        absolute.setAbsolutePos(x + margin, y);

        LabWidget& relative = *fRelative;
        relative.setSize(columnWidth, h);
        relative.setAbsolutePos(x + w - margin - columnWidth, y);

        // the middle column: Mode's group as tall as a control group, the glassy display taking the rest
        const int middleX = x + margin + columnWidth + gap;
        const int middleWidth = std::max(1, w - (margin + columnWidth + gap) * 2);
        const int bottomHeight = d_roundToIntPositive(DeEsserGroupReference::height * fScaleFactor);
        const int topHeight = std::max(1, h - bottomHeight - gap);

        fGlass->setSize(middleWidth, topHeight);
        fGlass->setAbsolutePos(middleX, y);

        LabWidget& mode = *fMode;
        mode.setSize(middleWidth, bottomHeight);
        mode.setAbsolutePos(middleX, y + topHeight + gap);
    }

    void onPositionChanged(const PositionChangedEvent& ev) final
    {
        LabWidget::onPositionChanged(ev);
        layout();
    }

    void onResize(const ResizeEvent& ev) final
    {
        LabWidget::onResize(ev);
        layout();
    }
};

// --------------------------------------------------------------------------------------------------------------------
// The stage: the starfield, the display above the controls and the control row at the bottom. Easy and Expert pages
// show the same controls for now.

class DeEsserStageWidget final : public StageWidget<DeEsserPageWidget, DeEsserPageWidget>
{
    using BaseWidget = StageWidget<DeEsserPageWidget, DeEsserPageWidget>;

    const std::shared_ptr<DeEsserDisplayWidget> fDisplay { addGraphWidget<DeEsserDisplayWidget>() };

public:
    explicit DeEsserStageWidget(LabWidget* const parent)
        : BaseWidget(parent) {}

    void setSpectrum(const float values[LibreAudioAnalyzerIPC::kNumBins])
    {
        fDisplay->setSpectrum(values);
    }

    Size<uint> getGraphSize() const noexcept final
    {
        const LabMetrics<R> m { this };
        const LabMetrics<DeEsserPageReference> pm { this };

        const uint space = m.border + pm.margin + getEasyWidget()->getLastWidgetHeight();

        return Size<uint>(getWidth() - m.border * 2, std::max(1u, getHeight() - std::min(getHeight(), space)));
    }
};

using DeEsserMainArea = MainAreaContainerWidget<DeEsserStageWidget>;
using DeEsserRootWidget = RootWidget<TopBar, DeEsserMainArea>;

// --------------------------------------------------------------------------------------------------------------------
// The plugin UI: the starfield behind the stage, with the input spectrum passed to the glassy display.

class DeEsserUI : public LibreAudioBaseUI
{
public:
    DeEsserUI()
    {
        const std::list<std::shared_ptr<ShaderWidgetInterface>> shaders {
            std::make_shared<ShaderBackgroundWidget>(this, this),
            std::make_shared<BotShaderWidget<SHADER_CLOUDSTARFIELD>>(this, this),
        };

        createRootWidget<DeEsserRootWidget>();

        DeEsserRootWidget* const root = static_cast<DeEsserRootWidget*>(fRootWidget.get());
        root->enableShaders(shaders);
        fStage = root->getMainAreaWidget()->getStageWidget();
    }

private:
    std::shared_ptr<DeEsserStageWidget> fStage;

   #if LIBREAUDIO_WANT_GRAPH_ANALYZER
    void audioGraphReceived(const float values[LibreAudioAnalyzerIPC::kNumBins]) final
    {
        fStage->setSpectrum(values);
    }
   #endif
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

UI* createUI()
{
    return new LibreAudio::DeEsserUI();
}

END_NAMESPACE_DISTRHO
