// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "LibreAudioBaseUI.hpp"

#include "ui/containers/main-area.hpp"
#include "ui/containers/top-bar.hpp"
#include "ui/widgets/button.hpp"
#include "ui/widgets/button-group.hpp"
#include "ui/widgets/number-box.hpp"
#include "ui/widgets/power-box.hpp"
#include "ui/widgets/shader.hpp"
#include "ui/_lab/knob.hpp"

#include "LibreAudioParameters.hpp"

#include "las-resources.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <vector>

// --------------------------------------------------------------------------------------------------------------------

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------
// The stage holds the display, with the starfield behind it, and under it a row of control groups.
// Each group is a raised glass panel styled like the eq's option bar.

// height of the boxes in the groups: taller than a number box, for room around their contents
static constexpr const uint kBoxHeight = 45;

// between two groups side by side in a row
static constexpr const uint kGroupGap = 8;

// the controls of a group, under its header; the labelled group around them draws the panel
struct CompressorGroupReference : Reference::Zero {
    static constexpr const uint borderRadius = 10;
    static constexpr const uint margin = 8;
    static constexpr const uint padding = 6;
    static constexpr const uint height = kBoxHeight + margin * 2;
};

// accent colour of a parameter, as set by [accentcolor] in compressor.dsp

template<FaustParameterIndex id>
constexpr const Color& accentColor() noexcept
{
    constexpr uint accent = kFaustParameters[id].accentcolor;
    static_assert(accent >= 1 && accent <= 6, "accentcolor must be 1..6");

    if constexpr (accent == 1) return Reference::Colors::acc1;
    else if constexpr (accent == 2) return Reference::Colors::acc2;
    else if constexpr (accent == 3) return Reference::Colors::acc3;
    else if constexpr (accent == 4) return Reference::Colors::acc4;
    else if constexpr (accent == 5) return Reference::Colors::acc5;
    else return Reference::Colors::acc6;
}

template<FaustParameterIndex id>
using CompressorNumberBoxWidget = NumberBoxWidget<accentColor<id>(), kBoxHeight>;

template<FaustParameterIndex id>
using CompressorPowerBoxWidget = PowerBoxWidget<accentColor<id>(), kBoxHeight>;

// --------------------------------------------------------------------------------------------------------------------
// One group's controls: number boxes, which share whatever width the row gives the group, and controls of their own
// width.

class CompressorGroupWidget : public ReferenceContainerWidget<CompressorGroupReference>
{
    using R = CompressorGroupReference;
    using BaseWidget = ReferenceContainerWidget<R>;

    const uint fBoxCount;

public:
    CompressorGroupWidget(LabWidget* const parent, const uint boxCount)
        : BaseWidget(parent),
          fBoxCount(boxCount) {}

    [[nodiscard]] uint getBoxCount() const noexcept
    {
        return fBoxCount;
    }

    // width without the number boxes: panel margins, the space between the shown controls and any control of its own
    // width
    [[nodiscard]] uint getFixedWidth() const noexcept
    {
        const uint margin = d_roundToUnsignedInt((R::border + R::margin) * fScaleFactor);
        const uint padding = d_roundToUnsignedInt(R::padding * fScaleFactor);

        uint width = margin * 2;
        uint shown = 0;
        for (const SubWidgetWithSizeHint& s : Layout::widgets)
        {
            if (! s.widget->isVisible())
                continue;
            if (s.sizeHint == Fixed)
                width += s.widget->getWidth();
            ++shown;
        }
        return width + padding * (std::max(1u, shown) - 1);
    }

    void setGroupWidth(const uint width)
    {
        LabWidget::setWidth(width);
    }
};

// --------------------------------------------------------------------------------------------------------------------
// A gap in a group's controls standing in for the one between two groups: the gap between the groups and their two
// side margins, less the two spaces between controls it takes the place of. A group of eight with one in the middle
// then lines its number boxes up with two groups of four under it.

class CompressorGroupSpacerWidget final : public LabWidget
{
    using G = CompressorGroupReference;

    static constexpr const uint kWidth = kGroupGap + (G::border + G::margin) * 2 - G::padding * 2;

public:
    explicit CompressorGroupSpacerWidget(LabWidget* const parent)
        : LabWidget(parent)
    {
        updateSize(false);
    }

private:
    void onNanoDisplay() final
    {
    }

    void updateSize(const bool updateChildren) final
    {
        setSize(d_roundToUnsignedInt(kWidth * fScaleFactor), std::max(1u, getHeight()));
        LabWidget::updateSize(updateChildren);
    }
};

// --------------------------------------------------------------------------------------------------------------------
// How the gain moves: lookahead, attack, hold and release, with the release's curve, the release half spaced off from
// the attack half.

class CompressorResponseGroupWidget final : public CompressorGroupWidget
{
    const std::array<std::shared_ptr<LabWidget>, 7> widgets {
        addWidget<CompressorNumberBoxWidget<kFaustParameterLookahead>, Expanding>(kFaustParameterLookahead),
        addWidget<CompressorNumberBoxWidget<kFaustParameterAttack>, Expanding>(kFaustParameterAttack),
        addWidget<CompressorNumberBoxWidget<kFaustParameterHold>, Expanding>(kFaustParameterHold),
        addWidget<CompressorGroupSpacerWidget>(),
        addWidget<CompressorNumberBoxWidget<kFaustParameterRelease>, Expanding>(kFaustParameterRelease),
        addWidget<CompressorNumberBoxWidget<kFaustParameterRelease_curve>, Expanding>(kFaustParameterRelease_curve),
        addWidget<CompressorNumberBoxWidget<kFaustParameterAuto_release>, Expanding>(kFaustParameterAuto_release),
    };

public:
    explicit CompressorResponseGroupWidget(LabWidget* const parent)
        : CompressorGroupWidget(parent, 6) {}
};

// What the detector measures and how the channels are linked.

class CompressorDetectorGroupWidget final : public CompressorGroupWidget
{
    const std::array<std::shared_ptr<LabWidget>, 4> widgets {
        addWidget<CompressorNumberBoxWidget<kFaustParameterPeak_rms>, Expanding>(kFaustParameterPeak_rms),
        addWidget<CompressorNumberBoxWidget<kFaustParameterRms_time>, Expanding>(kFaustParameterRms_time),
        addWidget<CompressorNumberBoxWidget<kFaustParameterFeedback>, Expanding>(kFaustParameterFeedback),
        addWidget<CompressorNumberBoxWidget<kFaustParameterLink>, Expanding>(kFaustParameterLink),
    };

public:
    explicit CompressorDetectorGroupWidget(LabWidget* const parent)
        : CompressorGroupWidget(parent, 4) {}
};

// The sidechain source and the filters shaping what the detector hears: External and SC Tilt, with the two boxes of
// the filter SC Filter picks between them.

class CompressorSidechainGroupWidget final : public CompressorGroupWidget,
                                             private IdleCallback
{
    // a filter's two boxes follow External, in the order of SC Filter's values
    static constexpr const uint kFilterStart = 1;

    const std::array<std::shared_ptr<LabWidget>, 8> widgets {
        addWidget<CompressorNumberBoxWidget<kFaustParameterSc_mix>, Expanding>(kFaustParameterSc_mix),
        addWidget<CompressorNumberBoxWidget<kFaustParameterSc_hp>, Expanding>(kFaustParameterSc_hp),
        addWidget<CompressorNumberBoxWidget<kFaustParameterSc_lp>, Expanding>(kFaustParameterSc_lp),
        addWidget<CompressorNumberBoxWidget<kFaustParameterSc_band_freq>, Expanding>(kFaustParameterSc_band_freq),
        addWidget<CompressorNumberBoxWidget<kFaustParameterSc_band_q>, Expanding>(kFaustParameterSc_band_q),
        addWidget<CompressorNumberBoxWidget<kFaustParameterSc_bell_freq>, Expanding>(kFaustParameterSc_bell_freq),
        addWidget<CompressorNumberBoxWidget<kFaustParameterSc_bell_gain>, Expanding>(kFaustParameterSc_bell_gain),
        addWidget<CompressorNumberBoxWidget<kFaustParameterSc_tilt>, Expanding>(kFaustParameterSc_tilt),
    };

    uint fFilter = UINT_MAX;

public:
    explicit CompressorSidechainGroupWidget(LabWidget* const parent)
        : CompressorGroupWidget(parent, 4)
    {
        addIdleCallback(this);
        showFilter(d_roundToUnsignedInt(kFaustParameters[kFaustParameterSc_filter].init));
    }

private:
    bool showFilter(const uint filter)
    {
        if (filter == fFilter)
            return false;

        fFilter = filter;

        for (uint i = kFilterStart; i < widgets.size() - 1; ++i)
            widgets[i]->setVisible((i - kFilterStart) / 2 == filter);

        return true;
    }

    void idleCallback() final
    {
        if (showFilter(d_roundToUnsignedInt(fInterface->getParameterValue(kParametersMainStart
                                                                          + kFaustParameterSc_filter))))
            updateSize(true);
    }
};

// --------------------------------------------------------------------------------------------------------------------
// The envelope display: the gain reduction a 20 dB step gets from the response controls, worked out with the rate law
// of the ballistics in compressor.dsp. The time axis runs in sections, attack, hold and release, each as wide as the
// square root of its duration, attack and release plus a share of the whole, so a short attack stays readable beside a
// long release. Each section is drawn in its parameter's accent colour. Behind it a made-up drum hit, faint in
// Lookahead's accent colour, shows where the transient reaches the output, lookahead later than the attack starts, and
// with Auto Release up a faint second release shows it stretched by a sustained 6 dB of reduction. It sits in a well
// like a number box's.

