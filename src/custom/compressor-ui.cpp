// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "LibreAudioBaseUI.hpp"

#include "ui/containers/main-area.hpp"
#include "ui/containers/top-bar.hpp"
#include "ui/widgets/number-box.hpp"
#include "ui/widgets/power-box.hpp"
#include "ui/widgets/shader.hpp"
#include "ui/widgets/threshold-meter.hpp"

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

    // width without the number boxes: panel margins, the space between controls and any control of its own width
    [[nodiscard]] uint getFixedWidth() const noexcept
    {
        const uint margin = d_roundToUnsignedInt((R::border + R::margin) * fScaleFactor);
        const uint padding = d_roundToUnsignedInt(R::padding * fScaleFactor);

        uint width = margin * 2 + padding * (static_cast<uint>(Layout::widgets.size()) - 1);
        for (const SubWidgetWithSizeHint& s : Layout::widgets)
            if (s.sizeHint == Fixed)
                width += s.widget->getWidth();
        return width;
    }

    void setGroupWidth(const uint width)
    {
        LabWidget::setWidth(width);
    }
};

// --------------------------------------------------------------------------------------------------------------------
// How the gain moves: lookahead, attack, hold and release, with their curves.

class CompressorResponseGroupWidget final : public CompressorGroupWidget
{
    const std::array<std::shared_ptr<LabWidget>, 8> widgets {
        addWidget<CompressorNumberBoxWidget<kFaustParameterLookahead>, Expanding>(kFaustParameterLookahead),
        addWidget<CompressorNumberBoxWidget<kFaustParameterAttack>, Expanding>(kFaustParameterAttack),
        addWidget<CompressorNumberBoxWidget<kFaustParameterAttack_curve>, Expanding>(kFaustParameterAttack_curve),
        addWidget<CompressorNumberBoxWidget<kFaustParameterHold>, Expanding>(kFaustParameterHold),
        addWidget<CompressorNumberBoxWidget<kFaustParameterRelease>, Expanding>(kFaustParameterRelease),
        addWidget<CompressorNumberBoxWidget<kFaustParameterRelease_curve>, Expanding>(kFaustParameterRelease_curve),
        addWidget<CompressorPowerBoxWidget<kFaustParameterRelease_s>, Expanding>(kFaustParameterRelease_s, "S-Curve"),
        addWidget<CompressorNumberBoxWidget<kFaustParameterAuto_release>, Expanding>(kFaustParameterAuto_release),
    };

public:
    explicit CompressorResponseGroupWidget(LabWidget* const parent)
        : CompressorGroupWidget(parent, 8) {}
};

// What the detector measures and how the channels are linked.

class CompressorDetectorGroupWidget final : public CompressorGroupWidget
{
    const std::array<std::shared_ptr<LabWidget>, 5> widgets {
        addWidget<CompressorNumberBoxWidget<kFaustParameterPeak_rms>, Expanding>(kFaustParameterPeak_rms),
        addWidget<CompressorNumberBoxWidget<kFaustParameterRms_time>, Expanding>(kFaustParameterRms_time),
        addWidget<CompressorNumberBoxWidget<kFaustParameterFeedback>, Expanding>(kFaustParameterFeedback),
        addWidget<CompressorNumberBoxWidget<kFaustParameterLink>, Expanding>(kFaustParameterLink),
        addWidget<CompressorPowerBoxWidget<kFaustParameterMid_side>, Expanding>(kFaustParameterMid_side),
    };

public:
    explicit CompressorDetectorGroupWidget(LabWidget* const parent)
        : CompressorGroupWidget(parent, 5) {}
};

// The sidechain source and the filters shaping what the detector hears.

class CompressorSidechainGroupWidget final : public CompressorGroupWidget
{
    const std::array<std::shared_ptr<LabWidget>, 4> widgets {
        addWidget<CompressorNumberBoxWidget<kFaustParameterSc_mix>, Expanding>(kFaustParameterSc_mix),
        addWidget<CompressorNumberBoxWidget<kFaustParameterSc_hp>, Expanding>(kFaustParameterSc_hp),
        addWidget<CompressorNumberBoxWidget<kFaustParameterSc_lp>, Expanding>(kFaustParameterSc_lp),
        addWidget<CompressorNumberBoxWidget<kFaustParameterSc_tilt>, Expanding>(kFaustParameterSc_tilt),
    };

public:
    explicit CompressorSidechainGroupWidget(LabWidget* const parent)
        : CompressorGroupWidget(parent, 4) {}
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
    static constexpr const float kSPeakMid = 0.45f;
    static constexpr const float kSPeakSpread = 1.8f;
    static constexpr const float kSBound = 64.f;
    static constexpr const float kSSpanFloor = 0.5f;
    static constexpr const float kAutoRefDb = 6.f;

