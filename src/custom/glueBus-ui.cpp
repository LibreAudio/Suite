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

// Low and high transient shelves, side by side.
using GlueBusTransientGroupsWidget = StaticContainerPairWidget<
    GlueBusGroupsContainerReference,
    Horizontal,
    SizeHintWidgetPair<Expanding, GlueBusGroupWidget<widgets::LowFreq, widgets::LowAttack, widgets::LowSustain>>,
    SizeHintWidgetPair<Expanding, GlueBusGroupWidget<widgets::HighFreq, widgets::HighAttack, widgets::HighSustain>>
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
// with a grid labelled on its lines, a level line at the Drive in the curve's colours with the area under it filled,
// dotted lines from the point down and to the left, and the point in the curve's colour with a white core.
// Clicking or dragging sets both; double-clicking resets both.

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
    // the ends of the Comp / Sat axis, at the bottom corners
    static constexpr const char* const labelLeft = "COMP";
    static constexpr const char* const labelRight = "SAT";
    static constexpr const Color crossColor { 1.f, 1.f, 1.f, 0.35f };
    // the cross lines are dotted: a dot this long, every this far, unscaled
    static constexpr const float crossDot = 1.f;
    static constexpr const float crossDotStep = 3.f;
    // mbComp5's curve colours, from left to right, blended between
    static constexpr const std::array<Color, 5> curveColors {{
        { 255, 191, 203 }, { 255, 223, 173 }, { 210, 253, 211 }, { 190, 241, 255 }, { 218, 193, 243 }
    }};
    // the area under the level line is filled in columns, each in the line's colour above it
    static constexpr const uint fillColumns = 32;
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

    float fValueX = kX.init;
    float fValueY = kY.init;
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

        // Grid across and down, its value on each line: Comp / Sat centred on its line along the bottom, Drive
        // centred on its line down the left. Each line stops short of its label and fades out at both ends, toward
        // its label and toward the panel's edge.
        fontFace("regular");
        fontSize(R::GridLabel::fontSize * s);
        strokeWidth(1.f);

        const float inset = R::GridLabel::inset * s;
        const float gap = R::GridLabel::gap * s;
        const float fade = R::GridLabel::fade * s;
        const Color clear { R::gridColor, 0.f };
        const float labelY = py + ph - inset - R::GridLabel::fontSize * 0.5f * s;

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

            std::snprintf(valuestr, sizeof(valuestr), "%g", std::abs(v) < 0.001f ? 0.f : v);

            // down, to the label at the bottom
            gridLine(false, gx, py, labelY - R::GridLabel::fontSize * 0.5f * s - gap);

            fillColor(R::GridLabel::color);
            textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
            text(gx, labelY, valuestr, nullptr);
        }

        // the axis's ends, in the bottom corners
        fillColor(R::GridLabel::color);
        textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
        text(px + inset + r * 0.5f, labelY, R::labelLeft, nullptr);
        textAlign(ALIGN_RIGHT | ALIGN_MIDDLE);
        text(px + pw - inset - r * 0.5f, labelY, R::labelRight, nullptr);

        // The top and bottom of the panel's rounded outline at an x.
        const auto outline = [&](const float x) {
            const float dx = std::min(r, std::max({ 0.f, px + r - x, x - (px + pw - r) }));
            const float dy = r - std::sqrt(r * r - dx * dx);
            return std::make_pair(py + dy, py + ph - dy);
        };

        // A stroke cut exactly to the outline: in one piece between the corners, and across them a column a pixel wide
        // at a time, scissored to the outline's height there. style sets the stroke's colour or paint, path builds the
        // path for the piece from x0 to x1.
        const auto clippedStroke = [&](const auto& style, const float width, const auto& path) {
            style();
            strokeWidth(width);

            const float corner = std::ceil(r);

            save();
            intersectScissor(px + corner, py, pw - corner * 2, ph);
            path(px + corner, px + pw - corner);
            stroke();
            restore();

            for (float i = 0.f; i < corner; i += 1.f)
            {
                for (const float cx : { px + i, px + pw - 1.f - i })
                {
                    const auto [top, bottom] = outline(cx + 0.5f);

                    save();
                    intersectScissor(cx, top, 1.f, bottom - top);
                    path(cx, cx + 1.f);
                    stroke();
                    restore();
                }
            }
        };

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

        // The level line from x0 to x1 at the Drive, held to the outline when it is to be filled, which for the area
        // under it is the same as cutting it there.
        const auto level = [&](const float x0, const float x1, const bool held) {
            beginPath();
            const float first = std::max(0.f, std::floor(x0 - px));
            const float last = std::min(pw, std::ceil(x1 - px));
            for (float x = first; x <= last; x += 1.f)
            {
                float y = pointY;

                if (held)
                {
                    const auto [top, bottom] = outline(px + x);
                    y = std::clamp(y, top, bottom);
                }

                if (x == first)
                    moveTo(px + x, y);
                else
                    lineTo(px + x, y);
            }
        };

        // The level line, cut exactly to the outline, in a section between each two of the curve's colours, blended
        // across it.
        const auto line = [&](const float alpha, const float width) {
            lineJoin(ROUND);

            const size_t sections = R::curveColors.size() - 1;
            for (size_t i = 0; i < sections; ++i)
            {
                const float left = px + pw * i / sections;
                const float right = px + pw * (i + 1) / sections;

                save();
                intersectScissor(left, py, right - left, ph);
                clippedStroke(
                    [&] {
                        strokePaint(linearGradient(left, 0, right, 0,
                                                   Color(R::curveColors[i], alpha),
                                                   Color(R::curveColors[i + 1], alpha)));
                    },
                    width,
                    [&](const float x0, const float x1) {
                        level(std::max(x0, left) - width - 1.f, std::min(x1, right) + width + 1.f, false);
                    });
                restore();
            }
        };

        // Filled down to the bottom, round its corners as the panel's, in columns each fading down from the line's
        // colour above it, then the line with a glow.
        for (uint i = 0; i < R::fillColumns; ++i)
        {
            const float left = std::round(px + pw * i / R::fillColumns);
            const float right = std::round(px + pw * (i + 1) / R::fillColumns);
            const float t = (i + 0.5f) / R::fillColumns;

            save();
            intersectScissor(left, py, right - left, ph);
            level(px, px + pw, true);
            lineTo(px + pw, py + ph - r);
            arcTo(px + pw, py + ph, px + pw - r, py + ph, r);
            lineTo(px + r, py + ph);
            arcTo(px, py + ph, px, py + ph - r, r);
            closePath();
            fillPaint(linearGradient(0, pointY, 0, py + ph, curveColor(t, 0.12f), curveColor(t, 0.02f)));
            fill();
            restore();
        }

        line(0.2f, 4.f * s);
        line(1.f, 1.5f * s);

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
// The meters floating over the starfield, as the compressor's: a title on top, a dB scale down both sides, a well with
// a wide track for each channel, and the channels' names under the tracks, L and R, or M and S with Mid / Side on.
// The tracks fill from 0 dB at the bottom of the well up to the meter's maximum at the top.