struct CompressorEnvelopeReference : Reference::Zero {
    static constexpr const uint height = 100;
    static constexpr const float labelFontSize = 9.f;
    // the share of the sum of the sections' square roots attack and release get on top of their own; hold gets none,
    // so it grows from nothing as Hold comes up from 0
    static constexpr const float minSection = 0.15f;
    // where 0 dB and the step's depth sit, as fractions of the height
    static constexpr const float top = 0.2f;
    static constexpr const float bottom = 0.84f;
};

class CompressorEnvelopeWidget final : public LabWidget,
                                       private IdleCallback
{
    using R = CompressorEnvelopeReference;
    using G = CompressorGroupReference;
    using Well = Reference::Widgets::NumberBox<>;

    // constants of the ballistics in compressor.dsp
    static constexpr const float kCurveRef = 20.f;
    static constexpr const float kCurveBound = 8.f;
    static constexpr const float kAttackCurve = 0.5f;
    static constexpr const float kAutoRefDb = 6.f;

    // depth of the step, in dB
    static constexpr const float kDepth = 20.f;
    // integration steps per ramp time, and the most a ramp may take
    static constexpr const float kStepsPerTime = 1000.f;
    static constexpr const uint kMaxSteps = 20000;

    static constexpr const std::array<FaustParameterIndex, 6> kParameters {
        kFaustParameterLookahead,
        kFaustParameterAttack,
        kFaustParameterHold,
        kFaustParameterRelease,
        kFaustParameterRelease_curve,
        kFaustParameterAuto_release,
    };

    struct Sample {
        float t;
        float db;
    };

    std::array<float, kParameters.size()> fValues {};

    // ramps in seconds from their own start, and the sections' durations
    std::vector<Sample> fAttack, fRelease, fAutoRelease;
    std::array<float, 3> fDurations {};
    float fLookahead = 0.f;

public:
    // unscaled
    // unscaled, with a gap above it as wide as the one the controls leave under the header
    static constexpr const uint kHeight = G::margin + R::height;

    explicit CompressorEnvelopeWidget(LabWidget* const parent)
        : LabWidget(parent)
    {
        for (size_t i = 0; i < kParameters.size(); ++i)
            fValues[i] = kFaustParameters[kParameters[i]].init;

        compute();
        updateSize(false);
        addIdleCallback(this);
    }

private:
    // dB per second of the power law at a distance from the target
    static float powerRate(const float dist, const float time, const float curve) noexcept
    {
        return kCurveRef / time * std::pow(std::clamp(dist / kCurveRef, 1.f / kCurveBound, kCurveBound), curve);
    }

    // a ramp from one dB to another, stepped until it lands, never stepping past the target
    template<class Rate>
    static std::vector<Sample> ramp(const float from, const float to, const float time, Rate rate)
    {
        const float dt = time / kStepsPerTime;
        std::vector<Sample> samples { { 0.f, from } };
        float db = from;

        for (uint i = 1; i <= kMaxSteps && d_isNotEqual(db, to); ++i)
        {
            const float dist = std::abs(to - db);
            const float step = rate(dist) * dt;
            db = step >= dist ? to : db + std::copysign(step, to - db);
            samples.push_back({ static_cast<float>(i) * dt, db });
        }

        return samples;
    }

    void compute()
    {
        const float attack = std::max(1e-5f, fValues[1] * 0.001f);
        const float release = std::max(1e-3f, fValues[3] * 0.001f);
        const float releaseCurve = fValues[4] / 100.f;
        const float autoRelease = fValues[5] / 100.f;

        fLookahead = fValues[0] * 0.001f;

        fAttack = ramp(0.f, -kDepth, attack, [=](const float dist) {
            return powerRate(dist, attack, kAttackCurve);
        });

        const auto releaseRamp = [=](const float time) {
            return ramp(-kDepth, 0.f, time, [=](const float dist) {
                return powerRate(dist, time, releaseCurve);
            });
        };

        // Each section is as long as the slowest curve, +1, takes to land, and its ramp is stretched to fill it, so the
        // curves show only as the ramps' shapes and leave their lengths alone.
        const auto section = [](const float time) {
            return ramp(0.f, -kDepth, time, [=](const float dist) {
                return powerRate(dist, time, 1.f);
            }).back().t;
        };

        const auto stretch = [](std::vector<Sample>& samples, const float length) {
            if (const float landing = samples.back().t; landing > 0.f)
                for (Sample& sample : samples)
                    sample.t *= length / landing;
        };

        const float attackSection = section(attack);
        stretch(fAttack, attackSection);

        const float releaseSection = section(release);
        fRelease = releaseRamp(release);
        stretch(fRelease, releaseSection);

        // with a sustained kAutoRefDb of reduction, Auto Release stretches the release by 1 + autoRelease
        if (autoRelease > 0.f)
        {
            fAutoRelease = releaseRamp(release * (1.f + autoRelease));
            stretch(fAutoRelease, releaseSection * (1.f + autoRelease));
        }
        else
        {
            fAutoRelease.clear();
        }

        fDurations = {
            attackSection,
            fValues[2] * 0.001f,
            releaseSection * (1.f + autoRelease),
        };
    }

    void idleCallback() final
    {
        bool changed = false;

        for (size_t i = 0; i < kParameters.size(); ++i)
        {
            if (const float value = fInterface->getParameterValue(kParametersMainStart + kParameters[i]);
                d_isNotEqual(fValues[i], value))
            {
                fValues[i] = value;
                changed = true;
            }
        }

        if (changed)
        {
            compute();
            repaint();
        }
    }

    void onNanoDisplay() final
    {
        const float s = fScaleFactor;
        const float x = (G::border + G::margin) * s;
        const float w = getWidth() - x * 2;
        const float gap = G::margin * s;
        const float h = getHeight() - gap;
        const float r = Well::borderRadius * s;

        // the display below the gap
        translate(0, gap);

        if (w <= 0.f)
            return;

        // the parameters' accent colours: lookahead for the drum hit, the others for their sections
        const Color& lookaheadColor = accentColor<kFaustParameterLookahead>();
        const std::array<Color, 3> colors {
            accentColor<kFaustParameterAttack>(),
            accentColor<kFaustParameterHold>(),
            accentColor<kFaustParameterRelease>(),
        };

        // the well, with a number box's inset shading
        beginPath();
        roundedRect(x, 0, w, h, r);
        fillColor(Well::backgroundColor);
        fill();
        fillPaint(linearGradient(0, 0, 0, Well::margin * s, Well::insetShadingColor, Reference::Colors::transparent));
        fill();
        strokeColor(Well::backgroundColor);
        strokeWidth(Well::border * s * 0.5f);
        stroke();

        // the sections across the width
        std::array<float, 3> widths {};
        {
            float roots = 0.f;
            for (const float d : fDurations)
                roots += std::sqrt(d);

            float sum = 0.f;
            for (size_t i = 0; i < widths.size(); ++i)
                sum += widths[i] = std::sqrt(fDurations[i]) + (i != 1 ? R::minSection * roots : 0.f);

            for (float& width : widths)
                width *= sum > 0.f ? w / sum : 0.f;
        }

        // x of a time, in seconds from the start of the attack
        const auto timeToX = [&](float t) {
            float tx = x;
            for (size_t i = 0; i < widths.size(); ++i)
            {
                if (t <= fDurations[i])
                    return tx + (fDurations[i] > 0.f ? t / fDurations[i] * widths[i] : 0.f);
                t -= fDurations[i];
                tx += widths[i];
            }
            return tx;
        };

        const float y0 = h * R::top;
        const float yDepth = h * R::bottom;
        const auto dbToY = [&](const float db) {
            return y0 - db / kDepth * (yDepth - y0);
        };

        save();
        intersectScissor(x, 0, w, h);

        // dB grid, every 10 dB
        for (float db = 0.f; db >= -kDepth; db -= 10.f)
        {
            beginPath();
            moveTo(x, std::round(dbToY(db)) + 0.5f);
            lineTo(x + w, std::round(dbToY(db)) + 0.5f);
            strokeColor(Color(1.f, 1.f, 1.f, 0.06f));
            strokeWidth(1.f);
            stroke();
        }

        // section edges, with the sections' names under the curve and their times, as the knobs set them and in their
        // accent colours, over it, where they fit
        {
            static constexpr const char* const names[] = { "ATTACK", "HOLD", "RELEASE" };
            const std::array<float, 3> times { fValues[1], fValues[2], fValues[3] };

            fontFace("regular");
            fontSize(R::labelFontSize * s);

            float sx = x;
            for (size_t i = 0; i < widths.size(); ++i)
            {
                // between sections only, the well's sides being the outer edges
                if (i != 0)
                {
                    beginPath();
                    moveTo(std::round(sx) + 0.5f, 0);
                    lineTo(std::round(sx) + 0.5f, h);
                    strokeColor(Color(1.f, 1.f, 1.f, 0.07f));
                    strokeWidth(1.f);
                    stroke();
                }

                const auto label = [&](const char* const str, const float y, const int align) {
                    Rectangle<float> bounds;
                    textAlign(ALIGN_CENTER | align);
                    if (widths[i] > 0.f && textBounds(0, 0, str, nullptr, bounds) + 6.f * s < widths[i])
                        text(sx + widths[i] * 0.5f, y, str, nullptr);
                };

                char timestr[16];
                if (times[i] < 1000.f)
                    std::snprintf(timestr, sizeof(timestr), "%.0f ms", times[i]);
                else
                    std::snprintf(timestr, sizeof(timestr), "%.3g s", times[i] / 1000.f);

                fillColor(Color(colors[i], 0.8f));
                label(timestr, 4.f * s, ALIGN_TOP);
                fillColor(Color(Reference::Colors::ink3, 0.8f));
                label(names[i], h - 4.f * s, ALIGN_BOTTOM);

                sx += widths[i];
            }
        }

        // a ramp's points, from a time on, skipping those less than half a pixel along
        const auto addRamp = [&](const std::vector<Sample>& samples, const float t0) {
            float lastX = -1.f;
            for (size_t i = 0; i < samples.size(); ++i)
            {
                const float px = timeToX(t0 + samples[i].t);
                if (px - lastX < 0.5f && i + 1 != samples.size())
                    continue;
                lineTo(px, dbToY(samples[i].db));
                lastX = px;
            }
        };

        const float releaseStart = fDurations[0] + fDurations[1];

        // The drum hit: a sine dropping in pitch with a little noise in it, under an envelope that jumps up where the
        // transient reaches the output and decays over a fixed share of the width. It is the input, so the response
        // controls only move it with Lookahead and leave its shape alone.
        {
            const float yMid = (y0 + yDepth) * 0.5f;
            const float amp = (yDepth - y0) * 0.5f;
            const float decay = w * 0.1f;
            const float xStart = timeToX(fLookahead);

            std::vector<std::pair<float, float>> points;
            float phase = 0.f;
            uint32_t noise = 0x12345678u;

            for (float px = xStart; px <= x + w; px += 1.f)
            {
                const float dx = px - xStart;
                const float env = std::exp(-dx / decay) * std::min(1.f, dx / (1.5f * s));
                if (env < 0.02f && dx > 0.f)
                    break;

                noise = noise * 1664525u + 1013904223u;
                const float n = static_cast<float>(noise >> 8) / static_cast<float>(1u << 24) * 2.f - 1.f;

                points.push_back({ px, amp * env * (0.8f * std::sin(phase) + 0.2f * n) });
                phase += 2.f * static_cast<float>(M_PI) / (s * (3.f + 6.f * (1.f - std::exp(-dx / (40.f * s)))));
            }

            if (points.size() > 1)
            {
                beginPath();
                moveTo(points.front().first, yMid - points.front().second);
                for (const std::pair<float, float>& p : points)
                    lineTo(p.first, yMid - p.second);
                strokeColor(Color(lookaheadColor, 0.35f));
                strokeWidth(1.f);
                stroke();
            }
        }

        // the stretched release, faint and behind
        if (! fAutoRelease.empty())
        {
            beginPath();
            moveTo(timeToX(releaseStart), dbToY(-kDepth));
            addRamp(fAutoRelease, releaseStart);
            strokeColor(Color(colors[2], 0.35f));
            strokeWidth(1.f * s);
            stroke();
        }

        // the envelope: flat at 0 dB, down the attack, held, back up the release
        const auto curve = [&]() {
            beginPath();
            moveTo(x, y0);
            lineTo(timeToX(0.f), y0);
            addRamp(fAttack, 0.f);
            lineTo(timeToX(releaseStart), dbToY(-kDepth));
            addRamp(fRelease, releaseStart);
            lineTo(x + w, y0);
        };

        // drawn once per section, cut to it, in its colour: the attack from the left edge, the release to the right
        const std::array<float, 4> edges { x, timeToX(fDurations[0]), timeToX(releaseStart), x + w };

        for (size_t i = 0; i < colors.size(); ++i)
        {
            if (edges[i + 1] <= edges[i])
                continue;

            save();
            intersectScissor(edges[i], 0, edges[i + 1] - edges[i], h);

            curve();
            closePath();
            fillPaint(linearGradient(0, y0, 0, yDepth, Color(colors[i], 0.04f), Color(colors[i], 0.3f)));
            fill();

            curve();
            strokeColor(Color(colors[i], 0.25f));
            strokeWidth(4.f * s);
            lineJoin(ROUND);
            stroke();
            strokeColor(colors[i]);
            strokeWidth(1.5f * s);
            stroke();

            restore();
        }

        restore();
    }

    // as wide as the group makes it
    void updateSize(const bool updateChildren) final
    {
        setSize(std::max(1u, getWidth()), d_roundToUnsignedInt(kHeight * fScaleFactor));
        LabWidget::updateSize(updateChildren);
    }
};

