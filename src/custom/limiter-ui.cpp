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

// --------------------------------------------------------------------------------------------------------------------

namespace LibreAudio {

// height of the number boxes, taller than the reference one, as in voiceIsolation
static constexpr const uint kBoxHeight = 45;

// two dropdowns or checkboxes stack in the height of a number box, this far apart
static constexpr const uint kRowGap = 5;
static constexpr const uint kRowHeight = (kBoxHeight - kRowGap) / 2;

// --------------------------------------------------------------------------------------------------------------------
// The stage is the display, with the starfield, the input waveform and the gain reduction behind it and the level
// grid and threshold line over them, and a row of control groups floats at its bottom.
// Each group is a panel styled like the mbComp5 band groups.

struct LimiterGroupReference : Reference::Zero {
    static constexpr const Color backgroundColor { 0x30, 0x32, 0x3c, 0.7f };
    static constexpr const Color borderColor = backgroundColor;
    static constexpr const Color borderHighlightColor { 0x40, 0x42, 0x4c, 0.8f };
    static constexpr const Color borderShadowColor { 0, 0, 0, 0.12f };
    static constexpr const uint border = 1;
    static constexpr const uint borderRadius = 8;
    static constexpr const uint margin = 8;
    static constexpr const uint padding = 6;
    static constexpr const uint height = kBoxHeight + margin * 2;

    // TESTING
    static constexpr const Color shadowColor { 0, 0, 0, 0.6f };
    static constexpr const Point<int> shadowOffset = { 0, 5 };
    // static constexpr const Size<int> shadowSize = { -6, -6 };
    static constexpr const Size<int> shadowSize = { -12, -12 };
    static constexpr const uint shadowFeather = 14;
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
using LimiterNumberBoxWidget = NumberBoxWidget<accentColor<id>(), kBoxHeight>;

// sets a parameter as one gesture, the way a click on a control does

static void setParameterGesture(LabUIWidgetInterface* const iface, const uint32_t id, const float value)
{
    iface->parameterControlPressed(id);
    iface->parameterControlModified(id, value);
    iface->parameterControlReleased(id);
}

// --------------------------------------------------------------------------------------------------------------------
// The control groups' menu: a list of a parameter's options above its dropdown. It is an overlay covering the whole
// display while open, so it draws over everything and takes every click: one on an option picks it, any other closes
// the menu.

class LimiterMenuWidget final : public LabWidget
{
    static constexpr const float kRowHeight = 20.f;
    static constexpr const float kPadding = 3.f;
    static constexpr const float kTextMargin = 10.f;
    static constexpr const float kFontSize = 11.f;

    FaustParameterIndex fId = kFaustParameterCount;
    Color fAccent;
    Rectangle<float> fBox;
    int fHoverRow = -1;

public:
    explicit LimiterMenuWidget(LabWidget* const parent)
        : LabWidget(parent)
    {
        hide();
    }

    // anchor is the dropdown's box, in absolute coordinates; the menu opens above it, aligned to its right edge
    void open(const FaustParameterIndex id, const Rectangle<float>& anchor, const Color& accent)
    {
        const FaustParameter& param = kFaustParameters[id];
        const float s = fScaleFactor;

        fId = id;
        fAccent = accent;
        fHoverRow = -1;

        fontFace("regular");
        fontSize(kFontSize * s);
        textAlign(0);

        float width = anchor.getWidth();
        for (uint i = 0; i < param.scalePointCount; ++i)
        {
            Rectangle<float> bounds;
            textBounds(0, 0, param.scalePoints[i].label, nullptr, bounds);
            width = std::max(width, bounds.getWidth() + kTextMargin * 2 * s);
        }

        const float height = (param.scalePointCount * kRowHeight + kPadding * 2) * s;
        const float right = anchor.getX() + anchor.getWidth() - getAbsoluteX();
        const float top = anchor.getY() - getAbsoluteY() - 4.f * s - height;

        fBox = Rectangle<float>(right - width, std::max(4.f * s, top), width, height);

        show();
        toFront();
        repaint();
    }

private:
    [[nodiscard]] int rowAt(const Point<double>& pos) const noexcept
    {
        if (! fBox.contains(static_cast<float>(pos.getX()), static_cast<float>(pos.getY())))
            return -1;

        const int row = static_cast<int>((pos.getY() - fBox.getY() - kPadding * fScaleFactor) / (kRowHeight * fScaleFactor));
        return row >= 0 && row < static_cast<int>(kFaustParameters[fId].scalePointCount) ? row : -1;
    }