struct GlueBusMeterReference : GlueBusFloatingReference {
    static constexpr const uint width = 116;
    // between the panel's edge and its contents
    static constexpr const uint inset = 10;
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
        static constexpr const uint width = 14;
        static constexpr const uint gap = 4;
    };
    struct Well {
        static constexpr const Color backgroundColor = Reference::Colors::track;
        static constexpr const Color insetShadingColor { 0.f, 0.f, 0.f, 0.55f };
        static constexpr const uint border = 1;
        static constexpr const uint borderRadius = 5;
    };
    struct Track {
        static constexpr const uint width = 26;
        static constexpr const uint gap = 3;
    };
    struct Tick {
        static constexpr const Color color { Reference::Colors::ink3, 0.15f };
    };
};

// The left meter shows compression and the right one saturation, each in the colour of the XY pad's end it is on.
template<FaustParameterIndex kMeter1, FaustParameterIndex kMeter2, bool kRight>
class GlueBusMeterWidget final : public LabWidget,
                                 private IdleCallback
{
    using R = GlueBusMeterReference;

    static constexpr const std::array<FaustParameterIndex, 2> kMeters { kMeter1, kMeter2 };
    static constexpr const float kMin = kFaustParameters[kMeter1].min;
    static constexpr const float kMax = kFaustParameters[kMeter1].max;
    static constexpr const float kScaleStep = 1.f;

    std::array<float, 2> fValues { kMin, kMin };
    bool fMidSide = false;

public:
    explicit GlueBusMeterWidget(LabWidget* const parent)
        : LabWidget(parent)
    {
        addIdleCallback(this);
    }

    [[nodiscard]] float getBorderRadius() const noexcept
    {
        return R::borderRadius * fScaleFactor;
    }

private:
    void idleCallback() final
    {
        for (size_t i = 0; i < kMeters.size(); ++i)
        {
            if (const float value = std::clamp(fInterface->getParameterValue(kParametersMainStart + kMeters[i]),
                                               kMin, kMax); d_isNotEqual(fValues[i], value))
            {
                fValues[i] = value;
                repaint();
            }
        }

        if (const bool midSide = d_isNotZero(fInterface->getParameterValue(kParametersMainStart
                                                                           + kFaustParameterMid_side));
            fMidSide != midSide)
        {
            fMidSide = midSide;
            repaint();
        }
    }

    void onNanoDisplay() final
    {
        const float s = fScaleFactor;
        const float w = getWidth();
        const float h = getHeight();

        const char* const title = kRight ? "SATURATION" : "COMPRESSION";
        const Color& color = kRight ? GlueBusXYPadReference::curveColors.back()
                                    : GlueBusXYPadReference::curveColors.front();

        drawFloatingPanel(*this, w, h, s, GlueBusFloatingReference::backgroundColor);

        // the well with a scale either side, centred across the panel together
        const float ww = (R::Well::border * 2 + R::Track::width * 2 + R::Track::gap) * s;
        const float sw = (R::Scale::width + R::Scale::gap) * s;
        const float x = std::round((w - ww - sw * 2) * 0.5f);
        const float wx = x + sw;
        const float wy = (R::inset + R::Title::height + R::Title::gap) * s;
        const float wh = h - wy - (R::inset + R::Channel::height + R::Channel::gap) * s;

        if (wh <= 0.f || ww <= 0.f)
            return;

        const float b = R::Well::border * s;
        const float wr = R::Well::borderRadius * s;
        const float tw = R::Track::width * s;
        const std::array<float, 2> trackX { wx + b, wx + ww - b - tw };
        const float ty = wy + b;
        const float tb = wy + wh - b;

        // the track area, inside the well's border, kMin at the bottom
        const auto dbToY = [&](const float db) {
            return tb - (tb - ty) * std::clamp((db - kMin) / (kMax - kMin), 0.f, 1.f);
        };

        // the title, shrunk to fit the panel if need be
        {
            const float maxWidth = w - R::inset * 2 * s;
            Rectangle<float> bounds;

            fontFace("regular");
            fontSize(R::Title::fontSize * s);
            textLetterSpacing(R::Title::letterSpacing * s);

            if (const float titleWidth = textBounds(0, 0, title, nullptr, bounds); titleWidth > maxWidth)
                fontSize(R::Title::fontSize * s * maxWidth / titleWidth);

            textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
            fillColor(R::Title::color);
            text(w * 0.5f, (R::inset + R::Title::height * 0.5f) * s, title, nullptr);
            textLetterSpacing(0.f);
        }

        // the well
        beginPath();
        roundedRect(wx, wy, ww, wh, wr);
        fillColor(R::Well::backgroundColor);
        fill();

        // the tracks, each cut out of the well's shape so the ends follow its corners
        for (size_t i = 0; i < fValues.size(); ++i)
        {
            const float y = dbToY(fValues[i]);

            if (tb - y < 0.5f)
                continue;

            save();
            scissor(trackX[i], y, tw, tb - y);
            beginPath();
            roundedRect(wx + b, ty, ww - b * 2, tb - ty, wr - b);
            fillPaint(linearGradient(0, y, 0, tb, color, Color(color, 0.45f)));
            fill();
            restore();
        }

        // inset shading
        beginPath();
        roundedRect(wx, wy, ww, wh, wr);
        fillPaint(linearGradient(0, wy, 0, wy + 4 * s, R::Well::insetShadingColor, Reference::Colors::transparent));
        fill();

        // the scale: ticks across the well, leaving out its ends, and the dB either side
        fontSize(R::Scale::fontSize * s);
        fillColor(R::Scale::color);

        for (float db = kMin; db <= kMax + 0.001f; db += kScaleStep)
        {
            const float y = dbToY(db);

            if (d_isNotEqual(db, kMin) && d_isNotEqual(db, kMax))
            {
                beginPath();
                moveTo(wx + b, std::round(y) + 0.5f);
                lineTo(wx + ww - b, std::round(y) + 0.5f);
                strokeColor(R::Tick::color);
                strokeWidth(1.f);
                stroke();
            }

            char dbstr[8];
            std::snprintf(dbstr, sizeof(dbstr), "%.0f", db);
            textAlign(ALIGN_RIGHT | ALIGN_MIDDLE);
            text(x + R::Scale::width * s, y, dbstr, nullptr);
            textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
            text(wx + ww + R::Scale::gap * s, y, dbstr, nullptr);
        }

        // the channels' names, under their tracks
        {
            const char* const names[2][2] = { { "L", "R" }, { "M", "S" } };

            fontSize(R::Channel::fontSize * s);
            textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
            fillColor(R::Channel::color);

            const float y = wy + wh + (R::Channel::gap + R::Channel::height * 0.5f) * s;
            for (size_t i = 0; i < trackX.size(); ++i)
                text(trackX[i] + tw * 0.5f, y, names[fMidSide ? 1 : 0][i], nullptr);
        }
    }
};

