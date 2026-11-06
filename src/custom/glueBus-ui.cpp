// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/containers/container.hpp"
#include "ui/containers/main-area.hpp"
#include "ui/containers/top-bar.hpp"
#include "ui/containers/ui.hpp"
#include "ui/widgets/number-box.hpp"
#include "ui/widgets/power-box.hpp"

#include "LibreAudioParameters.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>

// --------------------------------------------------------------------------------------------------------------------

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

static constexpr const uint kCommonMargin = 8;
static constexpr const uint kCommonPadding = 6;
static constexpr const uint kNumberBoxHeight = 50;

template<const Color& kAccentColor>
using GlueBusPowerBoxWidget = PowerBoxWidget<kAccentColor, kNumberBoxHeight>;

template<const Color& kAccentColor>
using GlueBusNumberBoxWidget = NumberBoxWidget<kAccentColor, kNumberBoxHeight>;

// --------------------------------------------------------------------------------------------------------------------

namespace widgets {
    using Link = ParameterWidgetPair<kFaustParameterLink, GlueBusNumberBoxWidget<Reference::Colors::acc4>>;
    using MidSide = ParameterWidgetPair<kFaustParameterMid_side, GlueBusPowerBoxWidget<Reference::Colors::acc4>>;

    using Drive = ParameterWidgetPair<kFaustParameterDrive, GlueBusNumberBoxWidget<Reference::Colors::acc1>>;
    using CompSat = ParameterWidgetPair<kFaustParameterComp_sat, GlueBusNumberBoxWidget<Reference::Colors::acc1>>;

    using Makeup = ParameterWidgetPair<kFaustParameterMakeup, GlueBusNumberBoxWidget<Reference::Colors::acc6>>;
    using DryWet = ParameterWidgetPair<kFaustParameterDrywet, GlueBusNumberBoxWidget<Reference::Colors::acc6>>;

    using LowFreq = ParameterWidgetPair<kFaustParameterLow_freq, GlueBusNumberBoxWidget<Reference::Colors::acc3>>;
    using LowAttack = ParameterWidgetPair<kFaustParameterLow_attack, GlueBusNumberBoxWidget<Reference::Colors::acc3>>;
    using LowSustain = ParameterWidgetPair<kFaustParameterLow_sustain, GlueBusNumberBoxWidget<Reference::Colors::acc3>>;

    using HighFreq = ParameterWidgetPair<kFaustParameterHigh_freq, GlueBusNumberBoxWidget<Reference::Colors::acc5>>;
    using HighAttack = ParameterWidgetPair<kFaustParameterHigh_attack, GlueBusNumberBoxWidget<Reference::Colors::acc5>>;
    using HighSustain = ParameterWidgetPair<kFaustParameterHigh_sustain, GlueBusNumberBoxWidget<Reference::Colors::acc5>>;
}

// --------------------------------------------------------------------------------------------------------------------