// --------------------------------------------------------------------------------------------------------------------
// The buttons in a group's header, taken from mbComp5's band headers: a small raised pill, its button lit in the
// accent colour while on.

template<const Color& kAccentColor>
struct CompressorHeaderButtonGroupReference : Reference::Zero {
    static constexpr const Color backgroundColor = { 0xff, 0xff, 0xff, 0.07f };
    static constexpr const Color borderColor = backgroundColor;
    static constexpr const Color borderHighlightColor { borderColor, 0.16f };
    static constexpr const Color borderShadowColor { 0, 0, 0, 0.16f };
    static constexpr const uint border = 1;
    static constexpr const uint borderRadius = 6;
    static constexpr const uint margin = 2;
    static constexpr const uint padding = 2;
    static constexpr const uint height = 18;

    static constexpr const Color shadowColor { 0, 0, 0, 0.6f };
    static constexpr const Point<int> shadowOffset = { 0, 2 };
    static constexpr const Size<int> shadowSize = { -8, -8 };
    static constexpr const uint shadowFeather = 12;

    struct Solo : Reference::Zero {
        static constexpr const Color backgroundColor〡checked = kAccentColor;
        static constexpr const Color color = Reference::Colors::ink2;
        static constexpr const Color color〡checked = Color(0x17, 0x17, 0x1b);
        static constexpr const Color color〡hover = kAccentColor;
        static constexpr const MouseCursor cursor〡hover = kMouseCursorHand;
        static constexpr const uint borderRadius = 4;
        static constexpr const uint width = height;
    };

    // as Solo, with a name in place of the icon
    struct Choice : Solo {
        static constexpr const float fontSize = 10.f;
        static constexpr const float letterSpacing = 0.4f;
        // either side of the name
        static constexpr const uint margin = 5;
        static constexpr const uint width = 0;
    };
};

// A listen button: the solo icon in the pill. kId is the parameter it switches.

template<FaustParameterIndex kId>
class CompressorListenButtonWidget final
    : public ReferenceButtonGroupWidget<CompressorHeaderButtonGroupReference<accentColor<kId>()>>
{
    using GR = CompressorHeaderButtonGroupReference<accentColor<kId>()>;
    using SR = typename GR::Solo;
    using BaseWidget = ReferenceButtonGroupWidget<GR>;

    class SoloButtonWidget final
        : public ImageButtonWidget<kCornerBoth, IMAGES_SOLO_28_PNG_DATA, IMAGES_SOLO_28_PNG_LEN, SR>
    {
        using ImageBaseWidget = ImageButtonWidget<kCornerBoth, IMAGES_SOLO_28_PNG_DATA, IMAGES_SOLO_28_PNG_LEN, SR>;

    public:
        explicit SoloButtonWidget(LabWidget* const parent)
            : ImageBaseWidget(parent) {}

    private:
        void onNanoDisplay() override
        {
            if (ImageBaseWidget::isChecked())
                ImageBaseWidget::template drawReferenceBackground<SR, SR::backgroundColor〡checked, kCornerBoth>();
            else
                ImageBaseWidget::template drawReferenceBackground<SR, SR::backgroundColor, kCornerBoth>();

            ImageBaseWidget::template drawReferenceBorder<SR, SR::borderColor>();

            if (ImageBaseWidget::isChecked())
                ImageBaseWidget::drawColoredImage(SR::color〡checked);
            else if (ImageBaseWidget::isHovered())
                ImageBaseWidget::drawColoredImage(SR::color〡hover);
            else
                ImageBaseWidget::drawColoredImage(SR::color);
        }
    };

public:
    explicit CompressorListenButtonWidget(LabWidget* const parent)
        : BaseWidget(parent)
    {
        BaseWidget::template addButton<SoloButtonWidget>(kParametersMainStart + kId,
                                                         getFaustParameterLabel(kFaustParameters[kId]));
        BaseWidget::done();
    }
};

// A choice between the values of a radio parameter: their names side by side in the pill, the one set lit. kId is the
// parameter it sets.

