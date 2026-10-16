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
#include <cmath>
#include <cstdio>

// --------------------------------------------------------------------------------------------------------------------

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------
// The stage is the display, with the starfield, the input waveform and the gain reduction behind it and the level
// grid and threshold line over them, and the control bar floats at its bottom.
// The bar is styled like the mbComp5 band groups.

struct LimiterBarReference : Reference::Zero {
    static constexpr const Color backgroundColor { 0xff, 0xff, 0xff, 0.05f };
    static constexpr const uint borderRadius = 8;
    static constexpr const uint margin = 8;
    static constexpr const uint padding = 6;
    static constexpr const uint height = Reference::Widgets::NumberBox<>::height + margin * 2;
};

// accent colour of a parameter, as set by [accentcolor] in limiter.dsp

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
using LimiterNumberBoxWidget = NumberBoxWidget<accentColor<id>()>;

// An on / off parameter shown as its name only: grey when off, accent colour when on, 70% accent when hovered.
// Two of them stack in the height of a number box.

struct LimiterToggleReference : Reference::Zero {
    static constexpr const Color color = Reference::Colors::ink3;
    static constexpr const float fontSize = 13.f;
    static constexpr const float letterSpacing = fontSize * 0.06f;
    static constexpr const uint margin = 6;
    static constexpr const uint height = Reference::Widgets::NumberBox<>::height / 2;
};

template<const Color& kAccentColor>
class LimiterToggleWidget final : public ToggleSwitchBaseWidget
{
    using R = LimiterToggleReference;
    using BaseWidget = ToggleSwitchBaseWidget;

public:
    explicit LimiterToggleWidget(LabWidget* const parent, const FaustParameterIndex id)
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

    void onNanoDisplay() final
    {
        setupFont();
        fillColor(isChecked() ? kAccentColor : isHovered() ? kAccentColor.withAlpha(0.7f) : R::color);
        textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
        text(R::margin * fScaleFactor, getHeight() * 0.5f, getName(), nullptr);
    }

    void updateSize(const bool updateChildren) final
    {
        Rectangle<float> bounds;
        setupFont();
        textAlign(0);
        textBounds(0, 0, getName(), nullptr, bounds);

        setSize(d_roundToUnsignedInt(bounds.getWidth() + R::margin * 2 * fScaleFactor),
                d_roundToUnsignedInt(R::height * fScaleFactor));

        BaseWidget::updateSize(updateChildren);
    }
};

class LimiterTogglesWidget final : public ReferenceContainerWidget<Reference::Zero, kVertical>
{
    using BaseWidget = ReferenceContainerWidget<Reference::Zero, kVertical>;

    const std::array<std::shared_ptr<LabWidget>, 2> widgets {
        addWidget<LimiterToggleWidget<accentColor<kFaustParameterTrue_peak>()>>(kFaustParameterTrue_peak),
        addWidget<LimiterToggleWidget<accentColor<kFaustParameterUnity_gain>()>>(kFaustParameterUnity_gain),
    };

public:
    explicit LimiterTogglesWidget(LabWidget* const parent)
        : BaseWidget(parent)
    {
        updateSize(false);
    }

private:
    void updateSize(const bool updateChildren) final
    {
        uint width = 0;
        for (const std::shared_ptr<LabWidget>& widget : widgets)
            width = std::max(width, widget->getWidth());

        LabWidget::setSize(width, d_roundToUnsignedInt(Reference::Widgets::NumberBox<>::height * fScaleFactor));

        BaseWidget::updateSize(updateChildren);
    }
};

// --------------------------------------------------------------------------------------------------------------------

// fixed space between the groups of number boxes, so the boxes get the width left over

struct LimiterGapReference : Reference::Zero {
    static constexpr const uint width = 18;
    static constexpr const uint height = 1;
};

class LimiterBarWidget final : public ReferenceContainerWidget<LimiterBarReference>
{
    using BaseWidget = ReferenceContainerWidget<LimiterBarReference>;
    using GapWidget = LabReferenceWidget<LimiterGapReference>;

