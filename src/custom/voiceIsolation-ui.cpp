// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "LibreAudioBaseUI.hpp"

#include "ui/containers/main-area.hpp"
#include "ui/containers/top-bar.hpp"
#include "ui/widgets/number-box.hpp"
#include "ui/widgets/toggle-switch.hpp"
#include "ui/widgets/shader.hpp"

#include "LibreAudioParameters.hpp"

#include "las-resources.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <utility>

// --------------------------------------------------------------------------------------------------------------------

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------
// The stage holds the display, with the starfield behind it, and under it a row of control groups.
// Each group is a raised glass panel styled like the eq's option bar.

struct VoiceIsolationGroupReference : Reference::Zero {
    static constexpr const Color backgroundColor { 0xff, 0xff, 0xff, 0.05f };
    static constexpr const uint borderRadius = 10;
    static constexpr const uint margin = 11;
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

// An on / off parameter shown as its name only, two words on two lines, as tall as a number box: grey when off,
// accent colour when on, 70% accent when hovered.

struct VoiceIsolationToggleReference : Reference::Zero {
    static constexpr const Color color = Reference::Colors::ink3;
    static constexpr const float fontSize = 13.f;
    static constexpr const float letterSpacing = fontSize * 0.06f;
    static constexpr const uint margin = 6;
    static constexpr const uint height = Reference::Widgets::NumberBox<>::height;
    // distance between the two lines of a two-word name
    static constexpr const float lineHeight = 16.f;
};

template<FaustParameterIndex id>
class VoiceIsolationToggleWidget final : public ToggleSwitchBaseWidget
{
    using R = VoiceIsolationToggleReference;
    using BaseWidget = ToggleSwitchBaseWidget;

public:
    explicit VoiceIsolationToggleWidget(LabWidget* const parent)
        : BaseWidget(parent, kParametersMainStart + id, kFaustParameters[id].name)
    {
        setChecked(d_isNotZero(kFaustParameters[id].init), false);
        updateSize(false);
    }

private:
    void setupFont()
    {
        fontFace("regular");
        fontSize(R::fontSize * fScaleFactor);
        textLetterSpacing(R::letterSpacing * fScaleFactor);
    }

    // the name in two lines, split at its first space, or one line without a space
    [[nodiscard]] std::array<std::pair<const char*, const char*>, 2> lines() const noexcept
    {
        const char* const name = getName();
        const char* const space = std::strchr(name, ' ');

        if (space == nullptr)
            return {{ { name, nullptr }, { nullptr, nullptr } }};

        return {{ { name, space }, { space + 1, nullptr } }};
    }

    void onNanoDisplay() final
    {
        constexpr const Color& accent = accentColor<id>();
        const auto [first, second] = lines();
        const float x = R::margin * fScaleFactor;
        const float cy = getHeight() * 0.5f;
        const float half = R::lineHeight * 0.5f * fScaleFactor;

        setupFont();
        fillColor(isChecked() ? accent : isHovered() ? accent.withAlpha(0.7f) : R::color);
        textAlign(ALIGN_LEFT | ALIGN_MIDDLE);

        if (second.first == nullptr)
        {
            text(x, cy, first.first, first.second);
            return;
        }

        text(x, cy - half, first.first, first.second);
        text(x, cy + half, second.first, second.second);
    }