    // depth of the step, in dB
    static constexpr const float kDepth = 20.f;
    // integration steps per ramp time, and the most a ramp may take
    static constexpr const float kStepsPerTime = 1000.f;
    static constexpr const uint kMaxSteps = 20000;

    static constexpr const std::array<FaustParameterIndex, 8> kParameters {
        kFaustParameterLookahead,
        kFaustParameterAttack,
        kFaustParameterAttack_curve,
        kFaustParameterHold,
        kFaustParameterRelease,
        kFaustParameterRelease_curve,
        kFaustParameterRelease_s,
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
        const float norm = 1.f / (1.f - std::min(0.f, curve));
        return norm * kCurveRef / time * std::pow(std::clamp(dist / kCurveRef, 1.f / kCurveBound, kCurveBound), curve);
    }

    // dB per second of the S-curve release at a distance from the target, over a span
    static float sCurveRate(const float dist, const float span, const float time, const float curve) noexcept
    {
        const float peak = kSPeakMid * std::pow(kSPeakSpread, curve);
        const float norm = peak * (std::log(kSBound) + 1.f) / 2.f + (1.f + 1.f / (kSBound * kSBound)) / (4.f * peak);
        const float w = std::clamp(dist / span, 1.f / kSBound, 1.f);
        return kCurveRef / time * norm * 2.f * peak * w / (peak * peak + w * w);
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
        const float attackCurve = fValues[2] / 100.f;
        const float release = std::max(1e-3f, fValues[4] * 0.001f);
        const float releaseCurve = fValues[5] / 100.f;
        const bool sCurve = fValues[6] > 0.5f;
        const float autoRelease = fValues[7] / 100.f;

        fLookahead = fValues[0] * 0.001f;

        fAttack = ramp(0.f, -kDepth, attack, [=](const float dist) {
            return powerRate(dist, attack, attackCurve);
        });

        // the span is the peak-hold of the distance, fading over four times the release
        const auto releaseRamp = [=](const float time) {
            if (! sCurve)
                return ramp(-kDepth, 0.f, time, [=](const float dist) {
                    return powerRate(dist, time, releaseCurve);
                });

            const float pole = std::exp(-time / kStepsPerTime / std::max(0.05f, 4.f * release));
            float span = kDepth;
            return ramp(-kDepth, 0.f, time, [=](const float dist) mutable {
                span = std::max(kSSpanFloor, std::max(dist, span * pole));
                return sCurveRate(dist, span, time, releaseCurve);
            });
        };

        // Each section is as long as the slowest power-law curve, +1, takes to land, and its ramp is stretched to fill
        // it, so the curve controls and Release S-Curve show only as the ramps' shapes and leave their lengths alone.
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
            fValues[3] * 0.001f,
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
            const std::array<float, 3> times { fValues[1], fValues[3], fValues[4] };

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
// A group's header, styled like the reverb's section headers: its name in the group's accent colour, at the panel's
// top left, in a 12 px row 8 px above the controls.

struct CompressorGroupHeaderReference : Reference::Zero {
    static constexpr const float fontSize = 12.f;
    static constexpr const float letterSpacing = 0.4f;
    // the label's row, as tall as its font, so it sits as far from the panel's top as from its left
    static constexpr const uint row = 12;
    static constexpr const uint gap = 8;
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

public:
    CompressorGroupHeaderWidget(LabWidget* const parent, const char* const label, const Color color)
        : LabWidget(parent),
          fLabel(label),
          fColor(color)
    {
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
    static constexpr const uint padding = 8;
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
    }
};

// --------------------------------------------------------------------------------------------------------------------
// The panels floating over the starfield: dark and see-through, lit along the top edge. The display draws their
// shadows, which reach outside them.

struct CompressorFloatingReference : Reference::Zero {
    static constexpr const uint borderRadius = 10;
    // between the panel's edge and its contents
    static constexpr const uint inset = 10;
    static constexpr const Color backgroundColor { Reference::Colors::bg0, 0.6f };
};

static void drawFloatingPanel(NanoVG& vg, const float w, const float h, const float scaleFactor)
{
    using R = CompressorFloatingReference;

    const float r = R::borderRadius * scaleFactor;

    vg.beginPath();
    vg.roundedRect(0, 0, w, h, r);
    vg.fillColor(R::backgroundColor);
    vg.fill();

    vg.beginPath();
    vg.roundedRect(0.5f, 0.5f, w - 1.f, h - 1.f, r - 0.5f);
    vg.strokePaint(vg.linearGradient(0, 0, 0, h, Color(1.f, 1.f, 1.f, 0.14f), Color(1.f, 1.f, 1.f, 0.03f)));
    vg.strokeWidth(1.f);
    vg.stroke();
}

// --------------------------------------------------------------------------------------------------------------------
// The input meter: mbComp5's band meter on the detector level of each channel, which is what the threshold is
// compared against, with the threshold's line and triangle over it. Dragging it sets the threshold.

class CompressorInputMeterWidget final : public LabWidget
{
    using R = CompressorFloatingReference;
    using MR = Reference::Widgets::ThresholdMeter<accentColor<kFaustParameterThreshold>()>;
    using MeterWidget = ThresholdMeterWidget<MR>;

    const std::unique_ptr<MeterWidget> fMeter {
        new MeterWidget(this, kFaustParameterThreshold, kFaustParameterLevel_1, kFaustParameterLevel_2)
    };

public:
    // unscaled
    static constexpr const uint kWidth = MR::width + R::inset * 2;

    explicit CompressorInputMeterWidget(LabWidget* const parent)
        : LabWidget(parent) {}

    [[nodiscard]] float getBorderRadius() const noexcept
    {
        return R::borderRadius * fScaleFactor;
    }

private:
    void onNanoDisplay() final
    {
        drawFloatingPanel(*this, getWidth(), getHeight(), fScaleFactor);
    }

    // the meter fills the panel less its inset
    void updateSize(const bool updateChildren) final
    {
        const int inset = d_roundToIntPositive(R::inset * fScaleFactor);

        fMeter->setHeight(std::max(1, static_cast<int>(getHeight()) - inset * 2));
        fMeter->setAbsolutePos(getAbsoluteX() + inset, getAbsoluteY() + inset);

        LabWidget::updateSize(updateChildren);
    }
};

// --------------------------------------------------------------------------------------------------------------------
// The gain reduction meter: the input meter's tracks, left and right, without the threshold, filling down from the top
// as each channel is pulled down, to the meter's floor. It floats in a panel like the input meter's.

class CompressorGainReductionMeterWidget final : public LabWidget,
                                                 private IdleCallback
{
    using R = CompressorFloatingReference;
    using MR = Reference::Widgets::ThresholdMeter<accentColor<kFaustParameterThreshold>()>;

    static constexpr const std::array<FaustParameterIndex, 2> kMeters { kFaustParameterGr_1, kFaustParameterGr_2 };
    static constexpr const float kFloor = kFaustParameters[kFaustParameterGr_1].min;
    static constexpr const float ticks[] = { -3, -6, -10, -15 };

    std::array<float, 2> fValues {};

public:
    // unscaled, the input meter's two tracks and the gap between them, in the panel's inset
    static constexpr const uint kWidth = MR::Track::width * 2 + MR::padding + R::inset * 2;

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
                                               kFloor, 0.f); d_isNotEqual(fValues[i], value))
            {
                fValues[i] = value;
                repaint();
            }
        }
    }

    void onNanoDisplay() final
    {
        const float s = fScaleFactor;
        const float w = getWidth();
        const float h = getHeight();

        drawFloatingPanel(*this, w, h, s);

        const float inset = R::inset * s;
        const float mborder = MR::Meter::border * s;
        const float mr = MR::Meter::borderRadius * s;
        const float mx = inset;
        const float my = inset;
        const float mw = w - inset * 2;
        const float mh = h - inset * 2;

        const float tx = mx + mborder;
        const float ty = my + mborder;
        const float tw2 = mw - mborder * 2;
        const float tw1 = MR::Track::width * s - 1.5f * s;
        const float th = mh - mborder * 2;

        if (mh <= 0.f)
            return;

        const auto reductionToHeight = [&](const float db) {
            return d_roundToIntPositive(th * d_clamp(db / kFloor, 0.f, 1.f));
        };

        // background
        beginPath();
        roundedRect(mx, my, mw, mh, mr);
        fillColor(MR::Meter::backgroundColor);
        fill();

        // the tracks, left and right, from the top down
        const std::array<float, 2> trackX { tx, mx + mw - mborder - tw1 };

        for (size_t i = 0; i < kMeters.size(); ++i)
        {
            const float vh = reductionToHeight(fValues[i]);
            if (vh <= 0.f)
                continue;

            beginPath();
            roundedRect(tx, ty, tw2, th, mr);
            fillPaint(linearGradient(0, ty + vh, 0, ty, MR::Track::colorGradientStart, MR::Track::colorGradientStop));

            save();
            scissor(trackX[i], ty, tw1, vh);
            fill();
            restore();
        }

        // inset shading
        beginPath();
        roundedRect(mx, my, mw, mh, mr);
        fillPaint(linearGradient(0, my, 0, my + 4 * s, MR::Meter::insetShadingColor, Reference::Colors::transparent));
        fill();

        // ticks
        strokeColor(MR::Tick::color);
        strokeWidth(MR::Tick::height * s);

        for (const float tick : ticks)
        {
            const float lh = reductionToHeight(tick);

            beginPath();
            moveTo(tx, ty + lh);
            lineTo(tx + tw2, ty + lh);
            stroke();
        }
    }
};