    void close()
    {
        hide();
        repaint();
    }

    void onNanoDisplay() final
    {
        const FaustParameter& param = kFaustParameters[fId];
        const float s = fScaleFactor;
        const float r = 6.f * s;
        const int current = d_roundToInt(fInterface->getParameterValue(kParametersMainStart + fId));

        beginPath();
        roundedRect(fBox.getX(), fBox.getY() + 3.f * s, fBox.getWidth(), fBox.getHeight(), r);
        fillColor(Color(0.f, 0.f, 0.f, 0.35f));
        fill();

        beginPath();
        roundedRect(fBox.getX(), fBox.getY(), fBox.getWidth(), fBox.getHeight(), r);
        fillColor(Color(0x1e, 0x1f, 0x24, 0.96f));
        fill();
        strokeColor(Color(0.f, 0.f, 0.f, 0.5f));
        strokeWidth(1.f);
        stroke();

        fontFace("regular");
        fontSize(kFontSize * s);
        textAlign(ALIGN_LEFT | ALIGN_MIDDLE);

        for (uint i = 0; i < param.scalePointCount; ++i)
        {
            const float y = fBox.getY() + (kPadding + i * kRowHeight) * s;

            if (static_cast<int>(i) == fHoverRow)
            {
                beginPath();
                roundedRect(fBox.getX() + kPadding * s, y, fBox.getWidth() - kPadding * 2 * s, kRowHeight * s, 4.f * s);
                fillColor(Color(1.f, 1.f, 1.f, 0.08f));
                fill();
            }

            fillColor(param.scalePoints[i].value == current ? fAccent : Reference::Colors::ink2);
            text(fBox.getX() + kTextMargin * s, y + kRowHeight * 0.5f * s, param.scalePoints[i].label, nullptr);
        }
    }

    bool onMouse(const Widget::MouseEvent& ev) final
    {
        if (ev.press)
        {
            if (const int row = rowAt(ev.pos); row >= 0 && ev.button == 1)
                setParameterGesture(fInterface, kParametersMainStart + fId, kFaustParameters[fId].scalePoints[row].value);

            close();
        }
        return true;
    }

    bool onMotion(const Widget::MotionEvent& ev) final
    {
        if (const int row = rowAt(ev.pos); row != fHoverRow)
        {
            fHoverRow = row;
            repaint();
        }
        return true;
    }

    bool onScroll(const Widget::ScrollEvent&) final
    {
        return true;
    }
};

// --------------------------------------------------------------------------------------------------------------------
// A slim dropdown for a parameter with options: its name in the accent colour, then a small box with the current option in the
// accent colour and a chevron. A click on the box opens the menu, scrolling over it steps through the options.

class LimiterDropdownWidget final : public LabWidget
{
    static constexpr const float kBoxWidth = 64.f;
    static constexpr const float kGap = 6.f;
    static constexpr const float kFontSize = Reference::Widgets::NumberBox<>::Name::fontSize;
    const FaustParameterIndex fId;
    const FaustParameter& fParameter;
    const Color fAccent;
    bool fHover = false;

public:
    explicit LimiterDropdownWidget(LabWidget* const parent, const FaustParameterIndex id)
        : LabWidget(parent),
          fId(id),
          fParameter(kFaustParameters[id]),
          fAccent(Reference::kAccentColors[std::clamp(kFaustParameters[id].accentcolor, 1u, 6u) - 1])
    {
        updateSize(false);
    }

private:
    [[nodiscard]] float boxX() const noexcept
    {
        return getWidth() - kBoxWidth * fScaleFactor;
    }

