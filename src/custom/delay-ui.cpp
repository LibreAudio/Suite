// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "LibreAudioBaseUI.hpp"

#include "ui/containers/main-area.hpp"
#include "ui/containers/top-bar.hpp"
#include "ui/widgets/number-box.hpp"
#include "ui/widgets/shader.hpp"

#include "LibreAudioParameters.hpp"

#include "las-resources.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>

// --------------------------------------------------------------------------------------------------------------------

namespace LibreAudio {

// height of the number boxes, taller than the reference one, as in voiceIsolation
static constexpr const uint kBoxHeight = 45;

// height of the checkboxes, as in the limiter
static constexpr const uint kRowHeight = 20;

// width of the number boxes, the same in every panel
static constexpr const uint kBoxWidth = 120;

// --------------------------------------------------------------------------------------------------------------------
// The controls sit in panels floating over the stage, styled like the mbComp5 band groups, as in the limiter.

struct DelayGroupReference : Reference::Zero {
    static constexpr const Color backgroundColor { 0x30, 0x32, 0x3c, 0.7f };
    static constexpr const Color borderColor = backgroundColor;
    static constexpr const Color borderHighlightColor { 0x40, 0x42, 0x4c, 0.8f };
    static constexpr const Color borderShadowColor { 0, 0, 0, 0.12f };
    static constexpr const uint border = 1;
    static constexpr const uint borderRadius = 8;
    static constexpr const uint margin = 8;
    static constexpr const uint padding = 6;

    static constexpr const Color shadowColor { 0, 0, 0, 0.6f };
    static constexpr const Point<int> shadowOffset = { 0, 5 };
    static constexpr const Size<int> shadowSize = { -12, -12 };
    static constexpr const uint shadowFeather = 14;
};

// accent colour of a parameter, as set by [accentcolor] in delay.dsp

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
using DelayNumberBoxWidget = NumberBoxWidget<accentColor<id>(), kBoxHeight>;

// sets a parameter as one gesture, the way a click on a control does

static void setParameterGesture(LabUIWidgetInterface* const iface, const uint32_t id, const float value)
{
    iface->parameterControlPressed(id);
    iface->parameterControlModified(id, value);
    iface->parameterControlReleased(id);
}

// --------------------------------------------------------------------------------------------------------------------
// A slim checkbox for an on / off parameter, as in the limiter: a square box at the left edge, with a check mark in
// the accent colour when on, then its name in the accent colour. A click anywhere on it toggles the parameter.

class DelayCheckboxWidget final : public LabWidget
{
    static constexpr const float kGap = 6.f;
    static constexpr const float kFontSize = Reference::Widgets::NumberBox<>::Name::fontSize;

    const FaustParameterIndex fId;
    const FaustParameter& fParameter;
    const Color fAccent;
    bool fHover = false;

public:
    explicit DelayCheckboxWidget(LabWidget* const parent, const FaustParameterIndex id)
        : LabWidget(parent),
          fId(id),
          fParameter(kFaustParameters[id]),
          fAccent(Reference::kAccentColors[std::clamp(kFaustParameters[id].accentcolor, 1u, 6u) - 1])
    {
        updateSize(false);
    }

private:
    [[nodiscard]] float boxSize() const noexcept
    {
        return kRowHeight * fScaleFactor;
    }

    [[nodiscard]] bool isChecked() const noexcept
    {
        return d_isNotZero(fInterface->getParameterValue(kParametersMainStart + fId));
    }

    void setupFont()
    {
        fontFace("regular");
        fontSize(kFontSize * fScaleFactor);
        textLetterSpacing(Reference::Widgets::NumberBox<>::Name::letterSpacing * fScaleFactor);
    }

