// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "LibreAudioBaseUI.hpp"

#include "ui/containers/main-area.hpp"
#include "ui/containers/top-bar.hpp"
#include "ui/widgets/number-box.hpp"
#include "ui/widgets/power-box.hpp"
#include "ui/widgets/shader.hpp"

#include "LibreAudioParameters.hpp"

#include "las-resources.h"

#include <algorithm>
#include <array>
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
// square root of its duration plus a share of the whole, so a short attack stays readable beside a long release. A
// dashed line marks where the transient reaches the output with lookahead on, and with Auto Release up a faint second
// release shows it stretched by a sustained 6 dB of reduction. A pane of glass covers it all.

struct CompressorEnvelopeReference : Reference::Zero {
    static constexpr const uint height = 100;
    static constexpr const uint borderRadius = 6;
    static constexpr const float labelFontSize = 9.f;
    // empty space left and right of the sections, as a fraction of the width
    static constexpr const float pad = 0.05f;
    // the share of the sum of the sections' square roots each section gets on top of its own
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
    static constexpr const uint kHeight = R::height;

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
        const float attackCurve = fValues[2];
        const float release = std::max(1e-3f, fValues[4] * 0.001f);
        const float releaseCurve = fValues[5];
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

        fRelease = releaseRamp(release);

            // with a sustained kAutoRefDb of reduction, Auto Release stretches the release by 1 + autoRelease
        if (autoRelease > 0.f)
            fAutoRelease = releaseRamp(release * (1.f + autoRelease));
        else
            fAutoRelease.clear();

        fDurations = {
            fAttack.back().t,
            fValues[3] * 0.001f,
            std::max(fRelease.back().t, fAutoRelease.empty() ? 0.f : fAutoRelease.back().t),
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
        const float h = getHeight();
        const float r = R::borderRadius * s;

        if (w <= 0.f)
            return;

        const Color& accent = accentColor<kFaustParameterAttack>();

        // the well behind the glass, shaded in from the top
        beginPath();
        roundedRect(x, 0, w, h, r);
        fillColor(Color(0.f, 0.f, 0.f, 0.3f));
        fill();
        fillPaint(linearGradient(0, 0, 0, 10.f * s, Color(0.f, 0.f, 0.f, 0.3f), Color(0.f, 0.f, 0.f, 0.f)));
        fill();

        // the sections across the width
        const float pad = w * R::pad;
        std::array<float, 3> widths {};
        {
            float roots = 0.f;
            for (const float d : fDurations)
                roots += std::sqrt(d);

            float sum = 0.f;
            for (size_t i = 0; i < widths.size(); ++i)
                sum += widths[i] = fDurations[i] > 0.f ? std::sqrt(fDurations[i]) + R::minSection * roots : 0.f;

            for (float& width : widths)
                width *= sum > 0.f ? (w - pad * 2) / sum : 0.f;
        }

        // x of a time, in seconds from the start of the attack
        const auto timeToX = [&](float t) {
            float tx = x + pad;
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

        // section edges, with the sections' names under the curve where they fit
        {
            static constexpr const char* const names[] = { "ATTACK", "HOLD", "RELEASE" };

            fontFace("regular");
            fontSize(R::labelFontSize * s);
            textAlign(ALIGN_CENTER | ALIGN_BOTTOM);

            float sx = x + pad;
            for (size_t i = 0; i <= widths.size(); ++i)
            {
                beginPath();
                moveTo(std::round(sx) + 0.5f, 0);
                lineTo(std::round(sx) + 0.5f, h);
                strokeColor(Color(1.f, 1.f, 1.f, 0.07f));
                strokeWidth(1.f);
                stroke();

                if (i == widths.size())
                    break;

                Rectangle<float> bounds;
                if (widths[i] > 0.f && textBounds(0, 0, names[i], nullptr, bounds) + 6.f * s < widths[i])
                {
                    fillColor(Color(Reference::Colors::ink3, 0.8f));
                    text(sx + widths[i] * 0.5f, h - 4.f * s, names[i], nullptr);
                }

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

        // the stretched release, faint and behind
        if (! fAutoRelease.empty())
        {
            beginPath();
            moveTo(timeToX(releaseStart), dbToY(-kDepth));
            addRamp(fAutoRelease, releaseStart);
            strokeColor(Color(accent, 0.35f));
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

        curve();
        closePath();
        fillPaint(linearGradient(0, y0, 0, yDepth, Color(accent, 0.04f), Color(accent, 0.3f)));
        fill();

        curve();
        strokeColor(Color(accent, 0.25f));
        strokeWidth(4.f * s);
        lineJoin(ROUND);
        stroke();
        strokeColor(accent);
        strokeWidth(1.5f * s);
        stroke();

        // where the transient reaches the output, dashed
        if (fLookahead > 0.f)
        {
            const float lx = std::round(timeToX(fLookahead)) + 0.5f;
            const float dash = 3.f * s;

            beginPath();
            for (float dy = 0.f; dy < h; dy += dash * 2)
            {
                moveTo(lx, dy);
                lineTo(lx, std::min(h, dy + dash));
            }
            strokeColor(Color(Reference::Colors::ink2, 0.6f));
            strokeWidth(1.f);
            stroke();
        }

        restore();

        // the glass: a gloss over the top half, a faint diagonal sheen, and a rim lit at the top and dark at the bottom
        save();
        intersectScissor(x, 0, w, h * 0.5f);
        beginPath();
        roundedRect(x, 0, w, h, r);
        fillPaint(linearGradient(0, 0, 0, h * 0.5f, Color(1.f, 1.f, 1.f, 0.1f), Color(1.f, 1.f, 1.f, 0.02f)));
        fill();
        restore();

        save();
        intersectScissor(x, 0, w, h);
        beginPath();
        moveTo(x + w * 0.58f, 0);
        lineTo(x + w * 0.68f, 0);
        lineTo(x + w * 0.62f, h);
        lineTo(x + w * 0.52f, h);
        closePath();
        fillPaint(linearGradient(0, 0, 0, h, Color(1.f, 1.f, 1.f, 0.05f), Color(1.f, 1.f, 1.f, 0.01f)));
        fill();
        restore();

        beginPath();
        roundedRect(x + 0.5f, 0.5f, w - 1.f, h - 1.f, r - 0.5f);
        strokePaint(linearGradient(0, 0, 0, h, Color(1.f, 1.f, 1.f, 0.22f), Color(0.f, 0.f, 0.f, 0.35f)));
        strokeWidth(1.f);
        stroke();

        beginPath();
        moveTo(x + r, 1.5f);
        lineTo(x + w - r, 1.5f);
        strokePaint(linearGradient(x, 0, x + w, 0, Color(1.f, 1.f, 1.f, 0.05f), Color(1.f, 1.f, 1.f, 0.3f)));
        strokeWidth(1.f);
        stroke();
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
// The display: the starfield fills it, inside a well border.

class CompressorDisplayWidget final : public LabWidget
{
    using R = Reference::Stage;

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

        // well border, over everything
        const float border = std::max(1, d_roundToIntPositive(R::border * s));
        const float half = border * 0.5f;

        beginPath();
        roundedRect(half, half, w - border, h - border, R::borderRadius * s - half);
        strokeColor(R::borderColor);
        strokeWidth(border);
        stroke();
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