    [[nodiscard]] int currentIndex() const noexcept
    {
        const int value = d_roundToInt(fInterface->getParameterValue(kParametersMainStart + fId));
        for (uint i = 0; i < fParameter.scalePointCount; ++i)
            if (fParameter.scalePoints[i].value == value)
                return static_cast<int>(i);
        return 0;
    }

    void setupFont()
    {
        fontFace("regular");
        fontSize(kFontSize * fScaleFactor);
        textLetterSpacing(Reference::Widgets::NumberBox<>::Name::letterSpacing * fScaleFactor);
    }

    // defined after the stage, which owns the menu
    void openMenu();

    void onNanoDisplay() final
    {
        const float s = fScaleFactor;
        const float h = getHeight();
        const float bx = boxX();

        setupFont();
        fillColor(fAccent);
        textAlign(ALIGN_RIGHT | ALIGN_MIDDLE);
        text(bx - kGap * s, h * 0.5f, fParameter.name, nullptr);
        textLetterSpacing(0.f);

        beginPath();
        roundedRect(bx + 0.5f, 0.5f, kBoxWidth * s - 1.f, h - 1.f, 4.f * s);
        fillColor(Reference::Colors::track);
        fill();
        strokeColor(Color(1.f, 1.f, 1.f, fHover ? 0.15f : 0.06f));
        strokeWidth(1.f);
        stroke();

        fillColor(fAccent);
        textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
        text(bx + 6.f * s, h * 0.5f, fParameter.scalePoints[currentIndex()].label, nullptr);

        const float cx = bx + (kBoxWidth - 9.f) * s;
        const float cy = h * 0.5f;
        beginPath();
        moveTo(cx - 3.f * s, cy - 1.5f * s);
        lineTo(cx, cy + 1.5f * s);
        lineTo(cx + 3.f * s, cy - 1.5f * s);
        lineCap(ROUND);
        lineJoin(ROUND);
        strokeColor(Reference::Colors::ink3);
        strokeWidth(1.2f * s);
        stroke();
    }

    bool onMouse(const Widget::MouseEvent& ev) final
    {
        if (ev.press && ev.button == 1 && contains(ev.pos) && ev.pos.getX() >= boxX())
        {
            openMenu();
            return true;
        }
        return LabWidget::onMouse(ev);
    }

    bool onMotion(const Widget::MotionEvent& ev) final
    {
        if (const bool hover = contains(ev.pos) && ev.pos.getX() >= boxX(); hover != fHover)
        {
            fHover = hover;
            repaint();
        }
        return LabWidget::onMotion(ev);
    }

    bool onScroll(const Widget::ScrollEvent& ev) final
    {
        if (! contains(ev.pos) || ev.pos.getX() < boxX() || d_isZero(ev.delta.getY()))
            return LabWidget::onScroll(ev);

        const int count = static_cast<int>(fParameter.scalePointCount);
        const int index = std::clamp(currentIndex() + (ev.delta.getY() > 0 ? -1 : 1), 0, count - 1);
        setParameterGesture(fInterface, kParametersMainStart + fId, fParameter.scalePoints[index].value);
        repaint();
        return true;
    }

    // as wide as its name and box need, or wider when the stack makes it so; the box stays at the right edge
    void updateSize(const bool updateChildren) final
    {
        Rectangle<float> bounds;
        setupFont();
        textAlign(0);
        textBounds(0, 0, fParameter.name, nullptr, bounds);
        textLetterSpacing(0.f);

        const uint width = d_roundToUnsignedInt(bounds.getWidth() + (kGap + kBoxWidth) * fScaleFactor);
        setSize(std::max(width, getWidth()), d_roundToUnsignedInt(kRowHeight * fScaleFactor));

        LabWidget::updateSize(updateChildren);
    }
};

// --------------------------------------------------------------------------------------------------------------------
// A slim checkbox for an on / off parameter, styled like the dropdown: a square box as tall as the dropdown's at the
// left edge, with a check mark in the accent colour when on, then its name in the accent colour. A click anywhere on it toggles
// the parameter.

class LimiterCheckboxWidget final : public LabWidget
{
    static constexpr const float kGap = 6.f;
    static constexpr const float kFontSize = Reference::Widgets::NumberBox<>::Name::fontSize;