    void onNanoDisplay() final
    {
        const float s = fScaleFactor;
        const float h = getHeight();
        const float size = boxSize();
        const float bx = 0.5f;
        const float by = (h - size) * 0.5f + 0.5f;

        setupFont();
        fillColor(fAccent);
        textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
        text(size + kGap * s, h * 0.5f, fParameter.name, nullptr);
        textLetterSpacing(0.f);

        beginPath();
        roundedRect(bx, by, size - 1.f, size - 1.f, 4.f * s);
        fillColor(Reference::Colors::track);
        fill();
        strokeColor(Color(1.f, 1.f, 1.f, fHover ? 0.15f : 0.06f));
        strokeWidth(1.f);
        stroke();

        if (! isChecked())
            return;

        const float cx = bx + size * 0.5f;
        const float cy = by + size * 0.5f;
        beginPath();
        moveTo(cx - 4.f * s, cy);
        lineTo(cx - 1.5f * s, cy + 2.5f * s);
        lineTo(cx + 4.f * s, cy - 3.f * s);
        lineCap(ROUND);
        lineJoin(ROUND);
        strokeColor(fAccent);
        strokeWidth(1.6f * s);
        stroke();
    }

    bool onMouse(const Widget::MouseEvent& ev) final
    {
        if (ev.press && ev.button == 1 && contains(ev.pos))
        {
            setParameterGesture(fInterface, kParametersMainStart + fId, isChecked() ? 0.f : 1.f);
            repaint();
            return true;
        }
        return LabWidget::onMouse(ev);
    }

    bool onMotion(const Widget::MotionEvent& ev) final
    {
        if (const bool hover = contains(ev.pos); hover != fHover)
        {
            fHover = hover;
            repaint();
        }
        return LabWidget::onMotion(ev);
    }

    // as wide as its box and name need, or wider when its cell makes it so
    void updateSize(const bool updateChildren) final
    {
        Rectangle<float> bounds;
        setupFont();
        textAlign(0);
        textBounds(0, 0, fParameter.name, nullptr, bounds);
        textLetterSpacing(0.f);

        const uint width = d_roundToUnsignedInt(bounds.getWidth() + kGap * fScaleFactor + boxSize());
        setSize(std::max(width, getWidth()), d_roundToUnsignedInt(kRowHeight * fScaleFactor));

        LabWidget::updateSize(updateChildren);
    }
};

// --------------------------------------------------------------------------------------------------------------------
// A row of a panel: one control per parameter, the panel's padding apart, sharing its width.

template<uint kHeight, template<FaustParameterIndex> class Control, FaustParameterIndex... kIds>
class DelayPanelRowWidget final
    : public ReferenceContainerWidget<Reference::Height<kHeight, Reference::Padding<DelayGroupReference::padding>>>
{
    using BaseWidget = ReferenceContainerWidget<Reference::Height<kHeight, Reference::Padding<DelayGroupReference::padding>>>;

    const std::array<std::shared_ptr<LabWidget>, sizeof...(kIds)> widgets {
        this->template addWidget<Control<kIds>, Expanding>(kIds)...
    };

public:
    static constexpr const uint height = kHeight;

    explicit DelayPanelRowWidget(LabWidget* const parent)
        : BaseWidget(parent) {}
};

template<FaustParameterIndex>
using DelayCheckboxCellWidget = DelayCheckboxWidget;

template<FaustParameterIndex... kIds>
using DelayBoxRowWidget = DelayPanelRowWidget<kBoxHeight, DelayNumberBoxWidget, kIds...>;

template<FaustParameterIndex... kIds>
using DelayCheckboxRowWidget = DelayPanelRowWidget<kRowHeight, DelayCheckboxCellWidget, kIds...>;

// --------------------------------------------------------------------------------------------------------------------
// A floating panel: rows of controls, kColumns number boxes wide, sized to fit them.

template<uint kColumns, class... Rows>
class DelayPanelWidget final : public ReferenceContainerWidget<DelayGroupReference, kVertical>
{
    using R = DelayGroupReference;
    using BaseWidget = ReferenceContainerWidget<R, kVertical>;

    static constexpr const uint kWidth = (R::border + R::margin) * 2 + kBoxWidth * kColumns + R::padding * (kColumns - 1);
    static constexpr const uint kHeight = (R::border + R::margin) * 2 + (Rows::height + ...)
                                        + R::padding * static_cast<uint>(sizeof...(Rows) - 1);

