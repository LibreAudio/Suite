// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "LibreAudioBaseUI.hpp"

#include "ui/containers/main-area.hpp"
#include "ui/containers/top-bar.hpp"
#include "ui/widgets/number-box.hpp"
#include "ui/widgets/switch-box.hpp"
#include "ui/widgets/shader.hpp"

#include "LibreAudioParameters.hpp"

#include "las-resources.h"

#include <algorithm>
#include <array>
#include <cstdio>

// --------------------------------------------------------------------------------------------------------------------

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------
// The stage holds the display, with the starfield behind it, and under it a row of control groups.
// Each group is a raised glass panel styled like the eq's option bar.

// the controls of a group, under its header; the labelled group around them draws the panel
struct VoiceIsolationGroupReference : Reference::Zero {
    static constexpr const uint borderRadius = 10;
    static constexpr const uint margin = 8;
    static constexpr const uint padding = 6;
    static constexpr const uint height = Reference::Widgets::NumberBox<>::height + margin * 2;
};

// accent colour of a parameter, as set by [accentcolor] in voiceIsolation.dsp

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
using VoiceIsolationNumberBoxWidget = NumberBoxWidget<accentColor<id>()>;

template<FaustParameterIndex id>
using VoiceIsolationSwitchBoxWidget = SwitchBoxWidget<accentColor<id>()>;

// --------------------------------------------------------------------------------------------------------------------
// One group's controls: number boxes, which share whatever width the row gives the group, and controls of their own
// width.

class VoiceIsolationGroupWidget : public ReferenceContainerWidget<VoiceIsolationGroupReference>
{
    using R = VoiceIsolationGroupReference;
    using BaseWidget = ReferenceContainerWidget<R>;