    const FaustParameterIndex fId;
    const FaustParameter& fParameter;
    const Color fAccent;
    bool fHover = false;

public:
    explicit LimiterCheckboxWidget(LabWidget* const parent, const FaustParameterIndex id)
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

    // as wide as its box and name need, or wider when the stack makes it so
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
// Two controls stacked in the height of a number box, kRowGap apart, the stack as wide as the wider one.

struct LimiterStackReference : Reference::Zero {
    static constexpr const uint padding = kRowGap;
};

template<class Top, FaustParameterIndex kTop, class Bottom, FaustParameterIndex kBottom>
class LimiterStackWidget final : public ReferenceContainerWidget<LimiterStackReference, kVertical>
{
    using BaseWidget = ReferenceContainerWidget<LimiterStackReference, kVertical>;

    const std::array<std::shared_ptr<LabWidget>, 2> widgets {
        addWidget<Top>(kTop),
        addWidget<Bottom>(kBottom),
    };

public:
    explicit LimiterStackWidget(LabWidget* const parent)
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

        LabWidget::setSize(width, d_roundToUnsignedInt(kBoxHeight * fScaleFactor));

        BaseWidget::updateSize(updateChildren);
    }
};

using LimiterTogglesWidget = LimiterStackWidget<LimiterCheckboxWidget, kFaustParameterTrue_peak,
                                                LimiterCheckboxWidget, kFaustParameterUnity_gain>;

using LimiterDitherWidget = LimiterStackWidget<LimiterDropdownWidget, kFaustParameterDither,
                                               LimiterDropdownWidget, kFaustParameterNoise_shaping>;

// --------------------------------------------------------------------------------------------------------------------

// A floating panel holding one group of controls: number boxes, which share whatever width the row gives the
// group, or a control of its own width.

class LimiterGroupWidget : public ReferenceContainerWidget<LimiterGroupReference>
{
    using R = LimiterGroupReference;
    using BaseWidget = ReferenceContainerWidget<R>;

    const uint fBoxCount;

public:
    LimiterGroupWidget(LabWidget* const parent, const uint boxCount)
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

template<FaustParameterIndex... kIds>
class LimiterBoxGroupWidget final : public LimiterGroupWidget
{
    const std::array<std::shared_ptr<LabWidget>, sizeof...(kIds)> widgets {
        addWidget<LimiterNumberBoxWidget<kIds>, Expanding>(kIds)...
    };

public:
    explicit LimiterBoxGroupWidget(LabWidget* const parent)
        : LimiterGroupWidget(parent, sizeof...(kIds)) {}
};

template<class W>
class LimiterControlGroupWidget final : public LimiterGroupWidget
{
    const std::array<std::shared_ptr<LabWidget>, 1> widgets {
        addWidget<W>(),
    };

public:
    explicit LimiterControlGroupWidget(LabWidget* const parent)
        : LimiterGroupWidget(parent, 0) {}
};

// --------------------------------------------------------------------------------------------------------------------
// The row of groups along the bottom of the display, a fixed gap apart. Every number box gets the same width, from
// what the row has left once the groups' fixed parts and the gaps are taken out.

struct LimiterGroupRowReference : Reference::Zero {
    static constexpr const uint padding = 8;
    static constexpr const uint height = LimiterGroupReference::height;
};

class LimiterGroupRowWidget final : public ReferenceContainerWidget<LimiterGroupRowReference>
{
    using R = LimiterGroupRowReference;
    using BaseWidget = ReferenceContainerWidget<R>;