struct GlueBusGroupReference : Reference::Zero {
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
using GlueBusGroupWidget = StaticParameterContainerWidget<GlueBusGroupReference, Horizontal, Expanding, Args...>;

// --------------------------------------------------------------------------------------------------------------------

struct GlueBusGroupsContainerReference : Reference::Zero {
    static constexpr const uint padding = kCommonPadding;
    static constexpr const uint height = GlueBusGroupReference::height + (border + margin) * 2;
};

// Channels, compressor and output, side by side.
using GlueBusMainGroupsWidget = StaticContainerPairWidget<
    GlueBusGroupsContainerReference,
    Horizontal,
    SizeHintWidgetPair<Expanding, GlueBusGroupWidget<widgets::Link, widgets::MidSide>>,
    SizeHintWidgetPair<Expanding, GlueBusGroupWidget<widgets::Drive, widgets::CompSat>>,
    SizeHintWidgetPair<Expanding, GlueBusGroupWidget<widgets::Makeup, widgets::DryWet>>
>;

// --------------------------------------------------------------------------------------------------------------------
// A transient shelf's gain meter, across its group above the number boxes: a well filling from 0 dB in the middle,
// left for a cut and right for a boost, to range either side, with a tick every dB.

struct GlueBusTransientMeterReference : Reference::Zero {
    static constexpr const uint height = 12;
    static constexpr const float range = 3.f;
    static constexpr const Color backgroundColor = Reference::Colors::track;
    static constexpr const Color insetShadingColor { 0.f, 0.f, 0.f, 0.55f };
    static constexpr const uint border = 1;
    static constexpr const uint borderRadius = 4;
    static constexpr const Color tickColor { Reference::Colors::ink3, 0.15f };
    static constexpr const Color centreColor { Reference::Colors::ink3, 0.5f };
};

template<FaustParameterIndex kMeter, const Color& kColor>
class GlueBusTransientMeterWidget final : public LabWidget,
                                          private IdleCallback
{
    using R = GlueBusTransientMeterReference;

    float fValue = 0.f;

public:
    explicit GlueBusTransientMeterWidget(LabWidget* const parent)
        : LabWidget(parent)
    {
        addIdleCallback(this);
        updateSize(false);
    }

private:
    void idleCallback() final
    {
        if (const float value = std::clamp(fInterface->getParameterValue(kParametersMainStart + kMeter),
                                           -R::range, R::range); d_isNotEqual(fValue, value))
        {
            fValue = value;
            repaint();
        }
    }

    void onNanoDisplay() final
    {
        const float s = fScaleFactor;
        const float w = getWidth();
        const float h = getHeight();
        const float b = R::border * s;
        const float r = R::borderRadius * s;
        const float tx = b;
        const float tw = w - b * 2;

        // the track area, inside the well's border, -range at the left
        const auto dbToX = [&](const float db) {
            return tx + tw * (db + R::range) / (R::range * 2);
        };

        // the well
        beginPath();
        roundedRect(0, 0, w, h, r);
        fillColor(R::backgroundColor);
        fill();

        // the fill, from the middle to the value, cut out of the well's shape so its end follows the corners
        const float xc = dbToX(0.f);
        const float xv = dbToX(fValue);
        const float x0 = std::min(xc, xv);
        const float x1 = std::max(xc, xv);

        if (x1 - x0 >= 0.5f)
        {
            save();
            scissor(x0, b, x1 - x0, h - b * 2);
            beginPath();
            roundedRect(b, b, tw, h - b * 2, r - b);
            fillPaint(linearGradient(xv, 0, xc, 0, kColor, Color(kColor, 0.45f)));
            fill();
            restore();
        }

        // inset shading
        beginPath();
        roundedRect(0, 0, w, h, r);
        fillPaint(linearGradient(0, 0, 0, 3 * s, R::insetShadingColor, Reference::Colors::transparent));
        fill();

        // a tick every dB, leaving out the ends, the middle one brighter
        strokeWidth(1.f);
        for (float db = 1.f - R::range; db < R::range - 0.001f; db += 1.f)
        {
            const float x = std::round(dbToX(db)) + 0.5f;

            beginPath();
            moveTo(x, b);
            lineTo(x, h - b);
            strokeColor(std::abs(db) < 0.001f ? R::centreColor : R::tickColor);
            stroke();
        }
    }

    void updateSize(const bool updateChildren) final
    {
        setHeight(d_roundToUnsignedInt(R::height * fScaleFactor));
        LabWidget::updateSize(updateChildren);
    }
};

// --------------------------------------------------------------------------------------------------------------------
// A transient shelf's group: its gain meter, and under it the number boxes.

struct GlueBusTransientRowReference : Reference::Zero {
    static constexpr const uint padding = kCommonPadding;
    static constexpr const uint height = kNumberBoxHeight;
};

struct GlueBusTransientGroupReference : GlueBusGroupReference {
    static constexpr const uint height = GlueBusTransientMeterReference::height + padding + kNumberBoxHeight
                                       + (border + margin) * 2;
};

template<class Meter, typename... Args>
class GlueBusTransientGroupWidget final : public ReferenceContainerWidget<GlueBusTransientGroupReference, Vertical>
{
    using BaseWidget = ReferenceContainerWidget<GlueBusTransientGroupReference, Vertical>;
    using Row = StaticParameterContainerWidget<GlueBusTransientRowReference, Horizontal, Expanding, Args...>;