    const uint fBoxCount;

public:
    VoiceIsolationGroupWidget(LabWidget* const parent, const uint boxCount)
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
// RNNoise's voice activity as a VU style needle, in a box drawn like a number box. The pivot sits below the box, so
// the needle swings across the arc's upper part. The arc from the gate's Threshold up is drawn in the gate's colour,
// so it shows where the needle opens the gate.

struct VoiceIsolationVadMeterReference : Reference::Zero {
    using Box = Reference::Widgets::NumberBox<>;
    static constexpr const Color scaleColor = Reference::Colors::ink3;
    static constexpr const Color needleColor = Reference::Colors::ink;
    // pivot below the box's bottom edge, in box heights
    static constexpr const float pivotDrop = 0.9f;
    // room left and right of the arc, and above it
    static constexpr const float margin = 10.f;
    static constexpr const float top = 9.f;
    // widest swing either side of vertical, in degrees
    static constexpr const float maxAngle = 50.f;
    // time constant the needle follows the meter with, in seconds
    static constexpr const double timeConstant = 0.1;
};

class VoiceIsolationVadMeterWidget final : public LabWidget,
                                           private IdleCallback
{
    using R = VoiceIsolationVadMeterReference;

    // as drawn, in %
    float fValue = 0.f;
    float fThreshold = kFaustParameters[kFaustParameterThreshold].init;
    double fLastTime = 0;

public:
    explicit VoiceIsolationVadMeterWidget(LabWidget* const parent)
        : LabWidget(parent)
    {
        updateSize(false);
        addIdleCallback(this);
    }

private:
    void idleCallback() final
    {
        const double now = getApp().getTime();
        const double dt = fLastTime != 0 ? std::min(now - fLastTime, 0.1) : 0;
        fLastTime = now;

        const float target = fInterface->getParameterValue(kParametersMainStart + kFaustParameterVad);
        const float threshold = fInterface->getParameterValue(kParametersMainStart + kFaustParameterThreshold);
        bool changed = false;

        if (std::abs(target - fValue) < 0.05f)
        {
            if (d_isNotEqual(target, fValue))
            {
                fValue = target;
                changed = true;
            }
        }
        else
        {
            fValue += (target - fValue) * static_cast<float>(1.0 - std::exp(-dt / R::timeConstant));
            changed = true;
        }

        if (d_isNotEqual(threshold, fThreshold))
        {
            fThreshold = threshold;
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

        drawReferenceBackground<R::Box>();

        // inset shading and border, as a number box draws them
        beginPath();
        roundedRect(0, 0, w, h, R::Box::borderRadius * s);
        fillPaint(linearGradient(0, 0, 0, R::Box::margin * s,
                                 R::Box::insetShadingColor, Reference::Colors::transparent));
        fill();
        strokeColor(R::Box::backgroundColor);
        strokeWidth(R::Box::border * s * 0.5f);
        stroke();

        // the arc: as wide as the box allows, up to maxAngle either side
        const float cx = w * 0.5f;
        const float cy = h * (1.f + R::pivotDrop);
        const float radius = cy - R::top * s;
        const float halfWidth = std::max(1.f, cx - R::margin * s);
        const float maxAngle = std::min(R::maxAngle * static_cast<float>(M_PI) / 180.f,
                                        std::asin(std::min(1.f, halfWidth / radius)));

        // 0 % at the left end, 100 % at the right, as an angle from vertical
        const auto angleOf = [=](const float percent) {
            return -maxAngle + std::clamp(percent, 0.f, 100.f) / 100.f * 2.f * maxAngle;
        };
        const auto pointAt = [=](const float angle, const float r) {
            return Point<float>(cx + std::sin(angle) * r, cy - std::cos(angle) * r);
        };

        save();
        intersectScissor(0, 0, w, h);

        // scale: grey below the threshold, the gate's colour from it up
        const float thresholdAngle = angleOf(fThreshold);
        const auto arc = [&](const float from, const float to, const Color& color) {
            if (to - from < 0.001f)
                return;
            beginPath();
            this->arc(cx, cy, radius, from - static_cast<float>(M_PI) * 0.5f, to - static_cast<float>(M_PI) * 0.5f, CW);
            strokeColor(color);
            strokeWidth(1.2f * s);
            stroke();
        };
        arc(-maxAngle, thresholdAngle, R::scaleColor);
        arc(thresholdAngle, maxAngle, accentColor<kFaustParameterThreshold>());

        // ticks every 25 %, and the threshold's tick in the gate's colour
        for (int percent = 0; percent <= 100; percent += 25)
        {
            const float angle = angleOf(percent);
            const Point<float> inner = pointAt(angle, radius - 4.f * s);
            const Point<float> outer = pointAt(angle, radius);
            beginPath();
            moveTo(inner.getX(), inner.getY());
            lineTo(outer.getX(), outer.getY());
            strokeColor(angle >= thresholdAngle ? accentColor<kFaustParameterThreshold>() : R::scaleColor);
            strokeWidth(1.f * s);
            stroke();
        }
        {
            const Point<float> inner = pointAt(thresholdAngle, radius - 6.f * s);
            const Point<float> outer = pointAt(thresholdAngle, radius + 2.f * s);
            beginPath();
            moveTo(inner.getX(), inner.getY());
            lineTo(outer.getX(), outer.getY());
            strokeColor(accentColor<kFaustParameterThreshold>());
            strokeWidth(1.5f * s);
            stroke();
        }

        // the needle, from the pivot below the box to just past the arc
        {
            const Point<float> tip = pointAt(angleOf(fValue), radius + 3.f * s);
            beginPath();
            moveTo(cx, cy);
            lineTo(tip.getX(), tip.getY());
            lineCap(ROUND);
            strokeColor(R::needleColor);
            strokeWidth(1.5f * s);
            stroke();
        }

        restore();
    }

    // as tall as a number box, as wide as the group makes it
    void updateSize(const bool updateChildren) final
    {
        setSize(std::max(1u, getWidth()), d_roundToUnsignedInt(R::Box::height * fScaleFactor));
        LabWidget::updateSize(updateChildren);
    }
};

class VoiceIsolationVoiceActivityGroupWidget final : public VoiceIsolationGroupWidget
{
    const std::array<std::shared_ptr<LabWidget>, 1> widgets {
        addWidget<VoiceIsolationVadMeterWidget, Expanding>(),
    };

public:
    explicit VoiceIsolationVoiceActivityGroupWidget(LabWidget* const parent)
        : VoiceIsolationGroupWidget(parent, 1) {}
};

class VoiceIsolationDenoiseGroupWidget final : public VoiceIsolationGroupWidget
{
    const std::array<std::shared_ptr<LabWidget>, 3> widgets {
        addWidget<VoiceIsolationNumberBoxWidget<kFaustParameterMax_reduction>, Expanding>(kFaustParameterMax_reduction),
        addWidget<VoiceIsolationNumberBoxWidget<kFaustParameterRelease>, Expanding>(kFaustParameterRelease),
        addWidget<VoiceIsolationSwitchBoxWidget<kFaustParameterPitch_enhancement>, Expanding>(kFaustParameterPitch_enhancement,
                                                                                         "Pitch Enhance"),
    };

public:
    explicit VoiceIsolationDenoiseGroupWidget(LabWidget* const parent)
        : VoiceIsolationGroupWidget(parent, 3) {}
};

class VoiceIsolationGateGroupWidget final : public VoiceIsolationGroupWidget
{
    const std::array<std::shared_ptr<LabWidget>, 2> widgets {
        addWidget<VoiceIsolationNumberBoxWidget<kFaustParameterThreshold>, Expanding>(kFaustParameterThreshold),
        addWidget<VoiceIsolationNumberBoxWidget<kFaustParameterGrace_period>, Expanding>(kFaustParameterGrace_period),
    };

public:
    explicit VoiceIsolationGateGroupWidget(LabWidget* const parent)
        : VoiceIsolationGroupWidget(parent, 2) {}
};

// The common Dry / Wet, which LibreAudioPlugin mixes with the latency-aligned input, described for a number box.

static constexpr const FaustParameter kDryWetParameter = {
    .bracket = "",
    .label = "Dry-Wet",
    .shortlabel = "",
    .name = "Dry / Wet",
    .symbol = "dry_wet",
    .tooltip = "",
    .unit = "%",
    .init = LIBREAUDIO_DRYWET_DEFAULT,
    .min = 0.f,
    .max = 100.f,
    .step = 1.f,
    .accentcolor = 1,
    .scalePoints = nullptr,
    .scalePointCount = 0,
    .requirement = {
        .context = nullptr,
        .symbol = nullptr,
        .value = -1,
    },
    .isBoolean = false,
    .isEasy = false,
    .isEnumerator = false,
    .isDynamic = false,
    .isInteger = false,
    .isLogarithmic = false,
    .isOutput = false,
    .isTrigger = false,
};

class VoiceIsolationMixGroupWidget final : public VoiceIsolationGroupWidget
{
    const std::array<std::shared_ptr<LabWidget>, 1> widgets {
        addWidget<NumberBoxWidget<Reference::Colors::acc1>, Expanding>(kCommonParameterDryWet, &kDryWetParameter),
    };

public:
    explicit VoiceIsolationMixGroupWidget(LabWidget* const parent)
        : VoiceIsolationGroupWidget(parent, 1) {}
};

// --------------------------------------------------------------------------------------------------------------------
// A group's header, styled like the reverb's section headers: its name in the group's accent colour, at the panel's
// top left, in a 20 px row 4 px above the controls.

struct VoiceIsolationGroupHeaderReference : Reference::Zero {
    static constexpr const float fontSize = 12.f;
    static constexpr const float letterSpacing = 0.4f;
    static constexpr const uint row = 20;
    static constexpr const uint gap = 4;
    // the panel's top margin, then the row and the gap; the controls' own top margin follows
    static constexpr const uint height = VoiceIsolationGroupReference::margin + row + gap
                                       - VoiceIsolationGroupReference::margin;
};

class VoiceIsolationGroupHeaderWidget final : public LabWidget
{
    using R = VoiceIsolationGroupHeaderReference;
    using G = VoiceIsolationGroupReference;

    const char* const fLabel;
    const Color fColor;

public:
    VoiceIsolationGroupHeaderWidget(LabWidget* const parent, const char* const label, const Color color)
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

// A group's panel: its header over its controls. The row sizes these, and they pass the sizes on to their controls.

struct VoiceIsolationLabeledGroupReference : Reference::Zero {
    static constexpr const uint height = VoiceIsolationGroupHeaderReference::height
                                       + VoiceIsolationGroupReference::height;
};

class VoiceIsolationLabeledGroupWidget : public ReferenceContainerWidget<VoiceIsolationLabeledGroupReference, kVertical>
{
    using BaseWidget = ReferenceContainerWidget<VoiceIsolationLabeledGroupReference, kVertical>;

    // addWidget's layout only points at its widgets, these own them
    const std::shared_ptr<VoiceIsolationGroupHeaderWidget> fHeader;

protected:
    std::shared_ptr<VoiceIsolationGroupWidget> fPanel;

public:
    VoiceIsolationLabeledGroupWidget(LabWidget* const parent, const char* const label, const Color color)
        : BaseWidget(parent),
          fHeader(addWidget<VoiceIsolationGroupHeaderWidget>(label, color)) {}

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

private:
    // the eq option bar's raised panel: a light tint, lit along the top edge, shaded along the bottom, with a faint
    // rim all around; the stage draws its shadow, which reaches outside the panel
    void onNanoDisplay() final
    {
        const float w = getWidth();
        const float h = getHeight();
        const float r = VoiceIsolationGroupReference::borderRadius * fScaleFactor;

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

template<class Panel>
class VoiceIsolationLabeledGroupOf final : public VoiceIsolationLabeledGroupWidget
{
public:
    VoiceIsolationLabeledGroupOf(LabWidget* const parent, const char* const label, const Color color)
        : VoiceIsolationLabeledGroupWidget(parent, label, color)
    {
        fPanel = addWidget<Panel>();
    }
};

// --------------------------------------------------------------------------------------------------------------------
// The row of groups along the bottom of the display, a fixed gap apart. Every number box gets the same width, from
// what the row has left once the groups' fixed parts and the gaps are taken out.

struct VoiceIsolationGroupRowReference : Reference::Zero {
    static constexpr const uint padding = 8;
    static constexpr const uint height = VoiceIsolationLabeledGroupReference::height;
};

class VoiceIsolationGroupRowWidget final : public ReferenceContainerWidget<VoiceIsolationGroupRowReference>
{
    using R = VoiceIsolationGroupRowReference;
    using BaseWidget = ReferenceContainerWidget<R>;

    const std::array<std::shared_ptr<VoiceIsolationLabeledGroupWidget>, 4> fGroups {
        addWidget<VoiceIsolationLabeledGroupOf<VoiceIsolationDenoiseGroupWidget>>(
            "DENOISER", accentColor<kFaustParameterMax_reduction>()),
        addWidget<VoiceIsolationLabeledGroupOf<VoiceIsolationVoiceActivityGroupWidget>>(
            "VOICE ACTIVITY", Reference::Colors::acc2),
        addWidget<VoiceIsolationLabeledGroupOf<VoiceIsolationGateGroupWidget>>(
            "GATE", accentColor<kFaustParameterThreshold>()),
        addWidget<VoiceIsolationLabeledGroupOf<VoiceIsolationMixGroupWidget>>(
            "MIXER", Reference::Colors::acc1),
    };

public:
    explicit VoiceIsolationGroupRowWidget(LabWidget* const parent)
        : BaseWidget(parent) {}

    [[nodiscard]] const std::array<std::shared_ptr<VoiceIsolationLabeledGroupWidget>, 4>& getGroups() const noexcept
    {
        return fGroups;
    }

private:
    void sizeGroups()
    {
        const int gap = d_roundToIntPositive(R::padding * fScaleFactor);
        int available = static_cast<int>(getWidth()) - gap * static_cast<int>(fGroups.size() - 1);
        uint boxes = 0;

        for (const std::shared_ptr<VoiceIsolationLabeledGroupWidget>& group : fGroups)
        {
            available -= static_cast<int>(group->getFixedWidth());
            boxes += group->getBoxCount();
        }

        const uint boxWidth = boxes != 0 ? static_cast<uint>(std::max(0, available) / static_cast<int>(boxes)) : 0;

        for (const std::shared_ptr<VoiceIsolationLabeledGroupWidget>& group : fGroups)
            group->setGroupWidth(group->getFixedWidth() + group->getBoxCount() * boxWidth);
    }

    // the controls of fixed width know their size only once updated, so the groups are sized again after that
    void updateSize(const bool updateChildren) final
    {
        sizeGroups();
        BaseWidget::updateSize(updateChildren);
        sizeGroups();
        BaseWidget::updateSize(updateChildren);
    }
};

// --------------------------------------------------------------------------------------------------------------------

// RNNoise's band layout (eband20ms in deps/rnnoise/src/denoise.c), in 50 Hz steps: the gain of band i applies at
// edge i + 1.

static constexpr const uint kNumBands = kFaustParameterBand_32 - kFaustParameterBand_1 + 1;
static constexpr const float kBandEdgeHz = 50.f;
static constexpr const std::array<int, kNumBands + 2> kBandEdges {{
    0, 2, 4, 6, 8, 10, 12, 15, 18, 21, 24, 28, 32, 36, 41, 47, 53,
    60, 68, 77, 87, 98, 110, 124, 140, 157, 176, 198, 223, 251, 282, 317, 356, 400,
}};

// the display's gain reduction scale: dB a margin below its top and a margin above its bottom; deeper reduction runs
// out through the bottom
static constexpr const float kDisplayDbMax = 0.f;
static constexpr const float kDisplayDbMin = -48.f;

// hairline grid across the scale, labelled on both sides
static constexpr const float kDisplayDbStep = 6.f;

// how quickly the drawn curve follows the band meters, in seconds
static constexpr const double kCurveTimeConstant = 0.06;

// mbComp5 rainbow
static constexpr const std::array<Color, 5> kRainbow {{
    { 255, 191, 203 }, { 255, 223, 173 }, { 210, 253, 211 }, { 190, 241, 255 }, { 218, 193, 243 },
}};

static Color rainbow(const float t, const float alpha) noexcept
{
    const float p = std::clamp(t, 0.f, 1.f) * (kRainbow.size() - 1);
    const size_t i = std::min(kRainbow.size() - 2, static_cast<size_t>(p));
    return Color(kRainbow[i], kRainbow[i + 1], p - i).withAlpha(alpha);
}

// --------------------------------------------------------------------------------------------------------------------
// The display: the starfield fills it, and it draws the denoiser's gain reduction over the frequency range as
// mbComp5's rainbow curve, eased over time and joined smoothly between the bands, inside a well border.

class VoiceIsolationDisplayWidget final : public LabWidget,
                                          private IdleCallback
{
    using R = Reference::Stage;

    // points along the curve
    static constexpr const int kSteps = 256;

    // band reduction as drawn, easing towards the band meters, in dB
    std::array<float, kNumBands> fBandReduction {};
    double fLastTime = 0;

    // Max Reduction as drawn, in dB
    float fMaxReduction = kFaustParameters[kFaustParameterMax_reduction].init;

public:
    explicit VoiceIsolationDisplayWidget(LabWidget* const parent)
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
        // ease the drawn reduction towards the meters, 0 dB while bypassed
        const double now = getApp().getTime();
        const double dt = fLastTime != 0 ? std::min(now - fLastTime, 0.1) : 0;
        const float coef = static_cast<float>(1.0 - std::exp(-dt / kCurveTimeConstant));
        const bool off = bypassed();
        bool changed = false;

        fLastTime = now;

        for (uint i = 0; i < kNumBands; ++i)
        {
            const float target = off ? 0.f
                                     : fInterface->getParameterValue(kParametersMainStart + kFaustParameterBand_1 + i);

            if (std::abs(target - fBandReduction[i]) < 0.01f)
            {
                if (d_isNotEqual(target, fBandReduction[i]))
                {
                    fBandReduction[i] = target;
                    changed = true;
                }
                continue;
            }

            fBandReduction[i] += (target - fBandReduction[i]) * coef;
            changed = true;
        }

        const float maxReduction = fInterface->getParameterValue(kParametersMainStart + kFaustParameterMax_reduction);

        if (d_isNotEqual(maxReduction, fMaxReduction))
        {
            fMaxReduction = maxReduction;
            changed = true;
        }

        if (changed)
            repaint();
    }

    [[nodiscard]] bool bypassed() const noexcept
    {
        return fInterface->getParameterValue(kCommonParameterBypass) > 0.5f;
    }

    // Reduction in dB at each normalized frequency of xs: a monotone cubic (Fritsch-Carlson) through the bands in dB
    // over log frequency, held flat outside them. Being monotone, it never overshoots the bands, so never rises above
    // 0 dB.
    template<size_t N>
    void reductionAt(const std::array<float, N>& xs, std::array<float, N>& ys) const noexcept
    {
        std::array<float, kNumBands> px, py, m;
        std::array<float, kNumBands - 1> d;

        for (uint i = 0; i < kNumBands; ++i)
        {
            px[i] = freq2norm(kBandEdges[i + 1] * kBandEdgeHz);
            py[i] = fBandReduction[i];
        }

        for (uint i = 0; i < kNumBands - 1; ++i)
            d[i] = (py[i + 1] - py[i]) / (px[i + 1] - px[i]);

        m[0] = d[0];
        m[kNumBands - 1] = d[kNumBands - 2];
        for (uint i = 1; i < kNumBands - 1; ++i)
            m[i] = d[i - 1] * d[i] <= 0.f ? 0.f : (d[i - 1] + d[i]) * 0.5f;

        for (uint i = 0; i < kNumBands - 1; ++i)
        {
            if (d_isZero(d[i]))
            {
                m[i] = m[i + 1] = 0.f;
                continue;
            }

            const float a = m[i] / d[i];
            const float b = m[i + 1] / d[i];

            if (const float h = a * a + b * b; h > 9.f)
            {
                const float t = 3.f / std::sqrt(h);
                m[i] = t * a * d[i];
                m[i + 1] = t * b * d[i];
            }
        }

        uint k = 0;
        for (size_t j = 0; j < N; ++j)
        {
            const float x = xs[j];

            if (x <= px[0])
            {
                ys[j] = py[0];
                continue;
            }
            if (x >= px[kNumBands - 1])
            {
                ys[j] = py[kNumBands - 1];
                continue;
            }

            while (x > px[k + 1])
                ++k;

            const float h = px[k + 1] - px[k];
            const float t = (x - px[k]) / h;
            const float t2 = t * t;
            const float t3 = t2 * t;

            ys[j] = (2.f * t3 - 3.f * t2 + 1.f) * py[k] + (t3 - 2.f * t2 + t) * h * m[k]
                  + (-2.f * t3 + 3.f * t2) * py[k + 1] + (t3 - t2) * h * m[k + 1];
        }
    }

    void onNanoDisplay() final
    {
        const float s = fScaleFactor;
        const float w = getWidth();
        const float h = getHeight();
        const bool off = bypassed();

        // the scale starts half a label below the margin, leaving the frequency labels above it, as in mbComp5
        const float fontsize = 10.f * s;
        const float top = R::margin * s + fontsize * 0.5f;
        const float bottom = h - R::margin * s;

        const auto yOfDb = [=](const float db) {
            return top + (kDisplayDbMax - db) / (kDisplayDbMax - kDisplayDbMin) * (bottom - top);
        };

        std::array<float, kSteps + 1> xs, ys;
        for (int i = 0; i <= kSteps; ++i)
            xs[i] = static_cast<float>(i) / kSteps;

        reductionAt(xs, ys);

        for (float& y : ys)
            y = yOfDb(y);

        const auto path = [&]() {
            beginPath();
            moveTo(0, ys[0]);
            for (int i = 1; i <= kSteps; ++i)
                lineTo(i * w / kSteps, ys[i]);
        };

        // NanoVG draws every widget's paths at the end of the frame, after DPF has lifted its per-widget clip, so the
        // curve clips itself to the display
        save();
        scissor(0, 0, w, h);

        fontFace("regular");
        fontSize(fontsize);
        textLetterSpacing(0.f);
        strokeWidth(1.f);

        // mbComp5's frequency grid: 9 lines per decade, each decade's first stronger and its fifth less so, labelled
        // above the scale at 1, 2 and 5 of each decade from 50 Hz
        {
            const Color color(0x3b, 0x3b, 0x44);
            char label[16];

            textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
            fillColor(Color(140, 142, 150));

            for (int dec = 10; dec < kFreqMax; dec *= 10)
            {
                for (int m = 0; m < 9; ++m)
                {
                    const int freq = (m + 1) * dec;
                    if (freq <= kFreqMin || freq >= kFreqMax)
                        continue;

                    const float x = std::round(freq2norm(freq) * w) + 0.5f;

                    beginPath();
                    moveTo(x, top);
                    lineTo(x, h);
                    strokeColor(color.withAlpha(m == 0 ? 0.7f : m == 4 ? 0.45f : 0.22f));
                    stroke();

                    if ((m == 0 || m == 1 || m == 4) && freq >= 50)
                    {
                        if (freq >= 1000)
                            std::snprintf(label, sizeof(label), "%dk", freq / 1000);
                        else
                            std::snprintf(label, sizeof(label), "%d", freq);

                        text(x, top * 0.5f, label, nullptr);
                    }
                }
            }
        }

        // hairline grid, 0 dB stronger, labelled on both sides on the lines themselves; each line stops short of its
        // labels and fades in from them
        {
            const float labelMargin = 10.f * s;
            const float labelGap = 6.f * s;
            const float fade = 40.f * s;
            char label[16];

            for (float db = kDisplayDbMax; db >= kDisplayDbMin; db -= kDisplayDbStep)
            {
                const float y = std::round(yOfDb(db)) + 0.5f;
                const Color color(0x64, 0x64, 0x70, d_isZero(db) ? 0.5f : 0.22f);

                std::snprintf(label, sizeof(label), "%d%s", static_cast<int>(db),
                              d_isEqual(db, kDisplayDbMin) ? " dB" : "");

                Rectangle<float> bounds;
                textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
                textBounds(0, 0, label, nullptr, bounds);

                const float x0 = labelMargin + bounds.getWidth() + labelGap;
                const float x1 = w - x0;

                if (x1 - x0 > fade * 2.f)
                {
                    const auto segment = [&](const float from, const float to, const Paint& paint) {
                        beginPath();
                        moveTo(from, y);
                        lineTo(to, y);
                        strokePaint(paint);
                        stroke();
                    };

                    segment(x0, x0 + fade, linearGradient(x0, y, x0 + fade, y, color.withAlpha(0.f), color));
                    segment(x0 + fade, x1 - fade, linearGradient(x0 + fade, y, x1 - fade, y, color, color));
                    segment(x1 - fade, x1, linearGradient(x1 - fade, y, x1, y, color, color.withAlpha(0.f)));
                }

                fillColor(Color(140, 142, 150));
                text(labelMargin, y, label, nullptr);
                textAlign(ALIGN_RIGHT | ALIGN_MIDDLE);
                text(w - labelMargin, y, label, nullptr);
            }
        }

        // Max Reduction, the floor the curve rests on: a dotted grey line, under the display's bottom while unlimited
        {
            const float y = yOfDb(-fMaxReduction);
            const float gap = 6.f * s;

            if (y < h)
            {
                beginPath();
                for (float x = gap * 0.5f; x < w; x += gap)
                    circle(x, y, 1.2f * s);
                fillColor(Color(Reference::Colors::ink3, 0.85f));
                fill();
            }
        }

        // fill between 0 dB and the curve, fading out to fully transparent at 0 dB, in the curve's colours
        const float zero = yOfDb(0.f);
        for (int i = 0; i < kSteps; ++i)
        {
            const float edge = std::max(ys[i], ys[i + 1]);
            if (edge - zero < 0.01f)
                continue;

            const float left = i * w / kSteps;
            const float right = (i + 1) * w / kSteps;

            beginPath();
            moveTo(left, zero);
            lineTo(left, ys[i]);
            lineTo(right, ys[i + 1]);
            lineTo(right, zero);
            closePath();
            fillPaint(linearGradient(0, zero, 0, edge, rainbow((left + right) / (2 * w), 0.f),
                                                       rainbow((left + right) / (2 * w), 0.24f)));
            fill();
        }

        // the curve in four sections, each a gradient between two rainbow colours, over a wide faint glow
        for (uint section = 0; section < kRainbow.size() - 1; ++section)
        {
            const float left = section * w / (kRainbow.size() - 1);
            const float right = (section + 1) * w / (kRainbow.size() - 1);
            const float opacity = off ? 0.18f : 0.7f;

            save();
            intersectScissor(left, 0, right - left, h);

            path();
            strokeColor(rainbow((section + 0.5f) / (kRainbow.size() - 1), off ? 0.025f : 0.08f));
            strokeWidth(4.f * s);
            stroke();

            path();
            strokePaint(linearGradient(left, 0, right, 0, Color(kRainbow[section], opacity),
                                                          Color(kRainbow[section + 1], opacity)));
            strokeWidth(1.2f * s);
            stroke();

            restore();
        }

        restore();

        // The scissor is square, so the display's rounded corners are covered again with the window background, the
        // same gradient over the window's height that LibreAudioBaseUI paints, and the same radius the starfield
        // stops at.
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
// The stage: the display on top, and under it the row of control groups in Expert mode or four preset cards in Easy
// mode; the cards are the suite's Easy page cards and do not load anything yet. On the other pages the display
// takes the whole stage.

class VoiceIsolationStageWidget final : public ReferenceContainerWidget<Reference::TransparentStage, kVertical>,
                                        private IdleCallback
{
    using R = Reference::TransparentStage;
    using BaseWidget = ReferenceContainerWidget<R, kVertical>;

    const std::shared_ptr<VoiceIsolationDisplayWidget> fDisplay = addWidget<VoiceIsolationDisplayWidget, Expanding>();
    const std::shared_ptr<VoiceIsolationGroupRowWidget> fGroups = addWidget<VoiceIsolationGroupRowWidget>();
    const std::shared_ptr<CardGroupWidget> fCards = addCards();

    Page fPage = kPageEasy;

public:
    explicit VoiceIsolationStageWidget(LabWidget* const parent)
        : BaseWidget(parent)
    {
        showPage(getCurrentPage(fInterface));
        addIdleCallback(this);
    }

    [[nodiscard]] const VoiceIsolationDisplayWidget& getDisplay() const noexcept
    {
        return *fDisplay;
    }

private:
    std::shared_ptr<CardGroupWidget> addCards()
    {
        std::shared_ptr<CardGroupWidget> widget { new CardGroupWidget(this) };

        widget->addCard(kWidgetPresetA, "NATURAL DOUBLE", "Subtle ADT thickening");
        widget->addCard(kWidgetPresetB, "TIGHT SLAP", "Short doubled slap");
        widget->addCard(kWidgetPresetC, "WIDE TRIPLE", "Three-voice spread");
        widget->addCard(kWidgetPresetD, "LUSH STACK", "Full ensemble stack");
        widget->done();

        Layout::widgets.push_back({ widget.get(), Fixed });
        return widget;
    }

    // the groups in Expert mode, the cards in Easy mode, neither on the other pages
    void showPage(const Page page)
    {
        fPage = page;
        fGroups->setVisible(page == kPageExpert);
        fCards->setVisible(page == kPageEasy);
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

        if (! fGroups->isVisible())
            return;

        const float s = fScaleFactor;
        const float r = VoiceIsolationGroupReference::borderRadius * s;

        for (const std::shared_ptr<VoiceIsolationLabeledGroupWidget>& group : fGroups->getGroups())
        {
            const float x = group->getAbsoluteX() - getAbsoluteX();
            const float y = group->getAbsoluteY() - getAbsoluteY();
            const float w = group->getWidth();
            const float h = group->getHeight();

            beginPath();
            rect(x - 20.f * s, y, w + 40.f * s, h + 24.f * s);
            roundedRect(x, y, w, h, r);
            pathWinding(CW);
            fillPaint(boxGradient(x + 6.f * s, y + 10.f * s, w - 12.f * s, h - 12.f * s, r, 14.f * s,
                                  Color(0.f, 0.f, 0.f, 0.6f), Color(0.f, 0.f, 0.f, 0.f)));
            fill();
        }
    }
};

// --------------------------------------------------------------------------------------------------------------------
// The starfield fills the display only, not the controls under it, so the main area reports the display's area.

class VoiceIsolationMainAreaWidget final : public MainAreaContainerWidget<VoiceIsolationStageWidget>
{
public:
    explicit VoiceIsolationMainAreaWidget(LabTopLevelWidget* const parent)
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

// The starfield sits on the display, and follows it as switching between Easy and Expert mode changes its height.

class VoiceIsolationRootWidget final : public RootBaseWidget,
                                       private IdleCallback
{
    std::shared_ptr<TopBar> fTopBar = addWidget<TopBar>();
    std::shared_ptr<VoiceIsolationMainAreaWidget> fMainArea = addWidget<VoiceIsolationMainAreaWidget, Expanding>();

    Point<int> fLastDisplayPos;
    Size<uint> fLastDisplaySize;

public:
    VoiceIsolationRootWidget(Window& window, LabUIWidgetInterface* const iface)
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

class VoiceIsolationUI final : public LibreAudioBaseUI
{
    using AnalyserShaderWidget = LibreAudio::BotShaderWidget<SHADERS_ANALYSER_FFT_FRAG_DATA,
                                                             SHADERS_ANALYSER_FFT_FRAG_LEN,
                                                             LibreAudioAnalyzerIPC::kNumBins>;

    const std::unique_ptr<LibreAudio::ShaderBaseWidget> fShaderBackground {
        new LibreAudio::BotShaderWidget<SHADERS_SHADERTOY_CLOUDSTARFIELD_FRAG_DATA,
                                        SHADERS_SHADERTOY_CLOUDSTARFIELD_FRAG_LEN>(this, this)
    };

    // output spectrum, above the starfield and below the gain reduction curve, as in mbComp5
    const std::unique_ptr<AnalyserShaderWidget> fShaderAnalyser {
        new AnalyserShaderWidget(this, this)
    };

public:
    VoiceIsolationUI()
        : LibreAudioBaseUI()
    {
        createRootWidget<LibreAudio::VoiceIsolationRootWidget>();

        fRootWidget->enableShaders({ fShaderBackground.get(), fShaderAnalyser.get() });
    }

private:
    void audioGraphReceived(const float values[LibreAudioAnalyzerIPC::kNumBins]) final
    {
        fShaderAnalyser->replace(values);
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoiceIsolationUI)
};

UI* createUI()
{
    return new VoiceIsolationUI();
}

END_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------