    const std::array<std::shared_ptr<LimiterGroupWidget>, 5> fGroups {
        addWidget<LimiterControlGroupWidget<LimiterTogglesWidget>>(),
        addWidget<LimiterBoxGroupWidget<kFaustParameterThreshold, kFaustParameterCeiling>>(),
        addWidget<LimiterBoxGroupWidget<kFaustParameterAttack, kFaustParameterAttack_shape>>(),
        addWidget<LimiterBoxGroupWidget<kFaustParameterRelease, kFaustParameterAuto_release>>(),
        addWidget<LimiterControlGroupWidget<LimiterDitherWidget>>(),
    };

public:
    explicit LimiterGroupRowWidget(LabWidget* const parent)
        : BaseWidget(parent) {}

private:
    void sizeGroups()
    {
        const int gap = d_roundToIntPositive(R::padding * fScaleFactor);
        int available = static_cast<int>(getWidth()) - gap * static_cast<int>(fGroups.size() - 1);
        uint boxes = 0;

        for (const std::shared_ptr<LimiterGroupWidget>& group : fGroups)
        {
            available -= static_cast<int>(group->getFixedWidth());
            boxes += group->getBoxCount();
        }

        const uint boxWidth = boxes != 0 ? static_cast<uint>(std::max(0, available) / static_cast<int>(boxes)) : 0;

        for (const std::shared_ptr<LimiterGroupWidget>& group : fGroups)
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

// The display's level scale, shared with the waveform shaders: 0 dB at its top edge and the lowest grid line,
// kDisplayDbFloor, a little above the control groups, so the dB at its bottom edge follows the display's height.

static constexpr const float kDisplayDbMax = 0.f;
static constexpr const float kDisplayDbFloor = -24.f;
static constexpr const float kDisplayDbStep = 6.f;

// The display's time scale: seconds of waveform across its width, now at the right edge, the oldest 256 points left
// out as MARGIN in waveform-scroll.frag; and the time grid's spacing.
static constexpr const float kDisplaySeconds = kNumSecondsForWaveform * (1.f - 256.f / kNumSamplePointsForWaveform);
static constexpr const float kDisplayTimeStep = 1.f;

// from the display's bottom edge up to the lowest grid line, in px at scale 1: the groups with the display's padding
// below and above them
static constexpr const uint kDisplayFloorOffset = Reference::Stage::padding * 2 + LimiterGroupRowReference::height;

// dB at the bottom edge of a display this tall
static float displayDbMin(const float height, const float scaleFactor) noexcept
{
    const float floorY = std::max(1.f, height - kDisplayFloorOffset * scaleFactor);
    return kDisplayDbMax + (kDisplayDbFloor - kDisplayDbMax) * height / floorY;
}

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
// over them. The threshold line is dragged up and down to set the Threshold. Along the bottom it shows the row of
// control groups in Expert mode and four preset cards in Easy mode; the cards are the suite's Easy page cards and do
// not load anything yet.

class LimiterStageWidget final : public ReferenceContainerWidget<Reference::Stage, kVertical>,
                                 private IdleCallback
{
    using R = Reference::Stage;
    using BaseWidget = ReferenceContainerWidget<R, kVertical>;

    static constexpr const uint32_t kThresholdId = kParametersMainStart + kFaustParameterThreshold;

    // how close to the threshold line, in px at scale 1, a press or hover catches it
    static constexpr const float kGrabDistance = 6.f;

    const std::shared_ptr<LabWidget> fSpacer = addSpacer();
    const std::shared_ptr<LimiterGroupRowWidget> fGroups = addWidget<LimiterGroupRowWidget>();
    const std::shared_ptr<CardGroupWidget> fCards = addCards();

    // created after the groups, so it draws over them and gets events first
    const std::unique_ptr<LimiterMenuWidget> fMenu { new LimiterMenuWidget(this) };

    Page fPage = kPageEasy;
    bool fHover = false;
    bool fDragging = false;
    float fDragOffset = 0.f;

public:
    explicit LimiterStageWidget(LabWidget* const parent)
        : BaseWidget(parent)
    {
        showPage(getCurrentPage(fInterface));
        addIdleCallback(this);
    }

    void openMenu(const FaustParameterIndex id, const Rectangle<float>& anchor, const Color& accent)
    {
        fMenu->open(id, anchor, accent);
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
        fMenu->hide();
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

    void updateSize(const bool updateChildren) final
    {
        BaseWidget::updateSize(updateChildren);

        fMenu->setAbsolutePos(getAbsolutePos());
        fMenu->setSize(getSize());
    }

    [[nodiscard]] float yOfDb(const float db) const noexcept
    {
        return (kDisplayDbMax - db) / (kDisplayDbMax - displayDbMin(getHeight(), fScaleFactor)) * getHeight();
    }

    [[nodiscard]] float dbAtY(const float y) const noexcept
    {
        return kDisplayDbMax - y / getHeight() * (kDisplayDbMax - displayDbMin(getHeight(), fScaleFactor));
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

        // level grid and dB ticks below the top edge, which is 0 dB, down to the floor above the groups.
        // Each label sits on its line, and the line fades out just before reaching it.
        fontFace("mono");
        fontSize(10.f * s);
        textAlign(ALIGN_LEFT | ALIGN_MIDDLE);

        const Color gridColor(Reference::Colors::line2, 0.3f);
        const float floorY = std::round(yOfDb(kDisplayDbFloor)) + 0.5f;

        // time grid, a line per kDisplayTimeStep back from now at the right edge, down to the floor
        for (float t = kDisplayTimeStep; t < kDisplaySeconds; t += kDisplayTimeStep)
        {
            const float x = std::round(w * (1.f - t / kDisplaySeconds)) + 0.5f;

            beginPath();
            moveTo(x, border);
            lineTo(x, floorY);
            strokeColor(gridColor);
            strokeWidth(1.f);
            stroke();
        }

        for (float db = kDisplayDbMax - kDisplayDbStep; db >= kDisplayDbFloor; db -= kDisplayDbStep)
        {
            const float y = std::round(yOfDb(db)) + 0.5f;

            char label[8];
            std::snprintf(label, sizeof(label), "%d", static_cast<int>(db));
            fillColor(Reference::Colors::ink3);
            const float labelEnd = text(6.f * s, y, label, nullptr);

            const float fadeStart = labelEnd + 4.f * s;
            const float fadeEnd = fadeStart + 16.f * s;

            beginPath();
            moveTo(fadeStart, y);
            lineTo(w - border, y);
            strokePaint(linearGradient(fadeStart, y, fadeEnd, y, gridColor.withAlpha(0.f), gridColor));
            strokeWidth(1.f);
            stroke();
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

void LimiterDropdownWidget::openMenu()
{
    const Rectangle<float> box(getAbsoluteX() + boxX(), getAbsoluteY(), kBoxWidth * fScaleFactor, getHeight());

    for (Widget* widget = getParentWidget(); widget != nullptr;)
    {
        if (LimiterStageWidget* const stage = dynamic_cast<LimiterStageWidget*>(widget))
            return stage->openMenu(fId, box, fAccent);

        SubWidget* const sub = dynamic_cast<SubWidget*>(widget);
        widget = sub != nullptr ? sub->getParentWidget() : nullptr;
    }
}

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
            shader->setCustomUniform("uDbMin", [this, shader] {
                const LabUIWidgetInterface* const iface = this;
                return LibreAudio::displayDbMin(shader->getHeight(), iface->getScaleFactor());
            });
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

        const double dt = fLastTime != 0 ? std::min<double>(now - fLastTime, 0.1) : 0;
        fLastTime = now;

        fPointsShown += dt * kWaveformRate;

        const double error = fPointsPushed - kWaveformLag - fPointsShown;

        if (std::abs(error) > kWaveformRate * 0.5)
            fPointsShown += error;
        else
            fPointsShown += error * std::min<double>(1.0, dt * 2);

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