    const std::array<std::shared_ptr<LabWidget>, 2> widgets {
        addWidget<Meter, Fixed>(),
        addWidget<Row, Fixed>(),
    };

public:
    explicit GlueBusTransientGroupWidget(LabWidget* const parent)
        : BaseWidget(parent) {}
};

struct GlueBusTransientGroupsContainerReference : GlueBusGroupsContainerReference {
    static constexpr const uint height = GlueBusTransientGroupReference::height + (border + margin) * 2;
};

// Low and high transient shelves, side by side.
using GlueBusTransientGroupsWidget = StaticContainerPairWidget<
    GlueBusTransientGroupsContainerReference,
    Horizontal,
    SizeHintWidgetPair<Expanding, GlueBusTransientGroupWidget<
        GlueBusTransientMeterWidget<kFaustParameterLow_transient, Reference::Colors::acc3>,
        widgets::LowAttack, widgets::LowSustain, widgets::LowFreq>>,
    SizeHintWidgetPair<Expanding, GlueBusTransientGroupWidget<
        GlueBusTransientMeterWidget<kFaustParameterHigh_transient, Reference::Colors::acc5>,
        widgets::HighAttack, widgets::HighSustain, widgets::HighFreq>>
>;

// --------------------------------------------------------------------------------------------------------------------

struct GlueBusPageReference : Reference::Zero {
    static constexpr const uint margin = 12;
    static constexpr const uint padding = 8;
};

// The control rows at the bottom of the stage, the space above them left to the XY pad.
class GlueBusPageWidget final : public ReferenceContainerWidget<GlueBusPageReference, Vertical>
{
    using BaseWidget = ReferenceContainerWidget<GlueBusPageReference, Vertical>;

    const std::array<std::shared_ptr<LabWidget>, 3> widgets {
        addSpacer(),
        addWidget<GlueBusMainGroupsWidget, Fixed>(),
        addWidget<GlueBusTransientGroupsWidget, Fixed>(),
    };

public:
    explicit GlueBusPageWidget(LabWidget* const parent)
        : BaseWidget(parent) {}