// --------------------------------------------------------------------------------------------------------------------
// The compression controls next to the input meter: threshold, ratio, range and knee, stacked in a floating panel.

struct CompressorControlStackReference : CompressorFloatingReference {
    static constexpr const uint margin = inset;
    static constexpr const uint padding = CompressorGroupReference::padding;
    static constexpr const uint width = 110;
    static constexpr const uint height = margin * 2 + kBoxHeight * 4 + padding * 3;
};

class CompressorControlStackWidget final : public ReferenceContainerWidget<CompressorControlStackReference, kVertical>
{
    using R = CompressorControlStackReference;
    using BaseWidget = ReferenceContainerWidget<R, kVertical>;

    const std::array<std::shared_ptr<LabWidget>, 4> widgets {
        addWidget<CompressorNumberBoxWidget<kFaustParameterThreshold>>(kFaustParameterThreshold),
        addWidget<CompressorNumberBoxWidget<kFaustParameterRatio>>(kFaustParameterRatio),
        addWidget<CompressorNumberBoxWidget<kFaustParameterRange>>(kFaustParameterRange),
        addWidget<CompressorNumberBoxWidget<kFaustParameterKnee>>(kFaustParameterKnee),
    };

public:
    // unscaled
    static constexpr const uint kWidth = R::width;
    static constexpr const uint kHeight = R::height;

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

// --------------------------------------------------------------------------------------------------------------------
// The compression curve: output level against input level, both from -60 to 0 dB, for a steady signal, worked out with
// the gain computer and the Range ceiling of compressor.dsp. Makeup is left out, so 1:1 is the diagonal. The second
// channel's curve shows faintly when the Scale controls set the two channels apart. It floats over the starfield.

struct CompressorCurveReference : CompressorFloatingReference {
    static constexpr const float dbMin = -60.f;
    static constexpr const float gridStep = 12.f;
    static constexpr const Color gridColor { 1.f, 1.f, 1.f, 0.06f };
    static constexpr const Color unityColor { 1.f, 1.f, 1.f, 0.14f };
    static constexpr const Color thresholdColor { 1.f, 1.f, 1.f, 0.2f };
    static constexpr const Color curveColor = Reference::Colors::ink;
};

class CompressorCurveWidget final : public LabWidget,
                                    private IdleCallback
{
    using R = CompressorCurveReference;

    static constexpr const std::array<FaustParameterIndex, 6> kParameters {
        kFaustParameterThreshold,
        kFaustParameterRatio,
        kFaustParameterKnee,
        kFaustParameterRange,
        kFaustParameterScale_lm,
        kFaustParameterScale_rs,
    };

    std::array<float, kParameters.size()> fValues {};

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
    // gain reduction in dB for an input level, scaled for a channel, then held above the Range ceiling
    [[nodiscard]] float gainReduction(const float level, const float scale) const noexcept
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

        gr *= scale;

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

        if (changed)
            repaint();
    }