    const std::array<std::shared_ptr<LabWidget>, 10> widgets {
        addWidget<LimiterTogglesWidget>(),
        addSpacer(),
        addWidget<LimiterNumberBoxWidget<kFaustParameterThreshold>, Expanding>(kFaustParameterThreshold),
        addWidget<LimiterNumberBoxWidget<kFaustParameterCeiling>, Expanding>(kFaustParameterCeiling),
        addWidget<GapWidget>(),
        addWidget<LimiterNumberBoxWidget<kFaustParameterLookahead>, Expanding>(kFaustParameterLookahead),
        addWidget<GapWidget>(),
        addWidget<LimiterNumberBoxWidget<kFaustParameterRelease>, Expanding>(kFaustParameterRelease),
        addWidget<LimiterNumberBoxWidget<kFaustParameterAuto_release>, Expanding>(kFaustParameterAuto_release),
        addSpacer(),
    };

public:
    explicit LimiterBarWidget(LabWidget* const parent)
        : BaseWidget(parent) {}

private:
    // faint rim all around, lit along the top edge
    void onNanoDisplay() final
    {
        drawReferenceBackground<LimiterBarReference>();

        const float w = getWidth();
        const float h = getHeight();
        const float r = LimiterBarReference::borderRadius * fScaleFactor;

        beginPath();
        roundedRect(0.5f, 0.5f, w - 1.f, h - 1.f, r - 0.5f);
        strokeColor(Color(1.f, 1.f, 1.f, 0.05f));
        strokeWidth(1.f);
        stroke();

        save();
        intersectScissor(0, 0, w, r);
        beginPath();
        roundedRect(0.5f, 0.5f, w - 1.f, h - 1.f, r - 0.5f);
        strokePaint(linearGradient(0, 0, 0, r, Color(1.f, 1.f, 1.f, 0.12f), Color(1.f, 1.f, 1.f, 0.f)));
        strokeWidth(1.f);
        stroke();
        restore();
    }
};

// --------------------------------------------------------------------------------------------------------------------

// The display's level scale, shared with the waveform shader: dB at its top and bottom edge.

static constexpr const float kDisplayDbMax = 0.f;
static constexpr const float kDisplayDbMin = -24.f;
static constexpr const float kDisplayDbStep = 6.f;

// mbComp5 rainbow
static constexpr const std::array<Color, 5> kRainbow {{
    { 255, 191, 203 }, { 255, 223, 173 }, { 210, 253, 211 }, { 190, 241, 255 }, { 218, 193, 243 },
}};

static Color rainbow(const float t) noexcept
{
    const float p = std::clamp(t, 0.f, 1.f) * (kRainbow.size() - 1);
    const size_t i = std::min(kRainbow.size() - 2, static_cast<size_t>(p));
    return Color(kRainbow[i], kRainbow[i + 1], p - i);
}

// --------------------------------------------------------------------------------------------------------------------
// The stage is the display: the shaders fill it, and it draws the level grid, its dB ticks and the threshold line
// over them. The threshold line is dragged up and down to set the Threshold.

class LimiterStageWidget final : public ReferenceContainerWidget<Reference::Stage, kVertical>
{
    using R = Reference::Stage;
    using BaseWidget = ReferenceContainerWidget<R, kVertical>;

    static constexpr const uint32_t kThresholdId = kParametersMainStart + kFaustParameterThreshold;

    // how close to the threshold line, in px at scale 1, a press or hover catches it
    static constexpr const float kGrabDistance = 6.f;

    const std::array<std::shared_ptr<LabWidget>, 2> widgets {
        addSpacer(),
        addWidget<LimiterBarWidget>(),
    };

    bool fHover = false;
    bool fDragging = false;
    float fDragOffset = 0.f;

public:
    explicit LimiterStageWidget(LabWidget* const parent)
        : BaseWidget(parent) {}

private:
    [[nodiscard]] float yOfDb(const float db) const noexcept
    {
        return (kDisplayDbMax - db) / (kDisplayDbMax - kDisplayDbMin) * getHeight();
    }

    [[nodiscard]] float dbAtY(const float y) const noexcept
    {
        return kDisplayDbMax - y / getHeight() * (kDisplayDbMax - kDisplayDbMin);
    }

    [[nodiscard]] float threshold() const noexcept
    {
        return fInterface->getParameterValue(kThresholdId);
    }