template<FaustParameterIndex kId>
class CompressorChoiceButtonWidget final
    : public ReferenceContainerWidget<CompressorHeaderButtonGroupReference<accentColor<kId>()>>,
      private ButtonEventHandler::Callback,
      private IdleCallback
{
    using GR = CompressorHeaderButtonGroupReference<accentColor<kId>()>;
    using CR = typename GR::Choice;
    using BaseWidget = ReferenceContainerWidget<GR>;

    static constexpr const FaustParameter& kParameter = kFaustParameters[kId];

    class ChoiceWidget final : public ReferenceButtonWidget<CR, kCornerBoth>
    {
        using ButtonWidget = ReferenceButtonWidget<CR, kCornerBoth>;

    public:
        ChoiceWidget(LabWidget* const parent, const uint value, const char* const label)
            : ButtonWidget(parent)
        {
            ButtonWidget::setCheckable(true);
            ButtonWidget::setId(value);
            ButtonWidget::setName(label);
        }

    private:
        void onNanoDisplay() final
        {
            ButtonWidget::fontFace("regular");

            if (ButtonWidget::isChecked())
            {
                ButtonWidget::template drawReferenceBackground<CR, CR::backgroundColor〡checked, kCornerBoth>();
                ButtonWidget::template drawReferenceBorder<CR, CR::borderColor>();
                ButtonWidget::template drawReferenceText<CR, CR::color〡checked>(ButtonWidget::getName());
            }
            else
            {
                ButtonWidget::template drawReferenceBackground<CR, CR::backgroundColor, kCornerBoth>();
                ButtonWidget::template drawReferenceBorder<CR, CR::borderColor>();

                if (ButtonWidget::isHovered())
                    ButtonWidget::template drawReferenceText<CR, CR::color〡hover>(ButtonWidget::getName());
                else
                    ButtonWidget::template drawReferenceText<CR, CR::color>(ButtonWidget::getName());
            }
        }

        // as wide as its name, and the margins either side
        void updateSize(const bool updateChildren) final
        {
            const float s = ButtonWidget::fScaleFactor;

            Rectangle<float> bounds;
            ButtonWidget::fontFace("regular");
            ButtonWidget::fontSize(CR::fontSize * s);
            ButtonWidget::textAlign(0);
            ButtonWidget::textLetterSpacing(CR::letterSpacing * s);
            ButtonWidget::textBounds(0, 0, ButtonWidget::getName(), nullptr, bounds);
            ButtonWidget::setWidth(d_roundToUnsignedInt(bounds.getWidth() + CR::margin * 2 * s));

            ButtonWidget::updateSize(updateChildren);
        }
    };

    std::list<std::unique_ptr<ChoiceWidget>> fChoices;

public:
    explicit CompressorChoiceButtonWidget(LabWidget* const parent)
        : BaseWidget(parent)
    {
        BaseWidget::addIdleCallback(this);
        BaseWidget::setId(kParametersMainStart + kId);

        for (uint i = 0; i < kParameter.scalePointCount; ++i)
        {
            std::unique_ptr<ChoiceWidget> widget {
                new ChoiceWidget(this, kParameter.scalePoints[i].value, kParameter.scalePoints[i].label)
            };
            widget->setCallback(this);
            BaseWidget::widgets.push_back({ widget.get(), Fixed });
            fChoices.emplace_back(std::move(widget));
        }

        idleCallback();
        updateSize(true);
    }

private:
    void buttonClicked(SubWidget* const widget, int) final
    {
        ChoiceWidget* const choice = static_cast<ChoiceWidget*>(widget);

        // the one set stays lit
        if (! choice->isChecked())
        {
            choice->setChecked(true, false);
            return;
        }

        for (const std::unique_ptr<ChoiceWidget>& other : fChoices)
            if (other.get() != choice)
                other->setChecked(false, false);

        const uint id = BaseWidget::getId();
        BaseWidget::fInterface->parameterControlPressed(id);
        BaseWidget::fInterface->parameterControlModified(id, choice->getId());
        BaseWidget::fInterface->parameterControlReleased(id);
    }

    void idleCallback() final
    {
        const uint value = d_roundToUnsignedInt(BaseWidget::fInterface->getParameterValue(BaseWidget::getId()));

        for (const std::unique_ptr<ChoiceWidget>& choice : fChoices)
            choice->setChecked(choice->getId() == value, false);
    }

    // as wide as its choices, the pill's padding between them and its margins
    void updateSize(const bool updateChildren) final
    {
        DISTRHO_SAFE_ASSERT(updateChildren);

        BaseWidget::updateSize(true);

        const uint border = d_roundToUnsignedInt(GR::border * BaseWidget::fScaleFactor);
        const uint margin = d_roundToUnsignedInt(GR::margin * BaseWidget::fScaleFactor);
        const uint padding = d_roundToUnsignedInt(GR::padding * BaseWidget::fScaleFactor);

        uint width = (border + margin) * 2 + padding * (std::max<uint>(1, fChoices.size()) - 1);
        for (const std::unique_ptr<ChoiceWidget>& choice : fChoices)
            width += choice->getWidth();

        LabWidget::setWidth(width);

        BaseWidget::updateSize(false);
    }
};

// --------------------------------------------------------------------------------------------------------------------
// A group's header, styled like the reverb's section headers: its name in the group's accent colour, at the panel's
// top left, in a 12 px row 8 px above the controls.

struct CompressorGroupHeaderReference : Reference::Zero {
    static constexpr const float fontSize = 12.f;
    static constexpr const float letterSpacing = 0.4f;
    // the label's row, as tall as its font, so it sits as far from the panel's top as from its left
    static constexpr const uint row = 12;
    static constexpr const uint gap = 8;
    // between two buttons
    static constexpr const uint buttonGap = 6;
    // the panel's top margin, then the row and the gap; the controls' own top margin follows
    static constexpr const uint height = CompressorGroupReference::margin + row + gap
                                       - CompressorGroupReference::margin;
};

class CompressorGroupHeaderWidget final : public LabWidget
{
    using R = CompressorGroupHeaderReference;
    using G = CompressorGroupReference;

    const char* const fLabel;
    const Color fColor;

    std::vector<std::unique_ptr<LabWidget>> fButtons;

public:
    CompressorGroupHeaderWidget(LabWidget* const parent, const char* const label, const Color color)
        : LabWidget(parent),
          fLabel(label),
          fColor(color)
    {
        updateSize(false);
    }

    // buttons at the right of the label's row, centred on it, the first as far in from the panel's right as the label
    // is from its left and each one after left of the one before
    template<class B, typename... Args>
    void addButton(Args... args)
    {
        fButtons.emplace_back(new B(this, args...));
        updateSize(false);
    }

private:
    void onNanoDisplay() final
    {
        fontFace("regular");
        fontSize(R::fontSize * fScaleFactor);
        textLetterSpacing(R::letterSpacing * fScaleFactor);
        textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
        fillColor(fColor);
        text((G::border + G::margin) * fScaleFactor, (G::margin + R::row * 0.5f) * fScaleFactor, fLabel, nullptr);
    }

    // as wide as the group makes it
    void updateSize(const bool updateChildren) final
    {
        setSize(std::max(1u, getWidth()), d_roundToUnsignedInt(R::height * fScaleFactor));
        LabWidget::updateSize(updateChildren);

        const int gap = d_roundToIntPositive(R::buttonGap * fScaleFactor);
        const int rowCentre = d_roundToIntPositive((G::margin + R::row * 0.5f) * fScaleFactor);
        int right = getAbsoluteX() + static_cast<int>(getWidth())
                  - d_roundToIntPositive((G::border + G::margin) * fScaleFactor);

        for (const std::unique_ptr<LabWidget>& button : fButtons)
        {
            right -= static_cast<int>(button->getWidth());
            button->setAbsolutePos(right, getAbsoluteY() + rowCentre - static_cast<int>(button->getHeight()) / 2);
            right -= gap;
        }
    }
};

// A group's panel: its header over its controls, with a display between them in some groups. The row sizes these, and
// they pass the sizes on to their controls.

struct CompressorLabeledGroupReference : Reference::Zero {
    // without a display
    static constexpr const uint height = CompressorGroupHeaderReference::height + CompressorGroupReference::height;
};

class CompressorLabeledGroupWidget : public ReferenceContainerWidget<Reference::Zero, kVertical>
{
    using BaseWidget = ReferenceContainerWidget<Reference::Zero, kVertical>;

    // addWidget's layout only points at its widgets, these own them
    const std::shared_ptr<CompressorGroupHeaderWidget> fHeader;

protected:
    std::shared_ptr<LabWidget> fDisplay;
    std::shared_ptr<CompressorGroupWidget> fPanel;

    // unscaled
    uint fHeight = CompressorLabeledGroupReference::height;

public:
    CompressorLabeledGroupWidget(LabWidget* const parent, const char* const label, const Color color)
        : BaseWidget(parent),
          fHeader(addWidget<CompressorGroupHeaderWidget>(label, color)) {}

    // unscaled
    [[nodiscard]] uint getGroupHeight() const noexcept
    {
        return fHeight;
    }

    template<class B, typename... Args>
    void addHeaderButton(Args... args)
    {
        fHeader->addButton<B>(args...);
    }

    [[nodiscard]] uint getBoxCount() const noexcept
    {
        return fPanel->getBoxCount();
    }

    [[nodiscard]] uint getFixedWidth() const noexcept
    {
        return fPanel->getFixedWidth();
    }

    void setGroupWidth(const uint width)
    {
        LabWidget::setWidth(width);
        fPanel->setGroupWidth(width);
    }

protected:
    void updateSize(const bool updateChildren) final
    {
        LabWidget::setHeight(d_roundToUnsignedInt(fHeight * fScaleFactor));
        BaseWidget::updateSize(updateChildren);
    }

private:
    // the eq option bar's raised panel: a light tint, lit along the top edge, shaded along the bottom, with a faint
    // rim all around; the stage draws its shadow, which reaches outside the panel
    void onNanoDisplay() final
    {
        const float w = getWidth();
        const float h = getHeight();
        const float r = CompressorGroupReference::borderRadius * fScaleFactor;

        beginPath();
        roundedRect(0, 0, w, h, r);
        fillColor(Color(1.f, 1.f, 1.f, 0.05f));
        fill();

        const auto edge = [&](const float y0, const float y1, const Color& from, const Color& to) {
            save();
            intersectScissor(0, std::min(y0, y1), w, std::abs(y1 - y0));
            beginPath();
            roundedRect(0.5f, 0.5f, w - 1.f, h - 1.f, r - 0.5f);
            strokePaint(linearGradient(0, y0, 0, y1, from, to));
            strokeWidth(1.f);
            stroke();
            restore();
        };

        edge(0, r, Color(1.f, 1.f, 1.f, 0.12f), Color(1.f, 1.f, 1.f, 0.f));
        edge(h, h - r, Color(0.f, 0.f, 0.f, 0.22f), Color(0.f, 0.f, 0.f, 0.f));

        beginPath();
        roundedRect(0.5f, 0.5f, w - 1.f, h - 1.f, r - 0.5f);
        strokeColor(Color(1.f, 1.f, 1.f, 0.05f));
        strokeWidth(1.f);
        stroke();
    }
};