    // both rows and the padding between them
    [[nodiscard]] uint getControlsHeight() const noexcept
    {
        const LabMetrics<GlueBusPageReference> m { this };
        return widgets[1]->getHeight() + m.padding + widgets[2]->getHeight();
    }
};

// --------------------------------------------------------------------------------------------------------------------
// The panels floating over the starfield: dark and see-through, lit along the top edge.

struct GlueBusFloatingReference : Reference::Zero {
    static constexpr const uint borderRadius = 10;
    static constexpr const Color backgroundColor { Reference::Colors::bg2, 0.75f };
};

static void drawFloatingPanel(NanoVG& vg, const float w, const float h, const float scaleFactor, const Color& color)
{
    const float r = GlueBusFloatingReference::borderRadius * scaleFactor;

    vg.beginPath();
    vg.roundedRect(0, 0, w, h, r);
    vg.fillColor(color);
    vg.fill();

    vg.beginPath();
    vg.roundedRect(0.5f, 0.5f, w - 1.f, h - 1.f, r - 0.5f);
    vg.strokePaint(vg.linearGradient(0, 0, 0, h, Color(1.f, 1.f, 1.f, 0.14f), Color(1.f, 1.f, 1.f, 0.03f)));
    vg.strokeWidth(1.f);
    vg.stroke();
}

// --------------------------------------------------------------------------------------------------------------------
// The XY pad, drawn as the compressor's curve display: Comp / Sat across, Drive up. A panel under a pane of glass,
// with a grid labelled on its lines, wide meters rising from the bottom, compression in the left half and
// saturation in the right, rainbow lines across and down through the point, fading out from it toward the edges,
// dotted lines from the point down and to the left, and the point in the curve's colour with a white core. Clicking or dragging sets both; double-clicking
// resets both.

struct GlueBusXYPadReference : GlueBusFloatingReference {
    // see-through, the starfield showing behind the plot
    static constexpr const Color backgroundColor { Reference::Colors::bg0, 0.f };
    // grid steps, in each parameter's units
    static constexpr const float gridStepX = 0.5f;
    static constexpr const float gridStepY = 6.f;
    static constexpr const Color gridColor { 1.f, 1.f, 1.f, 0.06f };
    struct GridLabel {
        static constexpr const Color color { 1.f, 1.f, 1.f, 0.22f };
        static constexpr const float fontSize = 9.f;
        // from the panel's edge
        static constexpr const float inset = 4.f;
        // between a label and its line's end
        static constexpr const float gap = 3.f;
        // how far a line takes to fade out toward its label
        static constexpr const float fade = 24.f;
    };
    static constexpr const Color crossColor { 1.f, 1.f, 1.f, 0.35f };
    // the rainbow lines through the point, in this many sections along, half each side of the point
    static constexpr const uint lineSections = 32;
    // the cross lines are dotted: a dot this long, every this far, unscaled
    static constexpr const float crossDot = 1.f;
    static constexpr const float crossDotStep = 3.f;
    // mbComp5's curve colours, from left to right, blended between
    static constexpr const std::array<Color, 5> curveColors {{
        { 255, 191, 203 }, { 255, 223, 173 }, { 210, 253, 211 }, { 190, 241, 255 }, { 218, 193, 243 }
    }};
    // The meters, unscaled: two channels a pair, compression left and saturation right, filled in the curve's colours
    // across, fading from alpha at the value to clear at the bottom. Their maximum is at the top of the pad.
    struct Meter {
        static constexpr const float edge = 12.f;
        static constexpr const float pairGap = 24.f;
        static constexpr const float channelGap = 4.f;
        // a column of the fill, each in the curve's colour at its middle
        static constexpr const float column = 2.f;
        // of the corners
        static constexpr const float borderRadius = 4.f;
        // at the value, fading to 0 at the bottom
        static constexpr const float alpha = 0.8f;
        // over each pair, centred, at the top of the pad
        struct Label {
            static constexpr const float alpha = 0.45f;
            static constexpr const float fontSize = 10.f;
            static constexpr const float letterSpacing = fontSize * 0.08f;
            // from the pad's top edge
            static constexpr const float inset = 8.f;
        };
    };
    // the point, unscaled, larger while dragged
    static constexpr const float pointRadius = 4.f;
    static constexpr const float pointGlowRadius = 9.f;
    static constexpr const float pointDragScale = 1.3f;
    static constexpr const Color pointColor { 1.f, 1.f, 1.f, 1.f };
    static constexpr const MouseCursor cursor = kMouseCursorHand;
};

class GlueBusXYPadWidget final : public LabWidget,
                                 private IdleCallback
{
    using R = GlueBusXYPadReference;

    static constexpr const FaustParameterIndex kParamX = kFaustParameterComp_sat;
    static constexpr const FaustParameterIndex kParamY = kFaustParameterDrive;
    static constexpr const FaustParameter& kX = kFaustParameters[kParamX];
    static constexpr const FaustParameter& kY = kFaustParameters[kParamY];

    // compression 1 and 2, then saturation 1 and 2
    static constexpr const std::array<FaustParameterIndex, 4> kMeters {
        kFaustParameterReduction_1, kFaustParameterReduction_2, kFaustParameterSaturation_1, kFaustParameterSaturation_2
    };
    static constexpr const float kMeterMin = kFaustParameters[kFaustParameterReduction_1].min;
    static constexpr const float kMeterMax = kFaustParameters[kFaustParameterReduction_1].max;

    float fValueX = kX.init;
    float fValueY = kY.init;
    std::array<float, kMeters.size()> fMeters { kMeterMin, kMeterMin, kMeterMin, kMeterMin };
    bool fDragging = false;
    uint fLastClickTime = 0;

public:
    explicit GlueBusXYPadWidget(LabWidget* const parent)
        : LabWidget(parent)
    {
        addIdleCallback(this);
    }

    [[nodiscard]] float getBorderRadius() const noexcept
    {
        return R::borderRadius * fScaleFactor;
    }

private:

    // the curve's colour at t, from 0 at the left to 1 at the right
    [[nodiscard]] static Color curveColor(const float t, const float alpha) noexcept
    {
        const float p = std::clamp(t, 0.f, 1.f) * (R::curveColors.size() - 1);
        const size_t i = std::min(R::curveColors.size() - 2, static_cast<size_t>(p));
        return Color(Color(R::curveColors[i], R::curveColors[i + 1], p - i), alpha);
    }

    [[nodiscard]] static float normX(const float value) noexcept
    {
        return (value - kX.min) / (kX.max - kX.min);
    }

    [[nodiscard]] static float normY(const float value) noexcept
    {
        return (value - kY.min) / (kY.max - kY.min);
    }

    // a value from a normalized position, on the parameter's steps
    [[nodiscard]] static float fromNorm(const FaustParameter& param, const float norm) noexcept
    {
        const float value = param.min + std::clamp(norm, 0.f, 1.f) * (param.max - param.min);
        return std::clamp(param.min + std::round((value - param.min) / param.step) * param.step, param.min, param.max);
    }

    void setParameter(const FaustParameterIndex index, float& current, const float value)
    {
        if (d_isEqual(current, value))
            return;

        current = value;
        fInterface->parameterControlModified(kParametersMainStart + index, value);
    }

    // both parameters from a position in the widget
    void setFromPosition(const Point<double>& pos)
    {
        setParameter(kParamX, fValueX, fromNorm(kX, pos.getX() / getWidth()));
        setParameter(kParamY, fValueY, fromNorm(kY, 1.f - pos.getY() / getHeight()));
        repaint();
    }

    void idleCallback() final
    {
        for (size_t i = 0; i < kMeters.size(); ++i)
        {
            if (const float value = std::clamp(fInterface->getParameterValue(kParametersMainStart + kMeters[i]),
                                               kMeterMin, kMeterMax); d_isNotEqual(fMeters[i], value))
            {
                fMeters[i] = value;
                repaint();
            }
        }

        if (fDragging)
            return;

        const float x = fInterface->getParameterValue(kParametersMainStart + kParamX);
        const float y = fInterface->getParameterValue(kParametersMainStart + kParamY);

        if (d_isEqual(fValueX, x) && d_isEqual(fValueY, y))
            return;

        fValueX = x;
        fValueY = y;
        repaint();
    }

    bool onMouse(const MouseEvent& ev) final
    {
        if (ev.button != kMouseButtonLeft)
            return LabWidget::onMouse(ev);

        if (ev.press)
        {
            if (! contains(ev.pos))
                return LabWidget::onMouse(ev);

            fInterface->parameterControlPressed(kParametersMainStart + kParamX);
            fInterface->parameterControlPressed(kParametersMainStart + kParamY);

            // a second click within 300 ms, as knobs take it
            const bool doubleClick = fLastClickTime != 0 && ev.time > fLastClickTime && ev.time - fLastClickTime <= 300;
            fLastClickTime = doubleClick ? 0 : ev.time;

            if (doubleClick)
            {
                setParameter(kParamX, fValueX, kX.init);
                setParameter(kParamY, fValueY, kY.init);
                fInterface->parameterControlReleased(kParametersMainStart + kParamX);
                fInterface->parameterControlReleased(kParametersMainStart + kParamY);
                repaint();
                return true;
            }

            fDragging = true;
            fInterface->setActiveWidget(this);
            fInterface->setCursor(this, R::cursor);
            setFromPosition(ev.pos);
            return true;
        }

        if (! fDragging)
            return LabWidget::onMouse(ev);

        fDragging = false;
        fInterface->parameterControlReleased(kParametersMainStart + kParamX);
        fInterface->parameterControlReleased(kParametersMainStart + kParamY);
        fInterface->setActiveWidget(nullptr);
        fInterface->setCursor(this, kMouseCursorArrow);
        repaint();
        return true;
    }

    bool onMotion(const MotionEvent& ev) final
    {
        if (fDragging)
        {
            setFromPosition(ev.pos);
            return true;
        }

        return LabWidget::onMotion(ev);
    }

    void onNanoDisplay() final
    {
        drawPad(getWidth(), getHeight(), getBorderRadius());
    }

    void drawPad(const float w, const float h, const float r)
    {
        const float s = fScaleFactor;

        drawFloatingPanel(*this, w, h, s, R::backgroundColor);

        // the plot fills the panel, edge to edge
        const float px = 0.f;
        const float py = 0.f;
        const float pw = w;
        const float ph = h;

        const auto toX = [&](const float value) {
            return px + normX(value) * pw;
        };
        const auto toY = [&](const float value) {
            return py + ph - normY(value) * ph;
        };

        save();
        intersectScissor(px, py, pw, ph);

        // Grid across and down, Drive centred on its line down the left. Each line fades out at both ends, toward
        // its label or the panel's edge.
        fontFace("regular");
        fontSize(R::GridLabel::fontSize * s);
        strokeWidth(1.f);

        const float inset = R::GridLabel::inset * s;
        const float gap = R::GridLabel::gap * s;
        const float fade = R::GridLabel::fade * s;
        const Color clear { R::gridColor, 0.f };

        // a grid line from start to end, across at y = at or down at x = at, in two halves each fading out at its end
        const auto gridLine = [&](const bool across, const float at, const float start, const float end) {
            const float mid = (start + end) * 0.5f;
            const auto point = [&](const float v) {
                across ? lineTo(v, at) : lineTo(at, v);
            };
            const auto gradient = [&](const float from, const float to) {
                return across ? linearGradient(from, 0, to, 0, clear, R::gridColor)
                              : linearGradient(0, from, 0, to, clear, R::gridColor);
            };

            beginPath();
            across ? moveTo(start, at) : moveTo(at, start);
            point(mid);
            strokePaint(gradient(start, start + fade));
            stroke();

            beginPath();
            across ? moveTo(mid, at) : moveTo(at, mid);
            point(end);
            strokePaint(gradient(end, end - fade));
            stroke();
        };

        char valuestr[8];
        Rectangle<float> bounds;

        for (float v = kY.min + R::gridStepY; v < kY.max - 0.001f; v += R::gridStepY)
        {
            const float gy = std::round(toY(v)) + 0.5f;

            std::snprintf(valuestr, sizeof(valuestr), "%.0f", v);
            textBounds(0, 0, valuestr, nullptr, bounds);

            // across, from the label at the left
            gridLine(true, gy, px + inset + bounds.getWidth() + gap, px + pw);

            fillColor(R::GridLabel::color);
            textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
            text(px + inset, gy, valuestr, nullptr);
        }

        for (float v = kX.min + R::gridStepX; v < kX.max - 0.001f; v += R::gridStepX)
        {
            const float gx = std::round(toX(v)) + 0.5f;

            // down, the whole height
            gridLine(false, gx, py, py + ph);
        }

        // the meters, each in columns fading from the curve's colour at the value to clear at the pad's bottom
        {
            const float edge = R::Meter::edge * s;
            const float pairGap = R::Meter::pairGap * s;
            const float channelGap = R::Meter::channelGap * s;
            const float column = std::max(1.f, std::round(R::Meter::column * s));
            const float bw = std::max(1.f, (pw - edge * 2 - pairGap - channelGap * 2) / 4);

            const auto barLeft = [&](const size_t i) {
                return std::round(px + edge + bw * i + channelGap * (i - i / 2) + pairGap * (i / 2));
            };
            const auto barTop = [&](const size_t i) {
                return py + ph * (1.f - (fMeters[i] - kMeterMin) / (kMeterMax - kMeterMin));
            };

            for (size_t i = 0; i < fMeters.size(); ++i)
            {
                const float left = barLeft(i);
                const float right = std::round(left + bw);
                const float top = barTop(i);

                if (py + ph - top < 0.5f)
                    continue;

                // the corners are rounded, drawn there in columns a pixel wide, each cut to the corners' arc
                const float bh = py + ph - top;
                const float cr = std::min(R::Meter::borderRadius * s, std::min(bh, right - left) * 0.5f);

                for (float x = left; x < right;)
                {
                    const float fromSide = std::min(x - left, right - x - 1.f);
                    const bool inCorner = fromSide < cr;
                    const float cw = std::min(inCorner ? 1.f : column, right - x);
                    const float t = (x + cw * 0.5f - px) / pw;

                    float dy = 0.f;
                    if (inCorner)
                    {
                        const float dx = cr - (fromSide + 0.5f);
                        dy = cr - std::sqrt(std::max(0.f, cr * cr - dx * dx));
                    }

                    beginPath();
                    rect(x, top + dy, cw, bh - dy * 2);
                    fillPaint(linearGradient(0, top, 0, py + ph,
                                             curveColor(t, R::Meter::alpha), curveColor(t, 0.f)));
                    fill();

                    x += cw;
                }
            }

            // the pairs' names, at the top of the pad over them, in the curve's colour at their middle
            fontFace("regular");
            fontSize(R::Meter::Label::fontSize * s);
            textLetterSpacing(R::Meter::Label::letterSpacing * s);
            textAlign(ALIGN_CENTER | ALIGN_TOP);

            const char* const names[2] = { "COMPRESSION", "SATURATION" };
            const float labelTop = py + R::Meter::Label::inset * s;

            for (size_t pair = 0; pair < 2; ++pair)
            {
                const float cx = (barLeft(pair * 2) + barLeft(pair * 2 + 1) + bw) * 0.5f;

                fillColor(curveColor((cx - px) / pw, R::Meter::Label::alpha));
                text(cx, labelTop, names[pair], nullptr);
            }

            textLetterSpacing(0.f);
        }

        // the meters' gain reduction down the right, every dB between the bottom and the top
        fontSize(R::GridLabel::fontSize * s);
        textAlign(ALIGN_RIGHT | ALIGN_MIDDLE);
        fillColor(R::GridLabel::color);

        for (float db = kMeterMin + 1.f; db < kMeterMax - 0.001f; db += 1.f)
        {
            std::snprintf(valuestr, sizeof(valuestr), "%.0f", db);
            text(px + pw - inset, py + ph * (1.f - (db - kMeterMin) / (kMeterMax - kMeterMin)), valuestr, nullptr);
        }

        const float pointX = toX(fValueX);
        const float pointY = toY(fValueY);

        // dotted lines from the point down to the bottom and across to the left, fading out at the edges as the grid
        // does, the dots on whole pixels from the point
        {
            const float tx = std::round(pointX);
            const float ty = std::round(pointY);
            const float dot = std::max(1.f, std::round(R::crossDot * s));
            const float step = std::max(dot + 1.f, std::round(R::crossDotStep * s));
            const Color crossClear { R::crossColor, 0.f };

            strokeWidth(1.f);
            lineCap(BUTT);

            beginPath();
            for (float y = ty; y < py + ph; y += step)
            {
                moveTo(tx + 0.5f, y);
                lineTo(tx + 0.5f, y + dot);
            }
            strokePaint(linearGradient(0, py + ph - fade, 0, py + ph, R::crossColor, crossClear));
            stroke();

            beginPath();
            for (float x = tx; x > px; x -= step)
            {
                moveTo(x, ty + 0.5f);
                lineTo(x - dot, ty + 0.5f);
            }
            strokePaint(linearGradient(px + fade, 0, px, 0, R::crossColor, crossClear));
            stroke();
        }

        // Rainbow lines through the point, across at the Drive and down at the Comp / Sat, in short sections each
        // blended between the curve's colours at its ends, along the line from left to right or bottom to top. They
        // are most opaque at the point and fade out toward the pad's edges, the sections meeting at the point.
        const auto line = [&](const bool vertical, const float alpha, const float width) {
            const float start = vertical ? py : px;
            const float length = vertical ? ph : pw;
            const float at = vertical ? pointY : pointX;

            // 1 at the point, eased to 0 at either edge
            const auto fadeAt = [&](const float pos) {
                const float e = pos < at ? (pos - start) / std::max(1e-3f, at - start)
                                         : (start + length - pos) / std::max(1e-3f, start + length - at);
                const float c = std::clamp(e, 0.f, 1.f);
                return c * c * (3.f - 2.f * c);
            };
            // the curve's position, 0 at the left or the bottom
            const auto colorAt = [&](const float pos) {
                const float t = (pos - start) / length;
                return vertical ? 1.f - t : t;
            };

            strokeWidth(width);

            const uint half = R::lineSections / 2;
            for (const auto& [from, to] : { std::make_pair(start, at), std::make_pair(at, start + length) })
            {
                for (uint i = 0; i < half; ++i)
                {
                    const float p0 = from + (to - from) * i / half;
                    const float p1 = from + (to - from) * (i + 1) / half;

                    if (p1 - p0 < 0.01f)
                        continue;

                    const Color c0 = curveColor(colorAt(p0), alpha * fadeAt(p0));
                    const Color c1 = curveColor(colorAt(p1), alpha * fadeAt(p1));

                    save();
                    beginPath();
                    if (vertical)
                    {
                        intersectScissor(px, p0, pw, p1 - p0);
                        moveTo(pointX, std::max(py, p0 - width - 1.f));
                        lineTo(pointX, std::min(py + ph, p1 + width + 1.f));
                        strokePaint(linearGradient(0, p0, 0, p1, c0, c1));
                    }
                    else
                    {
                        intersectScissor(p0, py, p1 - p0, ph);
                        moveTo(std::max(px, p0 - width - 1.f), pointY);
                        lineTo(std::min(px + pw, p1 + width + 1.f), pointY);
                        strokePaint(linearGradient(p0, 0, p1, 0, c0, c1));
                    }
                    stroke();
                    restore();
                }
            }
        };

        // the lines with a glow
        for (const bool vertical : { false, true })
        {
            line(vertical, 0.2f, 4.f * s);
            line(vertical, 1.f, 1.5f * s);
        }

        // the point, in the line's colour there with a white core
        {
            const float scale = fDragging ? R::pointDragScale : 1.f;
            const Color color = curveColor(normX(fValueX), 1.f);

            beginPath();
            circle(pointX, pointY, R::pointGlowRadius * scale * s);
            fillPaint(radialGradient(pointX, pointY, R::pointRadius * scale * s, R::pointGlowRadius * scale * s,
                                     Color(color, 0.5f), Color(color, 0.f)));
            fill();

            beginPath();
            circle(pointX, pointY, R::pointRadius * scale * s);
            fillColor(color);
            fill();

            beginPath();
            circle(pointX, pointY, R::pointRadius * 0.5f * scale * s);
            fillColor(R::pointColor);
            fill();
        }

        restore();

        drawGlass(w, h, r);
    }

    // A pane of glass over the plot: a gloss over the top whose lower edge sweeps up to the right, a faint diagonal
    // sheen across the middle, kept clear of the corners, a rim lit at the top and fading down, and a bright line just
    // inside the top edge.
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

// --------------------------------------------------------------------------------------------------------------------
// The display, above the controls: the XY pad floating over the starfield, with its shadow.

class GlueBusDisplayWidget final : public LabWidget
{
    // between the pad and the display's sides and top, as the controls below are in from the stage's edges
    static constexpr const uint kPanelMargin = GlueBusPageReference::margin;
    // between the pad and the controls below
    static constexpr const uint kPanelGap = GlueBusPageReference::padding;

    const std::unique_ptr<GlueBusXYPadWidget> fXYPad { new GlueBusXYPadWidget(this) };

public:
    explicit GlueBusDisplayWidget(LabWidget* const parent)
        : LabWidget(parent) {}

private:
    void onNanoDisplay() final
    {
        const float s = fScaleFactor;

        // the pad's shadow, as the group panels have theirs, left out where the panel is
        const auto shadow = [&](const LabWidget& panel, const float r) {
            const float x = panel.getAbsoluteX() - getAbsoluteX();
            const float y = panel.getAbsoluteY() - getAbsoluteY();
            const float pw = panel.getWidth();
            const float ph = panel.getHeight();

            beginPath();
            rect(x - 20.f * s, y, pw + 40.f * s, ph + 24.f * s);
            roundedRect(x, y, pw, ph, r);
            pathWinding(CW);
            fillPaint(boxGradient(x + 6.f * s, y + 10.f * s, pw - 12.f * s, ph - 12.f * s, r, 14.f * s,
                                  Color(0.f, 0.f, 0.f, 0.6f), Color(0.f, 0.f, 0.f, 0.f)));
            fill();
        };

        shadow(*fXYPad, fXYPad->getBorderRadius());
    }

    // the pad follows the display
    void layout()
    {
        const int margin = d_roundToIntPositive(kPanelMargin * fScaleFactor);
        const int gap = d_roundToIntPositive(kPanelGap * fScaleFactor);

        fXYPad->setSize(std::max(1, static_cast<int>(getWidth()) - margin * 2),
                        std::max(1, static_cast<int>(getHeight()) - margin - gap));
        fXYPad->setAbsolutePos(getAbsoluteX() + margin, getAbsoluteY() + margin);
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
// The stage: the starfield, the display above the controls and the control rows at the bottom. Easy and Expert pages
// show the same controls for now.

class GlueBusStageWidget final : public StageWidget<GlueBusPageWidget, GlueBusPageWidget>
{
    using BaseWidget = StageWidget<GlueBusPageWidget, GlueBusPageWidget>;

    const std::shared_ptr<GlueBusDisplayWidget> fDisplay { addGraphWidget<GlueBusDisplayWidget>() };

public:
    explicit GlueBusStageWidget(LabWidget* const parent)
        : BaseWidget(parent) {}

    Size<uint> getGraphSize() const noexcept final
    {
        const LabMetrics<R> m { this };
        const LabMetrics<GlueBusPageReference> pm { this };

        const GlueBusPageWidget* const page = fLastPage == kPageEasy ? getEasyWidget() : getExpertWidget();
        const uint space = m.border + pm.margin + page->getControlsHeight();

        return Size<uint>(getWidth() - m.border * 2, std::max(1u, getHeight() - std::min(getHeight(), space)));
    }
};

using GlueBusMainArea = MainAreaContainerWidget<GlueBusStageWidget>;
using GlueBusRootWidget = RootWidget<TopBar, GlueBusMainArea>;

// --------------------------------------------------------------------------------------------------------------------

class GlueBusUI : public LibreAudioBaseUI
{
public:
    GlueBusUI()
    {
        const std::list<std::shared_ptr<ShaderWidgetInterface>> shaders {
            std::make_shared<ShaderBackgroundWidget>(this, this),
            std::make_shared<BotShaderWidget<SHADER_CLOUDSTARFIELD>>(this, this),
        };

        createRootWidget<GlueBusRootWidget>();

        static_cast<GlueBusRootWidget*>(fRootWidget.get())->enableShaders(shaders);
    }
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

UI* createUI()
{
    return new LibreAudio::GlueBusUI();
}

END_NAMESPACE_DISTRHO