    [[nodiscard]] bool nearThreshold(const float y) const noexcept
    {
        return std::abs(y - yOfDb(threshold())) <= kGrabDistance * fScaleFactor;
    }

    void setHover(const bool hover)
    {
        if (fHover == hover)
            return;
        fHover = hover;
        getWindow().setCursor(hover ? kMouseCursorUpDown : kMouseCursorArrow);
        repaint();
    }

    void onNanoDisplay() final
    {
        BaseWidget::onNanoDisplay();

        const float s = fScaleFactor;
        const float w = getWidth();
        const float border = R::border * s;

        // level grid and dB ticks below the top edge, which is 0 dB
        fontFace("mono");
        fontSize(10.f * s);
        textAlign(ALIGN_LEFT | ALIGN_BOTTOM);

        for (float db = kDisplayDbMax - kDisplayDbStep; db > kDisplayDbMin; db -= kDisplayDbStep)
        {
            const float y = std::round(yOfDb(db)) + 0.5f;

            beginPath();
            moveTo(border, y);
            lineTo(w - border, y);
            strokeColor(Color(Reference::Colors::line2, 0.22f));
            strokeWidth(1.f);
            stroke();

            char label[8];
            std::snprintf(label, sizeof(label), "%d", static_cast<int>(db));
            fillColor(Reference::Colors::ink3);
            text(6.f * s, y - 2.f * s, label, nullptr);
        }

        // threshold: dotted line coloured through the rainbow from left to right, brighter while hovered or dragged
        const float value = threshold();
        const float y = yOfDb(value);
        const bool lit = fHover || fDragging;
        const float gap = 6.f * s;
        const float radius = (lit ? 1.5f : 1.2f) * s;

        for (float x = border + gap * 0.5f; x < w - border; x += gap)
        {
            beginPath();
            circle(x, y, radius);
            fillColor(rainbow(x / w).withAlpha(lit ? 1.f : 0.85f));
            fill();
        }

        char label[32];
        std::snprintf(label, sizeof(label), "THRESHOLD %.1f dB", value);
        fontFace("regular");
        fontSize(10.f * s);
        textLetterSpacing(1.f * s);
        textAlign(ALIGN_RIGHT | ALIGN_TOP);
        fillColor(rainbow(1.f).withAlpha(lit ? 1.f : 0.85f));
        text(w - 8.f * s, y + 4.f * s, label, nullptr);
        textLetterSpacing(0.f);
    }

    bool onMouse(const Widget::MouseEvent& ev) final
    {
        if (ev.button != 1)
            return BaseWidget::onMouse(ev);

        if (ev.press && nearThreshold(ev.pos.getY()))
        {
            fDragging = true;
            fDragOffset = ev.pos.getY() - yOfDb(threshold());
            fInterface->parameterControlPressed(kThresholdId);
            repaint();
            return true;
        }

        if (! ev.press && fDragging)
        {
            fDragging = false;
            fInterface->parameterControlReleased(kThresholdId);
            setHover(contains(ev.pos) && nearThreshold(ev.pos.getY()));
            repaint();
            return true;
        }

        return BaseWidget::onMouse(ev);
    }

    bool onMotion(const Widget::MotionEvent& ev) final
    {
        if (fDragging)
        {
            const FaustParameter& param = kFaustParameters[kFaustParameterThreshold];
            const float db = dbAtY(ev.pos.getY() - fDragOffset);
            const float value = std::clamp(std::round(db / param.step) * param.step, param.min, param.max);

            if (d_isNotEqual(value, threshold()))
            {
                fInterface->parameterControlModified(kThresholdId, value);
                repaint();
            }
            return true;
        }

        setHover(contains(ev.pos) && nearThreshold(ev.pos.getY()));

        return BaseWidget::onMotion(ev);
    }
};

using LimiterRootWidget = RootWidget<TopBar, MainAreaContainerWidget<LimiterStageWidget>>;

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

class LimiterUI final : public LibreAudioBaseUI
{
    static constexpr const double kWaveformSize = kNumSamplePointsForWaveform;
    static constexpr const double kWaveformRate = kWaveformSize / kNumSecondsForWaveform; // points per second
    static constexpr const double kWaveformLag = kWaveformRate * 0.05; // points shown behind the newest one