    void onNanoDisplay() final
    {
        const float s = fScaleFactor;
        const float w = getWidth();
        const float h = getHeight();

        drawFloatingPanel(*this, w, h, s);

        // the plot
        const float inset = R::inset * s;
        const float px = inset;
        const float py = inset;
        const float pw = w - inset * 2;
        const float ph = h - inset * 2;

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

        // grid every 12 dB, across and down
        beginPath();
        for (float db = R::dbMin; db <= 0.f; db += R::gridStep)
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

        // 1:1
        beginPath();
        moveTo(dbToX(R::dbMin), dbToY(R::dbMin));
        lineTo(dbToX(0.f), dbToY(0.f));
        strokeColor(R::unityColor);
        strokeWidth(1.f);
        stroke();

        // the threshold, up to where it meets the diagonal and across to the left
        {
            const float tx = std::round(dbToX(fValues[0])) + 0.5f;
            const float ty = std::round(dbToY(fValues[0])) + 0.5f;

            beginPath();
            moveTo(tx, py + ph);
            lineTo(tx, ty);
            lineTo(px, ty);
            strokeColor(R::thresholdColor);
            strokeWidth(1.f);
            stroke();
        }

        // a curve a pixel at a time
        const auto curve = [&](const float scale) {
            beginPath();
            for (float x = 0.f; x <= pw; x += 1.f)
            {
                const float in = R::dbMin + x / pw * -R::dbMin;
                const float y = dbToY(in + gainReduction(in, scale));
                if (x == 0.f)
                    moveTo(px + x, y);
                else
                    lineTo(px + x, y);
            }
        };

        const float scale1 = fValues[4] / 100.f;
        const float scale2 = fValues[5] / 100.f;

        if (d_isNotEqual(scale1, scale2))
        {
            curve(scale2);
            strokeColor(Color(R::curveColor, 0.35f));
            strokeWidth(1.f * s);
            stroke();
        }

        // filled down to the bottom, then the line with a glow
        curve(scale1);
        lineTo(px + pw, py + ph);
        lineTo(px, py + ph);
        closePath();
        fillPaint(linearGradient(0, py, 0, py + ph, Color(R::curveColor, 0.12f), Color(R::curveColor, 0.02f)));
        fill();

        curve(scale1);
        lineJoin(ROUND);
        strokeColor(Color(R::curveColor, 0.2f));
        strokeWidth(4.f * s);
        stroke();
        strokeColor(R::curveColor);
        strokeWidth(1.5f * s);
        stroke();

        restore();
    }
};