template<class Panel, class Display = void>
class CompressorLabeledGroupOf final : public CompressorLabeledGroupWidget
{
public:
    CompressorLabeledGroupOf(LabWidget* const parent, const char* const label, const Color color)
        : CompressorLabeledGroupWidget(parent, label, color)
    {
        if constexpr (! std::is_void_v<Display>)
        {
            fDisplay = addWidget<Display>();
            fHeight += Display::kHeight;
        }

        fPanel = addWidget<Panel>();
        LabWidget::setHeight(d_roundToUnsignedInt(fHeight * fScaleFactor));
    }
};

// --------------------------------------------------------------------------------------------------------------------
// A row of groups under the display, a fixed gap apart. Every number box in the row gets the same width, from what
// the row has left once the groups' fixed parts and the gaps are taken out.

struct CompressorGroupRowReference : Reference::Zero {
    static constexpr const uint padding = kGroupGap;
};

class CompressorGroupRowWidget : public ReferenceContainerWidget<CompressorGroupRowReference>
{
    using R = CompressorGroupRowReference;
    using BaseWidget = ReferenceContainerWidget<R>;

protected:
    std::vector<std::shared_ptr<CompressorLabeledGroupWidget>> fGroups;

    // unscaled, as tall as the tallest group
    uint fHeight = 0;

    template<class Panel, class Display = void>
    void addGroup(const char* const label, const Color color)
    {
        fGroups.push_back(addWidget<CompressorLabeledGroupOf<Panel, Display>>(label, color));
        fHeight = std::max(fHeight, fGroups.back()->getGroupHeight());
        LabWidget::setHeight(d_roundToUnsignedInt(fHeight * fScaleFactor));
    }

public:
    explicit CompressorGroupRowWidget(LabWidget* const parent)
        : BaseWidget(parent) {}

    [[nodiscard]] const std::vector<std::shared_ptr<CompressorLabeledGroupWidget>>& getGroups() const noexcept
    {
        return fGroups;
    }

private:
    void sizeGroups()
    {
        const int gap = d_roundToIntPositive(R::padding * fScaleFactor);
        int available = static_cast<int>(getWidth()) - gap * static_cast<int>(fGroups.size() - 1);
        uint boxes = 0;

        for (const std::shared_ptr<CompressorLabeledGroupWidget>& group : fGroups)
        {
            available -= static_cast<int>(group->getFixedWidth());
            boxes += group->getBoxCount();
        }

        const uint boxWidth = boxes != 0 ? static_cast<uint>(std::max(0, available) / static_cast<int>(boxes)) : 0;

        for (const std::shared_ptr<CompressorLabeledGroupWidget>& group : fGroups)
            group->setGroupWidth(group->getFixedWidth() + group->getBoxCount() * boxWidth);
    }

    // the controls of fixed width know their size only once updated, so the groups are sized again after that
    void updateSize(const bool updateChildren) final
    {
        LabWidget::setHeight(d_roundToUnsignedInt(fHeight * fScaleFactor));
        sizeGroups();
        BaseWidget::updateSize(updateChildren);
        sizeGroups();
        BaseWidget::updateSize(updateChildren);
    }
};

class CompressorResponseRowWidget final : public CompressorGroupRowWidget
{
public:
    explicit CompressorResponseRowWidget(LabWidget* const parent)
        : CompressorGroupRowWidget(parent)
    {
        addGroup<CompressorResponseGroupWidget, CompressorEnvelopeWidget>("COMPRESSION RESPONSE", accentColor<kFaustParameterAttack>());
    }
};

class CompressorDetectorRowWidget final : public CompressorGroupRowWidget
{
public:
    explicit CompressorDetectorRowWidget(LabWidget* const parent)
        : CompressorGroupRowWidget(parent)
    {
        addGroup<CompressorDetectorGroupWidget>("DETECTOR", accentColor<kFaustParameterPeak_rms>());
        addGroup<CompressorSidechainGroupWidget>("SIDECHAIN", accentColor<kFaustParameterSc_mix>());
        fGroups.back()->addHeaderButton<CompressorListenButtonWidget<kFaustParameterSc_listen>>();
        fGroups.back()->addHeaderButton<CompressorChoiceButtonWidget<kFaustParameterSc_filter>>();
    }
};

// --------------------------------------------------------------------------------------------------------------------
// The panels floating over the starfield: dark and see-through, lit along the top edge. The display draws their
// shadows, which reach outside them.

struct CompressorFloatingReference : Reference::Zero {
    static constexpr const uint borderRadius = 10;
    // between the panel's edge and its contents
    static constexpr const uint inset = 10;
    // of the meters and the control stacks
    static constexpr const uint panelWidth = 110;
    static constexpr const Color backgroundColor { Reference::Colors::bg2, 0.75f };
};

static void drawFloatingPanel(NanoVG& vg,
                              const float w,
                              const float h,
                              const float scaleFactor,
                              const Color& backgroundColor = CompressorFloatingReference::backgroundColor)
{
    using R = CompressorFloatingReference;

    const float r = R::borderRadius * scaleFactor;

    vg.beginPath();
    vg.roundedRect(0, 0, w, h, r);
    vg.fillColor(backgroundColor);
    vg.fill();

    vg.beginPath();
    vg.roundedRect(0.5f, 0.5f, w - 1.f, h - 1.f, r - 0.5f);
    vg.strokePaint(vg.linearGradient(0, 0, 0, h, Color(1.f, 1.f, 1.f, 0.14f), Color(1.f, 1.f, 1.f, 0.03f)));
    vg.strokeWidth(1.f);
    vg.stroke();
}

// --------------------------------------------------------------------------------------------------------------------
// The meters floating over the starfield: a title on top, a dB scale down the left, a well with a track for each
// channel, and the channels' names under the tracks, L and R, or M and S with Mid / Side on. 0 dB is at the top of the
// well and the meter's floor at the bottom.

struct CompressorMeterReference : CompressorFloatingReference {
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

struct CompressorMeterStyle {
    const char* title;
    Color color;
    // dB at the top and the bottom of the well
    float top;
    float floor;
    // dB the tracks fill from, toward the value
    float origin;
    // dB of the scale's labels and the ticks across the well, from the top down
    const float* scale;
    size_t scaleCount;
};

// The meter's well, between the title and the channel names, right of the scale and left of the triangle.
// The well is centred across the panel, with the scale to its left and the triangle to its right; x is where the
// scale starts.
struct CompressorMeterGeometry {
    float x, wx, wy, ww, wh;

    CompressorMeterGeometry(const float w, const float h, const float s) noexcept
    {
        using R = CompressorMeterReference;

        ww = (R::Well::border * 2 + R::Track::width * 2 + R::Track::gap) * s;
        wx = std::round((w - ww) * 0.5f);
        x = wx - (R::Scale::width + R::Scale::gap) * s;
        wy = (R::inset + R::Title::height + R::Title::gap) * s;
        wh = h - wy - (R::inset + R::Channel::height + R::Channel::gap) * s;
    }

    // the track area, inside the well's border
    [[nodiscard]] float dbToY(const float db, const float top, const float floor, const float s) const noexcept
    {
        const float b = CompressorMeterReference::Well::border * s;
        return wy + b + (wh - b * 2) * d_clamp((top - db) / (top - floor), 0.f, 1.f);
    }
};

static void drawMeter(NanoVG& vg,
                      const float w,
                      const float h,
                      const float s,
                      const CompressorMeterStyle& style,
                      const std::array<float, 2>& values,
                      const bool midSide)
{
    using R = CompressorMeterReference;

    drawFloatingPanel(vg, w, h, s);

    const CompressorMeterGeometry g(w, h, s);

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
        const float maxWidth = w - R::inset * 2 * s;
        Rectangle<float> bounds;

        vg.fontFace("regular");
        vg.fontSize(R::Title::fontSize * s);
        vg.textLetterSpacing(R::Title::letterSpacing * s);

        if (const float titleWidth = vg.textBounds(0, 0, style.title, nullptr, bounds); titleWidth > maxWidth)
            vg.fontSize(R::Title::fontSize * s * maxWidth / titleWidth);

        vg.textAlign(NanoVG::ALIGN_CENTER | NanoVG::ALIGN_MIDDLE);
        vg.fillColor(R::Title::color);
        vg.text(w * 0.5f, (R::inset + R::Title::height * 0.5f) * s, style.title, nullptr);
        vg.textLetterSpacing(0.f);
    }

    // the well
    vg.beginPath();
    vg.roundedRect(g.wx, g.wy, g.ww, g.wh, wr);
    vg.fillColor(R::Well::backgroundColor);
    vg.fill();

    // the tracks, each cut out of the well's shape so the ends follow its corners
    for (size_t i = 0; i < values.size(); ++i)
    {
        const float y = g.dbToY(values[i], style.top, style.floor, s);
        const float yo = g.dbToY(style.origin, style.top, style.floor, s);
        const float y0 = std::min(y, yo);
        const float y1 = std::max(y, yo);

        if (y1 - y0 < 0.5f)
            continue;

        vg.save();
        vg.scissor(trackX[i], y0, tw, y1 - y0);
        vg.beginPath();
        vg.roundedRect(g.wx + b, ty, g.ww - b * 2, tb - ty, wr - b);
        vg.fillPaint(vg.linearGradient(0, y, 0, yo, style.color, Color(style.color, 0.45f)));
        vg.fill();
        vg.restore();
    }

