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
#include <cstdio>

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
    widgets::Delta, widgets::Listen
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

// An empty column, holding the middle one's place.
using DeEsserPlaceholderWidget = LabReferenceWidget<DeEsserColumnReference>;

// --------------------------------------------------------------------------------------------------------------------
// The display, above the controls: three columns over the starfield, the absolute threshold at the left, the relative
// threshold at the right and the placeholder taking the width between them.

class DeEsserDisplayWidget final : public LabWidget
{
    // between the columns and the display's sides and top, as the controls below are in from the stage's edges
    static constexpr const uint kPanelMargin = DeEsserPageReference::margin;
    // between the columns side by side, and between them and the controls below
    static constexpr const uint kPanelGap = DeEsserPageReference::padding;

    using AbsoluteColumn = DeEsserThresholdColumnWidget<DeEsserAbsoluteMeter>;
    using RelativeColumn = DeEsserThresholdColumnWidget<DeEsserRelativeMeter>;

    const std::unique_ptr<AbsoluteColumn> fAbsolute { new AbsoluteColumn(this) };
    const std::unique_ptr<DeEsserPlaceholderWidget> fPlaceholder { new DeEsserPlaceholderWidget(this) };
    const std::unique_ptr<RelativeColumn> fRelative { new RelativeColumn(this) };

public:
    explicit DeEsserDisplayWidget(LabWidget* const parent)
        : LabWidget(parent) {}

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

        fPlaceholder->setSize(std::max(1, w - (margin + columnWidth + gap) * 2), h);
        fPlaceholder->setAbsolutePos(x + margin + columnWidth + gap, y);
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

} /* namespace LibreAudio */

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

UI* createUI()
{
    return new LibreAudio::UI<LibreAudio::DeEsserRootWidget>();
}

END_NAMESPACE_DISTRHO