// --------------------------------------------------------------------------------------------------------------------
// The display: the starfield fills it, inside a well border, with the compression curve floating in the middle: square,
// as tall as the display less a margin above and below, and centred across it. The input meter floats as tall at its
// left, the same margin in from the edge, and next to the meter the compression controls, from the same top. The gain
// reduction meter floats as tall at the right, the same margin in.

class CompressorDisplayWidget final : public LabWidget
{
    using R = Reference::Stage;

    // between the floating panels and the display's edges
    static constexpr const uint kPanelMargin = 16;
    // between the floating panels side by side
    static constexpr const uint kPanelGap = 8;

    const std::unique_ptr<CompressorInputMeterWidget> fInputMeter { new CompressorInputMeterWidget(this) };
    const std::unique_ptr<CompressorControlStackWidget> fControls { new CompressorControlStackWidget(this) };
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
        const int side = std::max(1, std::min(static_cast<int>(getHeight()), static_cast<int>(getWidth())) - margin * 2);

        fCurve->setSize(side, side);
        fCurve->setAbsolutePos(getAbsoluteX() + (static_cast<int>(getWidth()) - side) / 2, getAbsoluteY() + margin);

        fInputMeter->setSize(d_roundToUnsignedInt(CompressorInputMeterWidget::kWidth * fScaleFactor), side);
        fInputMeter->setAbsolutePos(getAbsoluteX() + margin, getAbsoluteY() + margin);

        const int grWidth = d_roundToIntPositive(CompressorGainReductionMeterWidget::kWidth * fScaleFactor);
        fGainReduction->setSize(grWidth, side);
        fGainReduction->setAbsolutePos(getAbsoluteX() + static_cast<int>(getWidth()) - margin - grWidth,
                                       getAbsoluteY() + margin);

        // as a widget, its layout having a setSize and setAbsolutePos of its own
        LabWidget& controls = *fControls;
        controls.setSize(d_roundToUnsignedInt(CompressorControlStackWidget::kWidth * fScaleFactor),
                         d_roundToUnsignedInt(CompressorControlStackWidget::kHeight * fScaleFactor));
        controls.setAbsolutePos(fInputMeter->getAbsoluteX() + static_cast<int>(fInputMeter->getWidth())
                                    + d_roundToIntPositive(kPanelGap * fScaleFactor),
                                getAbsoluteY() + margin);

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