    // inset shading
    vg.beginPath();
    vg.roundedRect(g.wx, g.wy, g.ww, g.wh, wr);
    vg.fillPaint(vg.linearGradient(0, g.wy, 0, g.wy + 4 * s, R::Well::insetShadingColor, Reference::Colors::transparent));
    vg.fill();

    // the scale: ticks across the well, leaving out its ends, and the dB to their left
    vg.fontSize(R::Scale::fontSize * s);
    vg.textAlign(NanoVG::ALIGN_RIGHT | NanoVG::ALIGN_MIDDLE);
    vg.fillColor(R::Scale::color);

    for (size_t i = 0; i < style.scaleCount; ++i)
    {
        const float db = style.scale[i];
        const float y = g.dbToY(db, style.top, style.floor, s);

        if (d_isNotEqual(db, style.top) && d_isNotEqual(db, style.floor))
        {
            vg.beginPath();
            vg.moveTo(g.wx + b, std::round(y) + 0.5f);
            vg.lineTo(g.wx + g.ww - b, std::round(y) + 0.5f);
            vg.strokeColor(R::Tick::color);
            vg.strokeWidth(1.f);
            vg.stroke();
        }

        char dbstr[8];
        std::snprintf(dbstr, sizeof(dbstr), db > 0.f ? "+%.0f" : "%.0f", db);
        vg.text(g.x + R::Scale::width * s, y, dbstr, nullptr);
    }

    // the channels' names, under their tracks
    {
        const char* const names[2][2] = { { "L", "R" }, { "M", "S" } };

        vg.fontSize(R::Channel::fontSize * s);
        vg.textAlign(NanoVG::ALIGN_CENTER | NanoVG::ALIGN_MIDDLE);
        vg.fillColor(R::Channel::color);

        const float y = g.wy + g.wh + (R::Channel::gap + R::Channel::height * 0.5f) * s;
        for (size_t i = 0; i < trackX.size(); ++i)
            vg.text(trackX[i] + tw * 0.5f, y, names[midSide ? 1 : 0][i], nullptr);
    }
}

// --------------------------------------------------------------------------------------------------------------------
// The input meter: the detector level of each channel, which is what the threshold is compared against, with the
// threshold's triangle right of the well and its line across it. Dragging it sets the threshold.

class CompressorInputMeterWidget final : public LabKnobWidget
{
    using R = CompressorMeterReference;

    static constexpr const FaustParameter& kParameterSlider = kFaustParameters[kFaustParameterThreshold];
    static constexpr const std::array<FaustParameterIndex, 2> kMeters { kFaustParameterLevel_1, kFaustParameterLevel_2 };
    static constexpr const float kFloor = kFaustParameters[kFaustParameterLevel_1].min;
    static constexpr const float kScale[] = { 0, -12, -24, -36, -48, -60 };

    std::array<float, 2> fValues { kFloor, kFloor };
    bool fMidSide = false;

public:
    // unscaled
    static constexpr const uint kWidth = R::panelWidth;

    explicit CompressorInputMeterWidget(LabWidget* const parent)
        : LabKnobWidget(parent, kParametersMainStart + kFaustParameterThreshold)
    {
        setName(kParameterSlider.label);
        setDefault(kParameterSlider.init);
        setRange(kParameterSlider.min, kParameterSlider.max);
        setStep(kParameterSlider.step);
        setValue(kParameterSlider.init, false);
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
                                               kFloor, 0.f); d_isNotEqual(fValues[i], value))
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

        LabKnobWidget::idleCallback();
    }

    void onNanoDisplay() final
    {
        const float s = fScaleFactor;
        const float w = getWidth();
        const float h = getHeight();
        const Color& color = accentColor<kFaustParameterThreshold>();

        drawMeter(*this, w, h, s, { "INPUT", color, 0.f, kFloor, kFloor, kScale, std::size(kScale) }, fValues, fMidSide);

        // the threshold: a triangle pointing at the well, and a line across it
        const CompressorMeterGeometry g(w, h, s);

        if (g.wh <= 0.f)
            return;

        const float y = g.dbToY(getValue(), 0.f, kFloor, s);
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
// The gain reduction meter: each channel's reduction with Makeup added, filling from 0 dB, down for a net cut and up
// for a net boost, between the meter's ceiling and floor.

class CompressorGainReductionMeterWidget final : public LabWidget,
                                                 private IdleCallback
{
    using R = CompressorMeterReference;

    static constexpr const std::array<FaustParameterIndex, 2> kMeters { kFaustParameterGr_1, kFaustParameterGr_2 };
    static constexpr const float kCeiling = kFaustParameters[kFaustParameterGr_1].max;
    static constexpr const float kFloor = kFaustParameters[kFaustParameterGr_1].min;
    static constexpr const float kScale[] = { 6, 3, 0, -3, -6, -9, -12 };

    std::array<float, 2> fValues {};
    bool fMidSide = false;

public:
    // unscaled
    static constexpr const uint kWidth = R::panelWidth;

    explicit CompressorGainReductionMeterWidget(LabWidget* const parent)
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
                                               kFloor, kCeiling); d_isNotEqual(fValues[i], value))
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
        drawMeter(*this, getWidth(), getHeight(), fScaleFactor,
                  { "GAIN", accentColor<kFaustParameterMakeup>(), kCeiling, kFloor, 0.f, kScale, std::size(kScale) },
                  fValues, fMidSide);
    }
};

// --------------------------------------------------------------------------------------------------------------------
// Controls stacked in a floating panel: number boxes, one above the other, in the height the display gives it.

struct CompressorControlStackReference : CompressorFloatingReference {
    static constexpr const uint margin = inset;
    static constexpr const uint padding = CompressorGroupReference::padding;
    static constexpr const uint width = panelWidth;
};

class CompressorControlStackWidget : public ReferenceContainerWidget<CompressorControlStackReference, kVertical>
{
    using R = CompressorControlStackReference;
    using BaseWidget = ReferenceContainerWidget<R, kVertical>;

public:
    // unscaled
    static constexpr const uint kWidth = R::width;

    explicit CompressorControlStackWidget(LabWidget* const parent)
        : BaseWidget(parent) {}

    [[nodiscard]] float getBorderRadius() const noexcept
    {
        return R::borderRadius * fScaleFactor;
    }

private:
    void onNanoDisplay() final
    {
        drawFloatingPanel(*this, getWidth(), getHeight(), fScaleFactor);
    }
};

// The compression controls, next to the input meter, at the height of a group's boxes: threshold and ratio at the top,
// knee and range at the bottom, the spacer between them taking what is left.

class CompressorThresholdStackWidget final : public CompressorControlStackWidget
{
    const std::array<std::shared_ptr<LabWidget>, 5> widgets {
        addWidget<CompressorNumberBoxWidget<kFaustParameterThreshold>>(kFaustParameterThreshold),
        addWidget<CompressorNumberBoxWidget<kFaustParameterRatio>>(kFaustParameterRatio),
        addSpacer(),
        addWidget<CompressorNumberBoxWidget<kFaustParameterKnee>>(kFaustParameterKnee),
        addWidget<CompressorNumberBoxWidget<kFaustParameterRange>>(kFaustParameterRange),
    };

public:
    explicit CompressorThresholdStackWidget(LabWidget* const parent)
        : CompressorControlStackWidget(parent) {}
};

// The output controls, next to the gain reduction meter, at the height of a group's boxes: drive and Mid / Side at the
// top, makeup and dry/wet at the bottom, the spacer between them taking what is left.

class CompressorOutputStackWidget final : public CompressorControlStackWidget
{
    const std::array<std::shared_ptr<LabWidget>, 5> widgets {
        addWidget<CompressorNumberBoxWidget<kFaustParameterDrive>>(kFaustParameterDrive),
        addWidget<CompressorPowerBoxWidget<kFaustParameterMid_side>>(kFaustParameterMid_side),
        addSpacer(),
        addWidget<CompressorNumberBoxWidget<kFaustParameterMakeup>>(kFaustParameterMakeup),
        addWidget<CompressorNumberBoxWidget<kFaustParameterDrywet>>(kFaustParameterDrywet),
    };

public:
    explicit CompressorOutputStackWidget(LabWidget* const parent)
        : CompressorControlStackWidget(parent) {}
};

// --------------------------------------------------------------------------------------------------------------------
// The compression curve: output level against input level, both from -60 to 0 dB, for a steady signal, worked out with
// the gain computer and the Range ceiling of compressor.dsp. Makeup is left out, so 1:1 is the diagonal. It floats
// over the starfield, in a panel as wide as the display leaves it, the plot filling it edge to edge under a pane of glass.

struct CompressorCurveReference : CompressorFloatingReference {
    // see-through, the starfield showing behind the plot
    static constexpr const Color backgroundColor { Reference::Colors::bg0, 0.f };
    static constexpr const float dbMin = -60.f;
    static constexpr const float gridStep = 12.f;
    static constexpr const Color gridColor { 1.f, 1.f, 1.f, 0.06f };
    static constexpr const Color unityColor { 1.f, 1.f, 1.f, 0.14f };
    static constexpr const Color thresholdColor { 1.f, 1.f, 1.f, 0.2f };
    // mbComp5's curve colours, from left to right, blended between
    static constexpr const std::array<Color, 5> curveColors {{
        { 255, 191, 203 }, { 255, 223, 173 }, { 210, 253, 211 }, { 190, 241, 255 }, { 218, 193, 243 }
    }};
    // the area under the curve is filled in columns, each in the curve's colour above it
    static constexpr const uint fillColumns = 32;
    // the point at the detector's level, unscaled
    static constexpr const float pointRadius = 3.f;
    static constexpr const float pointGlowRadius = 7.f;
    static constexpr const Color pointColor { 1.f, 1.f, 1.f, 1.f };
};