    void updateSize(const bool updateChildren) final
    {
        setupFont();
        textAlign(0);

        float width = 0.f;
        for (const auto& [start, end] : lines())
        {
            if (start == nullptr)
                continue;

            Rectangle<float> bounds;
            textBounds(0, 0, start, end, bounds);
            width = std::max(width, bounds.getWidth());
        }

        setSize(d_roundToUnsignedInt(width + R::margin * 2 * fScaleFactor),
                d_roundToUnsignedInt(R::height * fScaleFactor));

        BaseWidget::updateSize(updateChildren);
    }
};

// --------------------------------------------------------------------------------------------------------------------
// A floating panel holding one group of controls: number boxes, which share whatever width the row gives the
// group, and controls of their own width.

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

private:
    // a light tint, lit along the top edge, shaded along the bottom, with a faint rim all around;
    // the stage draws its shadow, which reaches outside the panel
    void onNanoDisplay() final
    {
        drawReferenceBackground<R>();

        const float w = getWidth();
        const float h = getHeight();
        const float r = R::borderRadius * fScaleFactor;

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

class VoiceIsolationDenoiseGroupWidget final : public VoiceIsolationGroupWidget
{
    const std::array<std::shared_ptr<LabWidget>, 4> widgets {
        addWidget<VoiceIsolationNumberBoxWidget<kFaustParameterDepth>, Expanding>(kFaustParameterDepth),
        addWidget<VoiceIsolationNumberBoxWidget<kFaustParameterMax_reduction>, Expanding>(kFaustParameterMax_reduction),
        addWidget<VoiceIsolationNumberBoxWidget<kFaustParameterRelease>, Expanding>(kFaustParameterRelease),
        addWidget<VoiceIsolationToggleWidget<kFaustParameterPitch_enhancement>>(),
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
// A group's name above its panel, centred, in the suite's bracket text style.

struct VoiceIsolationGroupLabelReference : Reference::Zero {
    using Bracket = Reference::Widgets::KnobGroup::Bracket;
    static constexpr const uint height = 18;
};

class VoiceIsolationGroupLabelWidget final : public LabWidget
{
    using R = VoiceIsolationGroupLabelReference;

    const char* const fLabel;

public:
    VoiceIsolationGroupLabelWidget(LabWidget* const parent, const char* const label)
        : LabWidget(parent),
          fLabel(label)
    {
        updateSize(false);
    }

private:
    void onNanoDisplay() final
    {
        fontFace("regular");
        fontSize(R::Bracket::fontSize * fScaleFactor);
        textLetterSpacing(R::Bracket::letterSpacing * fScaleFactor);
        textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
        fillColor(R::Bracket::color);
        text(getWidth() * 0.5f, getHeight() * 0.5f, fLabel, nullptr);
    }

    // as wide as the group makes it
    void updateSize(const bool updateChildren) final
    {
        setSize(std::max(1u, getWidth()), d_roundToUnsignedInt(R::height * fScaleFactor));
        LabWidget::updateSize(updateChildren);
    }
};

// A panel with its label above it. The row sizes these, and they pass the sizes on to their panel.

struct VoiceIsolationLabeledGroupReference : Reference::Zero {
    static constexpr const uint padding = 4;
    static constexpr const uint height = VoiceIsolationGroupLabelReference::height + padding
                                       + VoiceIsolationGroupReference::height;
};

class VoiceIsolationLabeledGroupWidget : public ReferenceContainerWidget<VoiceIsolationLabeledGroupReference, kVertical>
{
    using BaseWidget = ReferenceContainerWidget<VoiceIsolationLabeledGroupReference, kVertical>;

    // addWidget's layout only points at its widgets, these own them
    const std::shared_ptr<VoiceIsolationGroupLabelWidget> fLabel;

protected:
    std::shared_ptr<VoiceIsolationGroupWidget> fPanel;

public:
    VoiceIsolationLabeledGroupWidget(LabWidget* const parent, const char* const label)
        : BaseWidget(parent),
          fLabel(addWidget<VoiceIsolationGroupLabelWidget>(label)) {}

    [[nodiscard]] const VoiceIsolationGroupWidget& getPanel() const noexcept
    {
        return *fPanel;
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
};

template<class Panel>
class VoiceIsolationLabeledGroupOf final : public VoiceIsolationLabeledGroupWidget
{
public:
    VoiceIsolationLabeledGroupOf(LabWidget* const parent, const char* const label)
        : VoiceIsolationLabeledGroupWidget(parent, label)
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

    const std::array<std::shared_ptr<VoiceIsolationLabeledGroupWidget>, 3> fGroups {
        addWidget<VoiceIsolationLabeledGroupOf<VoiceIsolationDenoiseGroupWidget>>("DENOISER"),
        addWidget<VoiceIsolationLabeledGroupOf<VoiceIsolationGateGroupWidget>>("GATE"),
        addWidget<VoiceIsolationLabeledGroupOf<VoiceIsolationMixGroupWidget>>("MIXER"),
    };

public:
    explicit VoiceIsolationGroupRowWidget(LabWidget* const parent)
        : BaseWidget(parent) {}

    [[nodiscard]] const std::array<std::shared_ptr<VoiceIsolationLabeledGroupWidget>, 3>& getGroups() const noexcept
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

        const float top = R::margin * s;
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

        // hairline grid, 0 dB stronger, labelled on both sides as mbComp5 does
        {
            const float labelMargin = 10.f * s;
            char label[16];

            fontFace("regular");
            fontSize(10.f * s);
            textLetterSpacing(0.f);
            strokeWidth(1.f);

            for (float db = kDisplayDbMax; db >= kDisplayDbMin; db -= kDisplayDbStep)
            {
                const float y = std::round(yOfDb(db)) + 0.5f;

                beginPath();
                moveTo(0, y);
                lineTo(w, y);
                strokeColor(Color(0x64, 0x64, 0x70, d_isZero(db) ? 0.5f : 0.22f));
                stroke();

                std::snprintf(label, sizeof(label), "%d%s", static_cast<int>(db),
                              d_isEqual(db, kDisplayDbMin) ? " dB" : "");

                fillColor(Color(140, 142, 150));
                textAlign(ALIGN_LEFT | ALIGN_BOTTOM);
                text(labelMargin, y - 2.f * s, label, nullptr);
                textAlign(ALIGN_RIGHT | ALIGN_BOTTOM);
                text(w - labelMargin, y - 2.f * s, label, nullptr);
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

        // fill between 0 dB and the curve, fading toward 0 dB in the curve's colours
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
            fillPaint(linearGradient(0, zero, 0, edge, rainbow((left + right) / (2 * w), 0.015f),
                                                       rainbow((left + right) / (2 * w), 0.24f)));
            fill();
        }

        // the curve in four sections, each a gradient between two rainbow colours, over a wide faint glow
        for (uint section = 0; section < kRainbow.size() - 1; ++section)
        {
            const float left = section * w / (kRainbow.size() - 1);
            const float right = (section + 1) * w / (kRainbow.size() - 1);
            const float opacity = off ? 0.25f : 0.95f;

            save();
            intersectScissor(left, 0, right - left, h);

            path();
            strokeColor(rainbow((section + 0.5f) / (kRainbow.size() - 1), off ? 0.04f : 0.13f));
            strokeWidth(6.f * s);
            stroke();

            path();
            strokePaint(linearGradient(left, 0, right, 0, Color(kRainbow[section], opacity),
                                                          Color(kRainbow[section + 1], opacity)));
            strokeWidth(2.f * s);
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
            const VoiceIsolationGroupWidget& panel = group->getPanel();
            const float x = panel.getAbsoluteX() - getAbsoluteX();
            const float y = panel.getAbsoluteY() - getAbsoluteY();
            const float w = panel.getWidth();
            const float h = panel.getHeight();

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