using GlueBusCompressionMeterWidget = GlueBusMeterWidget<kFaustParameterReduction_1, kFaustParameterReduction_2, false>;
using GlueBusSaturationMeterWidget = GlueBusMeterWidget<kFaustParameterSaturation_1, kFaustParameterSaturation_2, true>;

// --------------------------------------------------------------------------------------------------------------------
// The display, above the controls: the compression meter at the left, the saturation meter at the right and the XY
// pad between them, floating over the starfield, with their shadows.

class GlueBusDisplayWidget final : public LabWidget
{
    // between the panels and the display's sides and top, as the controls below are in from the stage's edges
    static constexpr const uint kPanelMargin = GlueBusPageReference::margin;
    // between the panels and the controls below, and between the panels side by side
    static constexpr const uint kPanelGap = GlueBusPageReference::padding;

    const std::unique_ptr<GlueBusCompressionMeterWidget> fCompression { new GlueBusCompressionMeterWidget(this) };
    const std::unique_ptr<GlueBusXYPadWidget> fXYPad { new GlueBusXYPadWidget(this) };
    const std::unique_ptr<GlueBusSaturationMeterWidget> fSaturation { new GlueBusSaturationMeterWidget(this) };

public:
    explicit GlueBusDisplayWidget(LabWidget* const parent)
        : LabWidget(parent) {}

private:
    void onNanoDisplay() final
    {
        const float s = fScaleFactor;

        // the floating panels' shadows, as the group panels have theirs, left out where the panel is
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

        shadow(*fCompression, fCompression->getBorderRadius());
        shadow(*fXYPad, fXYPad->getBorderRadius());
        shadow(*fSaturation, fSaturation->getBorderRadius());
    }

    // the panels follow the display
    void layout()
    {
        const int margin = d_roundToIntPositive(kPanelMargin * fScaleFactor);
        const int gap = d_roundToIntPositive(kPanelGap * fScaleFactor);
        const int meterWidth = d_roundToIntPositive(GlueBusMeterReference::width * fScaleFactor);
        const int x = getAbsoluteX();
        const int y = getAbsoluteY() + margin;
        const int w = static_cast<int>(getWidth());
        const int h = std::max(1, static_cast<int>(getHeight()) - margin - gap);

        fCompression->setSize(meterWidth, h);
        fCompression->setAbsolutePos(x + margin, y);

        fSaturation->setSize(meterWidth, h);
        fSaturation->setAbsolutePos(x + w - margin - meterWidth, y);

        // the pad takes what is left between the meters
        const int padX = x + margin + meterWidth + gap;
        fXYPad->setSize(std::max(1, w - (margin + meterWidth + gap) * 2), h);
        fXYPad->setAbsolutePos(padX, y);
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