class CompressorCurveWidget final : public LabWidget,
                                    private IdleCallback
{
    using R = CompressorCurveReference;

    static constexpr const std::array<FaustParameterIndex, 4> kParameters {
        kFaustParameterThreshold,
        kFaustParameterRatio,
        kFaustParameterKnee,
        kFaustParameterRange,
    };

    // the detector's level meters, the curve's input
    static constexpr const std::array<FaustParameterIndex, 2> kLevels { kFaustParameterLevel_1, kFaustParameterLevel_2 };
    static constexpr const float kLevelFloor = kFaustParameters[kFaustParameterLevel_1].min;

    std::array<float, kParameters.size()> fValues {};

    // the louder channel's
    float fLevel = kLevelFloor;

public:
    explicit CompressorCurveWidget(LabWidget* const parent)
        : LabWidget(parent)
    {
        for (size_t i = 0; i < kParameters.size(); ++i)
            fValues[i] = kFaustParameters[kParameters[i]].init;

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

    // gain reduction in dB for an input level, held above the Range ceiling
    [[nodiscard]] float gainReduction(const float level) const noexcept
    {
        const float threshold = fValues[0];
        const float slope = 1.f - 1.f / std::max(1.f, fValues[1]);
        const float knee = fValues[2];
        const float range = fValues[3];

        // the gain computer, with a quadratic across the knee
        const float over = level - threshold;
        float gr;
        if (over <= -knee / 2)
            gr = 0.f;
        else if (over > knee / 2)
            gr = -slope * over;
        else
            gr = -slope * (over + knee / 2) * (over + knee / 2) / (2 * std::max(1e-9f, knee));

        // the Range ceiling, eased into over a knee as wide as Knee and at most twice the Range
        const float lim = -range;
        const float d = gr - lim;
        const float rk = std::min(knee, 2 * range);
        if (d <= -rk / 2)
            return lim;
        if (d > rk / 2)
            return gr;
        return lim + (d + rk / 2) * (d + rk / 2) / (2 * std::max(1e-9f, rk));
    }

    void idleCallback() final
    {
        bool changed = false;

        for (size_t i = 0; i < kParameters.size(); ++i)
        {
            if (const float value = fInterface->getParameterValue(kParametersMainStart + kParameters[i]);
                d_isNotEqual(fValues[i], value))
            {
                fValues[i] = value;
                changed = true;
            }
        }

        float level = kLevelFloor;
        for (const FaustParameterIndex id : kLevels)
            level = std::max(level, fInterface->getParameterValue(kParametersMainStart + id));

        if (d_isNotEqual(fLevel, level))
        {
            fLevel = level;
            changed = true;
        }

        if (changed)
            repaint();
    }

    void onNanoDisplay() final
    {
        const float s = fScaleFactor;
        const float w = getWidth();
        const float h = getHeight();

        drawFloatingPanel(*this, w, h, s, R::backgroundColor);

        // the plot fills the panel, edge to edge
        const float px = 0.f;
        const float py = 0.f;
        const float pw = w;
        const float ph = h;
        const float r = getBorderRadius();

        if (pw <= 0.f || ph <= 0.f)
            return;

        const auto dbToX = [&](const float db) {
            return px + (db - R::dbMin) / -R::dbMin * pw;
        };
        const auto dbToY = [&](const float db) {
            return py + ph - (db - R::dbMin) / -R::dbMin * ph;
        };

        save();
        intersectScissor(px, py, pw, ph);

        // grid every 12 dB, across and down, leaving the panel's edges to its rim
        beginPath();
        for (float db = R::dbMin + R::gridStep; db < 0.f; db += R::gridStep)
        {
            const float gx = std::round(dbToX(db)) + 0.5f;
            const float gy = std::round(dbToY(db)) + 0.5f;
            moveTo(gx, py);
            lineTo(gx, py + ph);
            moveTo(px, gy);
            lineTo(px + pw, gy);
        }
        strokeColor(R::gridColor);
        strokeWidth(1.f);
        stroke();

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

        // 1:1
        clippedStroke([&] { strokeColor(R::unityColor); }, 1.f, [&](float, float) {
            beginPath();
            moveTo(dbToX(R::dbMin), dbToY(R::dbMin));
            lineTo(dbToX(0.f), dbToY(0.f));
        });

        // the threshold, up to where it meets the diagonal and across to the left
        {
            const float tx = std::round(dbToX(fValues[0])) + 0.5f;
            const float ty = std::round(dbToY(fValues[0])) + 0.5f;

            clippedStroke([&] { strokeColor(R::thresholdColor); }, 1.f, [&](float, float) {
                beginPath();
                moveTo(tx, py + ph);
                lineTo(tx, ty);
                lineTo(px, ty);
            });
        }

        // The curve a pixel at a time from x0 to x1, held to the outline when it is to be filled, which for the area
        // under it is the same as cutting it there.
        const auto curve = [&](const float x0, const float x1, const bool held) {
            beginPath();
            const float first = std::max(0.f, std::floor(x0 - px));
            const float last = std::min(pw, std::ceil(x1 - px));
            for (float x = first; x <= last; x += 1.f)
            {
                const float in = R::dbMin + x / pw * -R::dbMin;
                float y = dbToY(in + gainReduction(in));

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

        // The curve's line, cut exactly to the outline, taking the curve only as far past each piece as the line is
        // wide. It is drawn in a section between each two of the curve's colours, blended across it.
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
                        curve(std::max(x0, left) - width - 1.f, std::min(x1, right) + width + 1.f, false);
                    });
                restore();
            }
        };

        // Filled down to the bottom, round its corners as the panel's, in columns each fading down from the curve's
        // colour above it, then the line with a glow.
        for (uint i = 0; i < R::fillColumns; ++i)
        {
            const float left = std::round(px + pw * i / R::fillColumns);
            const float right = std::round(px + pw * (i + 1) / R::fillColumns);
            const float t = (i + 0.5f) / R::fillColumns;

            save();
            intersectScissor(left, py, right - left, ph);
            curve(px, px + pw, true);
            lineTo(px + pw, py + ph - r);
            arcTo(px + pw, py + ph, px + pw - r, py + ph, r);
            lineTo(px + r, py + ph);
            arcTo(px, py + ph, px, py + ph - r, r);
            closePath();
            fillPaint(linearGradient(0, py, 0, py + ph, curveColor(t, 0.12f), curveColor(t, 0.02f)));
            fill();
            restore();
        }

        line(0.2f, 4.f * s);
        line(1.f, 1.5f * s);

        // the point at the detector's level, on the curve, in the curve's colour there with a white core
        if (fLevel > std::max(kLevelFloor, R::dbMin))
        {
            const float x = dbToX(fLevel);
            const float y = dbToY(fLevel + gainReduction(fLevel));
            const Color color = curveColor((fLevel - R::dbMin) / -R::dbMin, 1.f);

            beginPath();
            circle(x, y, R::pointGlowRadius * s);
            fillPaint(radialGradient(x, y, R::pointRadius * s, R::pointGlowRadius * s,
                                     Color(color, 0.5f), Color(color, 0.f)));
            fill();

            beginPath();
            circle(x, y, R::pointRadius * s);
            fillColor(color);
            fill();

            beginPath();
            circle(x, y, R::pointRadius * 0.5f * s);
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
// The display: the starfield fills it, inside a well border, with panels floating over it, all as tall as the display
// less a margin above and below. The input meter floats at its left, the same margin
// in from the edge, and next to the meter the compression controls. The gain reduction meter floats at the right, the
// same margin in, with the channel and output controls left of it. The compression curve fills the space between the
// two stacks of controls.

class CompressorDisplayWidget final : public LabWidget
{
    using R = Reference::Stage;

    // between the floating panels and the display's edges, as between the panels' edges and their contents
    static constexpr const uint kPanelMargin = CompressorFloatingReference::inset;
    // between the floating panels side by side
    static constexpr const uint kPanelGap = 8;

    const std::unique_ptr<CompressorInputMeterWidget> fInputMeter { new CompressorInputMeterWidget(this) };
    const std::unique_ptr<CompressorThresholdStackWidget> fControls { new CompressorThresholdStackWidget(this) };
    const std::unique_ptr<CompressorOutputStackWidget> fOutputs { new CompressorOutputStackWidget(this) };
    const std::unique_ptr<CompressorCurveWidget> fCurve { new CompressorCurveWidget(this) };
    const std::unique_ptr<CompressorGainReductionMeterWidget> fGainReduction {
        new CompressorGainReductionMeterWidget(this)
    };

public:
    explicit CompressorDisplayWidget(LabWidget* const parent)
        : LabWidget(parent) {}

    [[nodiscard]] float getBorderRadius() const noexcept
    {
        return R::borderRadius * fScaleFactor;
    }

private:
    void onNanoDisplay() final
    {
        const float s = fScaleFactor;
        const float w = getWidth();
        const float h = getHeight();

        // The display's rounded corners are covered with the window background, the same gradient over the window's
        // height that LibreAudioBaseUI paints, and the same radius the starfield stops at.
        {
            const float r = getBorderRadius();
            const float y0 = -static_cast<float>(getAbsoluteY());

            const auto corner = [&](const float cx, const float cy, const float dx, const float dy) {
                moveTo(cx, cy);
                lineTo(cx + dx * r, cy);
                arcTo(cx, cy, cx, cy + dy * r, r);
                closePath();
            };

            beginPath();
            corner(0, 0, 1.f, 1.f);
            corner(w, 0, -1.f, 1.f);
            corner(w, h, -1.f, -1.f);
            corner(0, h, 1.f, -1.f);
            fillPaint(linearGradient(0, y0, 0, y0 + getWindow().getHeight(),
                                     Reference::Window::backgroundGradientStart,
                                     Reference::Window::backgroundGradientStop));
            fill();
        }

        // the floating panels' shadows, as the group panels have theirs: 4 px down, blurred 14 px, pulled in 6 px, left
        // out where the panel is
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

        shadow(*fInputMeter, fInputMeter->getBorderRadius());
        shadow(*fControls, fControls->getBorderRadius());
        shadow(*fOutputs, fOutputs->getBorderRadius());
        shadow(*fCurve, fCurve->getBorderRadius());
        shadow(*fGainReduction, fGainReduction->getBorderRadius());

        // well border, over everything
        const float border = std::max(1, d_roundToIntPositive(R::border * s));
        const float half = border * 0.5f;

        beginPath();
        roundedRect(half, half, w - border, h - border, R::borderRadius * s - half);
        strokeColor(R::borderColor);
        strokeWidth(border);
        stroke();
    }

    // the panels follow the display, which the stage has placed by the time this runs a second time
    void updateSize(const bool updateChildren) final
    {
        const int margin = d_roundToIntPositive(kPanelMargin * fScaleFactor);
        const int side = std::max(1, static_cast<int>(getHeight()) - margin * 2);

        fInputMeter->setSize(d_roundToUnsignedInt(CompressorInputMeterWidget::kWidth * fScaleFactor), side);
        fInputMeter->setAbsolutePos(getAbsoluteX() + margin, getAbsoluteY() + margin);

        const int grWidth = d_roundToIntPositive(CompressorGainReductionMeterWidget::kWidth * fScaleFactor);
        fGainReduction->setSize(grWidth, side);
        fGainReduction->setAbsolutePos(getAbsoluteX() + static_cast<int>(getWidth()) - margin - grWidth,
                                       getAbsoluteY() + margin);

        const int gap = d_roundToIntPositive(kPanelGap * fScaleFactor);

        // as widgets, their layouts having a setSize and setAbsolutePos of their own
        LabWidget& controls = *fControls;
        controls.setSize(d_roundToUnsignedInt(CompressorThresholdStackWidget::kWidth * fScaleFactor), side);
        controls.setAbsolutePos(fInputMeter->getAbsoluteX() + static_cast<int>(fInputMeter->getWidth()) + gap,
                                getAbsoluteY() + margin);

        LabWidget& outputs = *fOutputs;
        outputs.setSize(d_roundToUnsignedInt(CompressorOutputStackWidget::kWidth * fScaleFactor), side);
        outputs.setAbsolutePos(fGainReduction->getAbsoluteX() - gap - static_cast<int>(outputs.getWidth()),
                               getAbsoluteY() + margin);

        // the curve takes what is left between the two stacks
        const int curveX = controls.getAbsoluteX() + static_cast<int>(controls.getWidth()) + gap;
        fCurve->setSize(std::max(1, outputs.getAbsoluteX() - gap - curveX), side);
        fCurve->setAbsolutePos(curveX, getAbsoluteY() + margin);

        LabWidget::updateSize(updateChildren);
    }
};

// --------------------------------------------------------------------------------------------------------------------
// The stage: the display on top, and under it the rows of control groups. There is only Expert mode for now, so the
// groups show in Easy mode too; on the other pages the display takes the whole stage.

class CompressorStageWidget final : public ReferenceContainerWidget<Reference::TransparentStage, kVertical>,
                                    private IdleCallback
{
    using R = Reference::TransparentStage;
    using BaseWidget = ReferenceContainerWidget<R, kVertical>;

    const std::shared_ptr<CompressorDisplayWidget> fDisplay = addWidget<CompressorDisplayWidget, Expanding>();
    const std::array<std::shared_ptr<CompressorGroupRowWidget>, 2> fRows {
        addWidget<CompressorResponseRowWidget>(),
        addWidget<CompressorDetectorRowWidget>(),
    };

    Page fPage = kPageEasy;

public:
    explicit CompressorStageWidget(LabWidget* const parent)
        : BaseWidget(parent)
    {
        showPage(getCurrentPage(fInterface));
        addIdleCallback(this);
    }

    [[nodiscard]] const CompressorDisplayWidget& getDisplay() const noexcept
    {
        return *fDisplay;
    }

private:
    void showPage(const Page page)
    {
        fPage = page;
        for (const std::shared_ptr<CompressorGroupRowWidget>& row : fRows)
            row->setVisible(page == kPageEasy || page == kPageExpert);
    }

    void idleCallback() final
    {
        if (const Page page = getCurrentPage(fInterface); page != fPage)
        {
            showPage(page);
            updateSize(true);
            repaint();
        }
    }

    // Shadows under the group panels, drawn here as they reach outside the panels: 4 px down, blurred 14 px, pulled
    // in 6 px, left out where the panel is.
    void onNanoDisplay() final
    {
        BaseWidget::onNanoDisplay();

        const float s = fScaleFactor;
        const float r = CompressorGroupReference::borderRadius * s;

        for (const std::shared_ptr<CompressorGroupRowWidget>& row : fRows)
        {
            if (! row->isVisible())
                continue;

            for (const std::shared_ptr<CompressorLabeledGroupWidget>& group : row->getGroups())
                drawGroupShadow(*group, s, r);
        }
    }

    void drawGroupShadow(const CompressorLabeledGroupWidget& group, const float s, const float r)
    {
        const float x = group.getAbsoluteX() - getAbsoluteX();
        const float y = group.getAbsoluteY() - getAbsoluteY();
        const float w = group.getWidth();
        const float h = group.getHeight();

        beginPath();
        rect(x - 20.f * s, y, w + 40.f * s, h + 24.f * s);
        roundedRect(x, y, w, h, r);
        pathWinding(CW);
        fillPaint(boxGradient(x + 6.f * s, y + 10.f * s, w - 12.f * s, h - 12.f * s, r, 14.f * s,
                              Color(0.f, 0.f, 0.f, 0.6f), Color(0.f, 0.f, 0.f, 0.f)));
        fill();
    }
};

// --------------------------------------------------------------------------------------------------------------------
// The starfield fills the display only, not the controls under it, so the main area reports the display's area.

class CompressorMainAreaWidget final : public MainAreaContainerWidget<CompressorStageWidget>
{
public:
    explicit CompressorMainAreaWidget(LabTopLevelWidget* const parent)
        : MainAreaContainerWidget(parent) {}

    [[nodiscard]] Point<int> getDisplayAbsolutePos() const noexcept
    {
        return fStage->getDisplay().getAbsolutePos();
    }

    [[nodiscard]] Size<uint> getDisplaySize() const noexcept
    {
        return fStage->getDisplay().getSize();
    }

    [[nodiscard]] float getDisplayBorderRadius() const noexcept
    {
        return fStage->getDisplay().getBorderRadius();
    }
};

// The starfield sits on the display, and follows it as switching pages changes its height.

class CompressorRootWidget final : public RootBaseWidget,
                                   private IdleCallback
{
    std::shared_ptr<TopBar> fTopBar = addWidget<TopBar>();
    std::shared_ptr<CompressorMainAreaWidget> fMainArea = addWidget<CompressorMainAreaWidget, Expanding>();

    Point<int> fLastDisplayPos;
    Size<uint> fLastDisplaySize;

public:
    CompressorRootWidget(Window& window, LabUIWidgetInterface* const iface)
        : RootBaseWidget(window, iface)
    {
        addIdleCallback(this);
    }

private:
    void idleCallback() final
    {
        if (fMainArea->getDisplayAbsolutePos() != fLastDisplayPos || fMainArea->getDisplaySize() != fLastDisplaySize)
            updateSize(false);
    }

    void updateSize(const bool updateChildren) final
    {
        RootBaseWidget::updateSize(updateChildren);

        fLastDisplayPos = fMainArea->getDisplayAbsolutePos();
        fLastDisplaySize = fMainArea->getDisplaySize();

        for (ShaderBaseWidget* const sw : fShaders)
        {
            sw->setAbsolutePos(fLastDisplayPos);
            sw->setSize(fLastDisplaySize);
            sw->setBorderRadius(fMainArea->getDisplayBorderRadius());
        }
    }
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

class CompressorUI final : public LibreAudioBaseUI
{
    const std::unique_ptr<LibreAudio::ShaderBaseWidget> fShaderBackground {
        new LibreAudio::BotShaderWidget<SHADERS_SHADERTOY_CLOUDSTARFIELD_FRAG_DATA,
                                        SHADERS_SHADERTOY_CLOUDSTARFIELD_FRAG_LEN>(this, this)
    };

public:
    CompressorUI()
        : LibreAudioBaseUI()
    {
        createRootWidget<LibreAudio::CompressorRootWidget>();

        fRootWidget->enableShaders({ fShaderBackground.get() });
    }

private:
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompressorUI)
};

UI* createUI()
{
    return new CompressorUI();
}

END_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------