    const std::array<std::shared_ptr<LabWidget>, sizeof...(Rows)> rows {
        addWidget<Rows>()...
    };

public:
    explicit DelayPanelWidget(LabWidget* const parent)
        : BaseWidget(parent)
    {
        updateSize(false);
    }

private:
    void updateSize(const bool updateChildren) final
    {
        LabWidget::setSize(d_roundToUnsignedInt(kWidth * fScaleFactor), d_roundToUnsignedInt(kHeight * fScaleFactor));
        BaseWidget::updateSize(updateChildren);
    }

    // faint rim all around, lit along the top edge
    void onNanoDisplay() final
    {
        drawReferenceBackground<R>();

        const float w = getWidth();
        const float h = getHeight();
        const float r = R::borderRadius * fScaleFactor;

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

using DelayRepeatPanelWidget = DelayPanelWidget<1,
    DelayBoxRowWidget<kFaustParameterFeedback>,
    DelayBoxRowWidget<kFaustParameterCross>,
    DelayBoxRowWidget<kFaustParameterDrive>>;

using DelayDynamicsPanelWidget = DelayPanelWidget<1,
    DelayBoxRowWidget<kFaustParameterDeess_amount>,
    DelayBoxRowWidget<kFaustParameterDuck>>;

using DelayModulationPanelWidget = DelayPanelWidget<1,
    DelayBoxRowWidget<kFaustParameterMod_rate>,
    DelayBoxRowWidget<kFaustParameterMod_depth>,
    DelayBoxRowWidget<kFaustParameterMod_stereo_phase>>;

using DelayOutputPanelWidget = DelayPanelWidget<1,
    DelayBoxRowWidget<kFaustParameterWidth>,
    DelayBoxRowWidget<kFaustParameterDrywet>>;

using DelayTimePanelWidget = DelayPanelWidget<2,
    DelayBoxRowWidget<kFaustParameterTime_l, kFaustParameterTime_r>,
    DelayBoxRowWidget<kFaustParameterOffset_l, kFaustParameterOffset_r>,
    DelayCheckboxRowWidget<kFaustParameterSync, kFaustParameterLink>>;

using DelayFilterPanelWidget = DelayPanelWidget<2,
    DelayBoxRowWidget<kFaustParameterHp_freq, kFaustParameterLp_freq>>;

// gap between panels, and between the columns of the stage
static constexpr const uint kPanelGap = 8;

// --------------------------------------------------------------------------------------------------------------------
// A side column of the stage: panels from the top down, kPanelGap apart, the column as wide as they are.

template<class... Panels>
class DelayColumnWidget final : public ReferenceContainerWidget<Reference::Padding<kPanelGap>, kVertical>
{
    using BaseWidget = ReferenceContainerWidget<Reference::Padding<kPanelGap>, kVertical>;

    const std::array<std::shared_ptr<LabWidget>, sizeof...(Panels)> fPanels {
        addWidget<Panels>()...
    };
    const std::shared_ptr<LabWidget> fSpacer = addSpacer();

public:
    explicit DelayColumnWidget(LabWidget* const parent)
        : BaseWidget(parent)
    {
        updateSize(false);
    }

private:
    void updateSize(const bool updateChildren) final
    {
        uint width = 0;
        for (const std::shared_ptr<LabWidget>& panel : fPanels)
            width = std::max(width, panel->getWidth());

        LabWidget::setWidth(width);
        BaseWidget::updateSize(updateChildren);
    }
};

// A panel centred in the width it is given, as tall as the panel.

template<class Panel>
class DelayCenteredWidget final : public ReferenceContainerWidget<Reference::Zero>
{
    using BaseWidget = ReferenceContainerWidget<Reference::Zero>;

    const std::shared_ptr<LabWidget> fLeft = addSpacer();
    const std::shared_ptr<LabWidget> fPanel = addWidget<Panel>();
    const std::shared_ptr<LabWidget> fRight = addSpacer();

public:
    explicit DelayCenteredWidget(LabWidget* const parent)
        : BaseWidget(parent)
    {
        updateSize(false);
    }

private:
    void updateSize(const bool updateChildren) final
    {
        LabWidget::setHeight(fPanel->getHeight());
        BaseWidget::updateSize(updateChildren);
    }
};

// --------------------------------------------------------------------------------------------------------------------
// The tap model behind the scope. curve-delay.frag draws the scope from the same formulas; this copy exists only to
// place the text a shader cannot draw. Change one, change both.

// prototype CSS px to plugin px at scale 1, REFPX in the shader
static constexpr const float kScopeRefPx = 1.5f;

// scope inset inside the display, prototype px, INSET_* in the shader
static constexpr const float kScopeInsetTop = 20.f;
static constexpr const float kScopeInsetX = 14.f;
static constexpr const float kScopeInsetBottom = 16.f;

struct DelayScopeModel {
    static constexpr const float kMaxDelayMs = 4000.f;
    static constexpr const float kAmpMin = 0.006f;
    static constexpr const float kMaxTaps = 32.f;

    float msL, msR;
    float g;
    int pingpong;
    int numTaps;    // repeats down to kAmpMin -- sets the zoom level only, every repeat on screen is drawn
    float level;    // the zoom level these settings call for, ms
    float span;     // the span actually shown, ms -- level, or a step of the flip toward it
    float gridStep;

    explicit DelayScopeModel(LabUIWidgetInterface* const iface)
    {
        const auto param = [iface](const FaustParameterIndex index) {
            return iface->getParameterValue(kParametersMainStart + index);
        };

        static constexpr const float kDivTable[] = {
            4.f, 2.f, 1.5f, 1.f, 2.f / 3.f, 0.75f, 0.5f, 1.f / 3.f, 0.375f, 0.25f, 1.f / 6.f
        };
        const auto divMult = [](const float index) {
            return kDivTable[std::clamp(d_roundToIntPositive(index), 0, static_cast<int>(ARRAY_SIZE(kDivTable)) - 1)];
        };

        // as delay.dsp resolves them
        const bool sync = param(kFaustParameterSync) > 0.5f;
        const bool linked = param(kFaustParameterLink) > 0.5f;
        const float beat = 60000.f / std::max(param(kFaustParameterBpm), 1.f);
        const float rawL = sync ? divMult(param(kFaustParameterDiv_l)) * beat : param(kFaustParameterTime_l);
        const float rawR = linked ? rawL : sync ? divMult(param(kFaustParameterDiv_r)) * beat
                                                : param(kFaustParameterTime_r);
        msL = std::clamp(rawL * (1.f + param(kFaustParameterOffset_l) / 100.f), 1.f, kMaxDelayMs);
        msR = std::clamp(rawR * (1.f + param(kFaustParameterOffset_r) / 100.f), 1.f, kMaxDelayMs);

        g = std::clamp(param(kFaustParameterFeedback) / 100.f, 0.f, 0.985f);
        pingpong = d_roundToIntPositive(param(kFaustParameterPingpong));

        numTaps = g > kAmpMin ? static_cast<int>(std::min(kMaxTaps, std::floor(std::log(kAmpMin) / std::log(g)))) : 0;

        float last = pingpong == 0 ? std::max(msL, msR) : 0.5f * (msL + msR);
        last = numTaps > 0 ? numTaps * last : std::max(msL, msR);
        // fixed zoom levels, doubling from 0.6 s to 153.6 s -- see setupModel in the shader
        level = 600.f;
        for (int i = 0; i < 8 && level < last * 1.06f; ++i)
            level *= 2.f;

        setSpan(level);
    }

    // Show `newSpan` ms top to bottom, and pick the grid for it.
    void setSpan(const float newSpan) noexcept
    {
        span = newSpan;
        gridStep = 32000.f;
        for (const float s : { 50.f, 100.f, 125.f, 250.f, 500.f, 1000.f, 2000.f, 4000.f, 8000.f, 16000.f })
        {
            if (span / s <= 7.f)
            {
                gridStep = s;
                break;
            }
        }
    }

    // "250 ms", "1 s", "1.5 s", "1.25 s" -- labelChars in the shader counts these.
    static void format(char* const buffer, const size_t size, const float ms)
    {
        if (ms < 1000.f)
        {
            std::snprintf(buffer, size, "%d ms", d_roundToInt(ms));
            return;
        }

        std::snprintf(buffer, size, "%.2f", ms / 1000.f);

        // trim trailing zeros, then a trailing point
        if (char* const dot = std::strchr(buffer, '.'))
        {
            char* end = dot + std::strlen(dot) - 1;
            while (end > dot && *end == '0')
                *end-- = '\0';
            if (end == dot)
                *end = '\0';
        }

        std::strncat(buffer, " s", size - std::strlen(buffer) - 1);
    }
};

// --------------------------------------------------------------------------------------------------------------------
// Animates the flips between zoom levels. The shader receives the current span as iSpan and the display draws its
// labels from the same value, so a flip is one tween both follow. It is a function of time rather than of repaints, so
// the two agree within a frame whichever of them asks first.

class DelayScopeZoom
{
public:
    // The span to show now, given the level the settings call for.
    float span(const float level, const double now) noexcept
    {
        if (fTo <= 0.f)
        {
            // first call: no flip on opening the UI
            fFrom = fTo = level;
            fStart = now;
        }
        else if (d_isNotEqual(level, fTo))
        {
            // retarget from wherever the flip has got to, so reversing mid-flip does not jump
            fFrom = spanAt(now);
            fTo = level;
            fStart = now;
        }

        return spanAt(now);
    }

private:
    static constexpr const double kFlipSeconds = 0.35;

    float fFrom = 0.f;
    float fTo = 0.f;
    double fStart = 0.0;

    // Eased in and out, and in log span: each doubling is the same zoom step, so the move reads as one zoom
    // rather than a scroll that speeds up toward the long end.
    [[nodiscard]] float spanAt(const double now) const noexcept
    {
        const double x = std::clamp<double>((now - fStart) / kFlipSeconds, 0.0, 1.0);
        const double e = x < 0.5 ? 4.0 * x * x * x : 1.0 - std::pow(2.0 - 2.0 * x, 3.0) / 2.0;
        return static_cast<float>(std::exp(std::log(fFrom) + (std::log(fTo) - std::log(fFrom)) * e));
    }
};

// --------------------------------------------------------------------------------------------------------------------
// The display: the scope, curve-delay.frag, floating over the starfield under a pane of glass, as the compressor's
// curve does. The shader draws underneath this widget, which keeps it on its own bounds and draws over it the text the
// shader cannot draw, the grid times down the left edge and L / R, and then the glass.

class DelayScopeWidget final : public LabWidget
{
    static constexpr const float kBorderRadius = 10.f;

    BotShaderBaseWidget* fShader = nullptr;
    DelayScopeZoom fZoom;

public:
    explicit DelayScopeWidget(LabWidget* const parent)
        : LabWidget(parent) {}

    void setShader(BotShaderBaseWidget* const shader)
    {
        fShader = shader;

        // one zoom, read by the shader and by the labels drawn over it
        fShader->setCustomUniform("iSpan", [this] {
            return fZoom.span(DelayScopeModel(fInterface).level, getApp().getTime());
        });

        syncShader();
    }

private:
    void syncShader()
    {
        if (fShader == nullptr)
            return;

        fShader->setAbsolutePos(getAbsolutePos());
        fShader->setSize(getSize());
        fShader->setBorderRadius(kBorderRadius * fScaleFactor);
        fShader->setVisible(isVisible());
    }

    // the stage sizes and places this before updating it, and on every page change
    void updateSize(const bool updateChildren) final
    {
        LabWidget::updateSize(updateChildren);
        syncShader();
    }

    void onNanoDisplay() final
    {
        drawLabels();
        drawGlass(getWidth(), getHeight(), kBorderRadius * fScaleFactor);
    }

    void drawLabels()
    {
        DelayScopeModel model(fInterface);
        model.setSpan(fZoom.span(model.level, getApp().getTime()));

        const float px = kScopeRefPx * fScaleFactor;
        const float x0 = kScopeInsetX * px;
        const float y0 = kScopeInsetTop * px;
        const float w = getWidth() - 2.f * kScopeInsetX * px;
        const float h = getHeight() - (kScopeInsetTop + kScopeInsetBottom) * px;
        const float fontPx = 8.f * px;

        if (w <= 0.f || h <= 0.f)
            return;

        // dimmed with the scope while bypassed
        const float alpha = fInterface->getParameterValue(kParametersCommonStart + kCommonParameterBypass) > 0.5f
                          ? 0.35f : 1.f;

        char buffer[24];

        fontFace("mono");
        fontSize(fontPx);

        // grid times, on their lines
        fillColor(Color(0x4a, 0x4b, 0x53, alpha));
        textLetterSpacing(0.1f * fontPx);
        textAlign(ALIGN_LEFT | ALIGN_MIDDLE);

        for (float s = model.gridStep; s < model.span; s += model.gridStep)
        {
            DelayScopeModel::format(buffer, sizeof(buffer), s);
            text(x0, y0 + s / model.span * h, buffer, nullptr);
        }

        // channel labels, above the scope
        fillColor(Color(0x6a, 0x6b, 0x74, alpha));
        textLetterSpacing(0.18f * fontPx);
        textAlign(ALIGN_LEFT | ALIGN_TOP);
        text(x0, y0 - 16.f * px, "L", nullptr);
        textAlign(ALIGN_RIGHT | ALIGN_TOP);
        text(x0 + w, y0 - 16.f * px, "R", nullptr);
        textLetterSpacing(0.f);
    }

    // A pane of glass over the scope, as over the compressor's curve: a gloss over the top whose lower edge sweeps up
    // to the right, a faint diagonal sheen across the middle, kept clear of the corners, a rim lit at the top and
    // fading down, and a bright line just inside the top edge.
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
// The Expert page: the scope in the centre, with the Time and Filter panels below it, between the Repeat and Dynamics
// panels on the left and the Modulation and Output panels on the right.

class DelayCentreWidget final : public ReferenceContainerWidget<Reference::Padding<kPanelGap>, kVertical>
{
    using BaseWidget = ReferenceContainerWidget<Reference::Padding<kPanelGap>, kVertical>;

    const std::shared_ptr<DelayScopeWidget> fScope = addWidget<DelayScopeWidget, Expanding>();
    const std::shared_ptr<LabWidget> fTime = addWidget<DelayCenteredWidget<DelayTimePanelWidget>>();
    const std::shared_ptr<LabWidget> fFilter = addWidget<DelayCenteredWidget<DelayFilterPanelWidget>>();

public:
    explicit DelayCentreWidget(LabWidget* const parent)
        : BaseWidget(parent) {}

    [[nodiscard]] DelayScopeWidget* getScope() const noexcept
    {
        return fScope.get();
    }
};

class DelayExpertWidget final : public ReferenceContainerWidget<Reference::Padding<kPanelGap>>
{
    using BaseWidget = ReferenceContainerWidget<Reference::Padding<kPanelGap>>;

    const std::shared_ptr<LabWidget> fLeft =
        addWidget<DelayColumnWidget<DelayRepeatPanelWidget, DelayDynamicsPanelWidget>>();
    const std::shared_ptr<DelayCentreWidget> fCentre = addWidget<DelayCentreWidget, Expanding>();
    const std::shared_ptr<LabWidget> fRight =
        addWidget<DelayColumnWidget<DelayModulationPanelWidget, DelayOutputPanelWidget>>();

public:
    explicit DelayExpertWidget(LabWidget* const parent)
        : BaseWidget(parent) {}

    [[nodiscard]] DelayScopeWidget* getScope() const noexcept
    {
        return fCentre->getScope();
    }
};

// --------------------------------------------------------------------------------------------------------------------
// The stage is the display area, with the starfield behind it: the Expert page in Expert mode, and in Easy mode four
// preset cards along its bottom. The cards are the suite's Easy page cards and do not load anything yet.

class DelayStageWidget final : public ReferenceContainerWidget<Reference::Stage, kVertical>,
                               private IdleCallback
{
    using R = Reference::Stage;
    using BaseWidget = ReferenceContainerWidget<R, kVertical>;

    const std::shared_ptr<DelayExpertWidget> fExpert = addWidget<DelayExpertWidget, Expanding>();
    const std::shared_ptr<LabWidget> fSpacer = addSpacer();
    const std::shared_ptr<CardGroupWidget> fCards = addCards();

    Page fPage = kPageEasy;

public:
    explicit DelayStageWidget(LabWidget* const parent)
        : BaseWidget(parent)
    {
        showPage(getCurrentPage(fInterface));
        addIdleCallback(this);
    }

    void setScopeShader(BotShaderBaseWidget* const shader)
    {
        fExpert->getScope()->setShader(shader);
    }

private:
    std::shared_ptr<CardGroupWidget> addCards()
    {
        std::shared_ptr<CardGroupWidget> widget { new CardGroupWidget(this) };

        widget->addCard(kWidgetPresetA, "SLAPBACK", "Short single echo");
        widget->addCard(kWidgetPresetB, "PING PONG", "Echoes across the stereo field");
        widget->addCard(kWidgetPresetC, "TAPE ECHO", "Warm, wobbling repeats");
        widget->addCard(kWidgetPresetD, "AMBIENT WASH", "Long, dark trails");
        widget->done();

        Layout::widgets.push_back({ widget.get(), Fixed });
        return widget;
    }

    // the Expert page in Expert mode, the cards below a spacer in Easy mode, neither on the other pages.
    // The scope is hidden along with the page, so it hides its shader too.
    void showPage(const Page page)
    {
        const bool expert = page == kPageExpert;

        fPage = page;
        fExpert->setVisible(expert);
        fExpert->getScope()->setVisible(expert);
        fSpacer->setVisible(! expert);
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
};

// --------------------------------------------------------------------------------------------------------------------

class DelayMainAreaWidget final : public MainAreaContainerWidget<DelayStageWidget>
{
public:
    explicit DelayMainAreaWidget(LabTopLevelWidget* const parent)
        : MainAreaContainerWidget(parent) {}

    void setScopeShader(BotShaderBaseWidget* const shader)
    {
        fStage->setScopeShader(shader);
    }
};

class DelayRootWidget final : public RootWidget<TopBar, DelayMainAreaWidget>
{
public:
    DelayRootWidget(Window& window, LabUIWidgetInterface* const iface)
        : RootWidget(window, iface) {}

    void setScopeShader(BotShaderBaseWidget* const shader)
    {
        fMainArea->setScopeShader(shader);
    }
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

class DelayUI final : public LibreAudioBaseUI
{
    const std::unique_ptr<LibreAudio::ShaderBaseWidget> fShaderBackground {
        new LibreAudio::BotShaderWidget<SHADERS_SHADERTOY_CLOUDSTARFIELD_FRAG_DATA,
                                        SHADERS_SHADERTOY_CLOUDSTARFIELD_FRAG_LEN>(this, this)
    };

    // the scope, above the starfield; the display places it
    const std::unique_ptr<LibreAudio::BotShaderBaseWidget> fShaderScope {
        new LibreAudio::BotShaderWidget<SHADERS_CURVE_DELAY_FRAG_DATA, SHADERS_CURVE_DELAY_FRAG_LEN>(this, this)
    };

public:
    DelayUI()
        : LibreAudioBaseUI()
    {
        createRootWidget<LibreAudio::DelayRootWidget>();

        fRootWidget->enableShaders({ fShaderBackground.get() });
        static_cast<LibreAudio::DelayRootWidget*>(fRootWidget.get())->setScopeShader(fShaderScope.get());
    }

private:
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DelayUI)
};

UI* createUI()
{
    return new DelayUI();
}

END_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------