    using WaveformShaderWidget = LibreAudio::BotShaderWidget<SHADERS_WAVEFORM_SCROLL_FRAG_DATA,
                                                             SHADERS_WAVEFORM_SCROLL_FRAG_LEN,
                                                             kNumSamplePointsForWaveform>;

    using GainReductionShaderWidget = LibreAudio::BotShaderWidget<SHADERS_CURVE_LIMITER_FRAG_DATA,
                                                                  SHADERS_CURVE_LIMITER_FRAG_LEN,
                                                                  kNumSamplePointsForWaveform>;

    const std::unique_ptr<LibreAudio::ShaderBaseWidget> fShaderBackground {
        new LibreAudio::BotShaderWidget<SHADERS_SHADERTOY_CLOUDSTARFIELD_FRAG_DATA,
                                        SHADERS_SHADERTOY_CLOUDSTARFIELD_FRAG_LEN>(this, this)
    };

    // scrolling input peaks, above the starfield
    const std::unique_ptr<WaveformShaderWidget> fShaderWaveform {
        new WaveformShaderWidget(this, this)
    };

    // gain reduction over the waveform, from a ring filled with the waveform's points and read with the same head
    const std::unique_ptr<GainReductionShaderWidget> fShaderGainReduction {
        new GainReductionShaderWidget(this, this)
    };

    // points pushed so far, and the point the waveform is drawn up to (fractional, moving at kWaveformRate)
    double fPointsPushed = 0;
    double fPointsShown = 0;
    double fLastTime = 0;
    float fHead = 0;

public:
    LimiterUI()
        : LibreAudioBaseUI()
    {
        createRootWidget<LibreAudio::LimiterRootWidget>();

        fRootWidget->enableShaders({ fShaderBackground.get(), fShaderWaveform.get(), fShaderGainReduction.get() });

        const std::array<LibreAudio::BotShaderBaseWidget*, 2> scrolling { fShaderWaveform.get(), fShaderGainReduction.get() };

        for (LibreAudio::BotShaderBaseWidget* const shader : scrolling)
        {
            shader->setCustomUniform("uHead", [this] { return waveformHead(); });
            shader->setCustomUniform("uDbMin", [] { return LibreAudio::kDisplayDbMin; });
            shader->setCustomUniform("uDbMax", [] { return LibreAudio::kDisplayDbMax; });
        }
    }

private:
    void audioPeaksReceived(const LibreAudioWaveformIPC<LIBREAUDIO_WANT_GRAPH_IO_COUNT>::ValueType& value) final
    {
        // each point carries the input peak and the deepest gain reduction within it (0..1 of the "gr" meter's range),
        // pushed to the same ring position in both shaders; the reduction goes in as dB (positive)
        fShaderWaveform->push(value.ptr[0]);
        fShaderGainReduction->push(value.ptr[1] * -kFaustParameters[kFaustParameterGr].min);
        fPointsPushed += 1;
    }

    // Peaks arrive in bursts, so the shown point runs at the steady rate and is eased towards kWaveformLag behind
    // the newest point, never past it. Returns its ring position: push number n lands at (size - 2 + n) mod size.
    // Both shaders ask for it each frame; the second ask within a millisecond gets the same head.
    float waveformHead()
    {
        const double now = getApp().getTime();
        if (fLastTime != 0 && now - fLastTime < 0.001)
            return fHead;

        const double dt = fLastTime != 0 ? std::min(now - fLastTime, 0.1) : 0;
        fLastTime = now;

        fPointsShown += dt * kWaveformRate;

        const double error = fPointsPushed - kWaveformLag - fPointsShown;

        if (std::abs(error) > kWaveformRate * 0.5)
            fPointsShown += error;
        else
            fPointsShown += error * std::min(1.0, dt * 2);

        fPointsShown = std::min(fPointsShown, fPointsPushed);

        double head = std::fmod(fPointsShown - 2, kWaveformSize);
        if (head < 0)
            head += kWaveformSize;
        fHead = static_cast<float>(head);
        return fHead;
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LimiterUI)
};

UI* createUI()
{
    return new LimiterUI();
}

END_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------
