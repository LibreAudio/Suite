// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "LibreAudioBaseUI.hpp"

#include "ui/containers/main-area.hpp"
#include "ui/containers/top-bar.hpp"
#include "ui/widgets/number-box.hpp"
#include "ui/widgets/pill-toggle.hpp"
#include "ui/widgets/shader.hpp"

#include "LibreAudioParameters.hpp"

#include "las-resources.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <tuple>

// --------------------------------------------------------------------------------------------------------------------

namespace LibreAudio {

// height of the number boxes, taller than the reference one, as in voiceIsolation
static constexpr const uint kBoxHeight = 45;

// height of the checkboxes, as in the limiter
static constexpr const uint kRowHeight = 20;

// width of the number boxes, the same in every panel
static constexpr const uint kBoxWidth = 110;

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
// A slim checkbox, as in the limiter: a square box at the left edge, with a check mark in the accent colour when on,
// then its name in the accent colour. It is on while the parameter equals its on value; a click anywhere on it sets
// the parameter to that value, or back to 0 when already on. Checkboxes sharing a parameter with different on values
// are therefore exclusive.

class DelayCheckboxWidget : public LabWidget
{
    static constexpr const float kGap = 6.f;
    static constexpr const float kFontSize = Reference::Widgets::NumberBox<>::Name::fontSize;

    const FaustParameterIndex fId;
    const FaustParameter& fParameter;
    const int fOnValue;
    const char* const fLabel;
    const Color fAccent;
    bool fHover = false;

public:
    // labelled with the parameter's name, on at 1
    explicit DelayCheckboxWidget(LabWidget* const parent, const FaustParameterIndex id)
        : DelayCheckboxWidget(parent, id, 1, kFaustParameters[id].name) {}

protected:
    DelayCheckboxWidget(LabWidget* const parent, const FaustParameterIndex id, const int onValue, const char* const label)
        : LabWidget(parent),
          fId(id),
          fParameter(kFaustParameters[id]),
          fOnValue(onValue),
          fLabel(label),
          fAccent(Reference::kAccentColors[std::clamp(kFaustParameters[id].accentcolor, 1u, 6u) - 1])
    {
        updateSize(false);
    }

    // the label of a parameter's choice, or its name when it has no label for that value
    static const char* choiceLabel(const FaustParameterIndex id, const int value) noexcept
    {
        const FaustParameter& parameter = kFaustParameters[id];
        for (uint i = 0; i < parameter.scalePointCount; ++i)
            if (parameter.scalePoints[i].value == value)
                return parameter.scalePoints[i].label;
        return parameter.name;
    }

private:
    [[nodiscard]] float boxSize() const noexcept
    {
        return kRowHeight * fScaleFactor;
    }

    [[nodiscard]] bool isChecked() const noexcept
    {
        return std::lround(fInterface->getParameterValue(kParametersMainStart + fId)) == fOnValue;
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
        text(size + kGap * s, h * 0.5f, fLabel, nullptr);
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
            setParameterGesture(fInterface, kParametersMainStart + fId, isChecked() ? 0.f : static_cast<float>(fOnValue));
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

    // as wide and tall as its box and name need, or larger when its cell makes it so; drawn centred vertically
    void updateSize(const bool updateChildren) final
    {
        Rectangle<float> bounds;
        setupFont();
        textAlign(0);
        textBounds(0, 0, fLabel, nullptr, bounds);
        textLetterSpacing(0.f);

        const uint width = d_roundToUnsignedInt(bounds.getWidth() + kGap * fScaleFactor + boxSize());
        setSize(std::max(width, getWidth()), std::max(d_roundToUnsignedInt(kRowHeight * fScaleFactor), getHeight()));

        LabWidget::updateSize(updateChildren);
    }
};

// --------------------------------------------------------------------------------------------------------------------
// A checkbox for one choice of a parameter, on while the parameter is kValue and labelled with that choice.

template<int kValue>
class DelayChoiceCheckboxWidget final : public DelayCheckboxWidget
{
public:
    explicit DelayChoiceCheckboxWidget(LabWidget* const parent, const FaustParameterIndex id)
        : DelayCheckboxWidget(parent, id, kValue, choiceLabel(id, kValue)) {}
};

// A cell of a panel row: the control widget for one parameter.

template<template<FaustParameterIndex> class Control, FaustParameterIndex kId>
struct DelayCell
{
    using Widget = Control<kId>;
    static constexpr const FaustParameterIndex id = kId;
    static constexpr const int sync = -1;
};

template<FaustParameterIndex>
using DelayCheckboxCellWidget = DelayCheckboxWidget;

template<FaustParameterIndex kId>
using DelayBox = DelayCell<DelayNumberBoxWidget, kId>;

// A number box greyed out while Link is on, when the left channel's value drives it; it stays adjustable.

template<FaustParameterIndex kId>
class DelayLinkedNumberBoxWidget final : public DelayNumberBoxWidget<kId>
{
public:
    explicit DelayLinkedNumberBoxWidget(LabWidget* const parent, const FaustParameterIndex id)
        : DelayNumberBoxWidget<kId>(parent, id)
    {
        update();
    }

private:
    void idleCallback() final
    {
        DelayNumberBoxWidget<kId>::idleCallback();
        update();
    }

    void update()
    {
        const bool link = d_isNotZero(this->fInterface->getParameterValue(kParametersMainStart + kFaustParameterLink));

        if (this->isEnabled() == link)
        {
            this->setEnabled(! link, false);
            this->repaint();
        }
    }
};

template<FaustParameterIndex kId>
using DelayLinkedBox = DelayCell<DelayLinkedNumberBoxWidget, kId>;

// A small triangle in the top right corner of a number box for a parameter with options. The widget covers the upper
// half of the box, and a click anywhere on it opens the stage's menu of the options below or above the box. Motion
// passes through it, so the lower half of the box under it still drags.

class DelayMenuArrowWidget final : public LabWidget
{
    const FaustParameterIndex fId;
    const Color fAccent;
    bool fHover = false;

public:
    // the square in the box's corner the triangle is centred in, in px at scale 1
    static constexpr const uint size = 18;

    explicit DelayMenuArrowWidget(LabWidget* const parent, const FaustParameterIndex id)
        : LabWidget(parent),
          fId(id),
          fAccent(Reference::kAccentColors[std::clamp(kFaustParameters[id].accentcolor, 1u, 6u) - 1])
    {
        updateSize(false);
    }

private:
    // defined after the stage, which owns the menu
    void openMenu();

    void onNanoDisplay() final
    {
        const float s = fScaleFactor;
        const float cx = getWidth() - size * 0.5f * s;
        const float cy = size * 0.5f * s;

        beginPath();
        moveTo(cx - 4.5f * s, cy - 2.5f * s);
        lineTo(cx + 4.5f * s, cy - 2.5f * s);
        lineTo(cx, cy + 2.5f * s);
        closePath();
        fillColor(fHover ? fAccent : Reference::Colors::ink3);
        fill();
    }

    bool onMouse(const Widget::MouseEvent& ev) final
    {
        if (ev.press && ev.button == 1 && contains(ev.pos))
        {
            openMenu();
            return true;
        }
        return false;
    }

    bool onMotion(const Widget::MotionEvent& ev) final
    {
        if (const bool hover = contains(ev.pos); hover != fHover)
        {
            fHover = hover;
            repaint();
        }
        return false;
    }

    // sized and placed by the box it sits in
    void updateSize(const bool updateChildren) final
    {
        LabWidget::updateSize(updateChildren);
    }
};

// A number box of kBox kind with a menu arrow over its upper half. The arrow is added after the box so it gets clicks
// first.

template<template<FaustParameterIndex> class kBox, FaustParameterIndex kId>
class DelayMenuNumberBoxWidget final : public LabWidget
{
    const std::unique_ptr<LabWidget> fBox;
    const std::unique_ptr<LabWidget> fArrow;

public:
    explicit DelayMenuNumberBoxWidget(LabWidget* const parent, const FaustParameterIndex id)
        : LabWidget(parent),
          fBox(new kBox<kId>(this, id)),
          fArrow(new DelayMenuArrowWidget(this, id))
    {
        updateSize(false);
    }

private:
    void onNanoDisplay() final {}

    void updateSize(const bool updateChildren) final
    {
        setSize(std::max(1u, getWidth()), d_roundToUnsignedInt(kBoxHeight * fScaleFactor));

        fBox->setAbsolutePos(getAbsolutePos());
        fBox->setWidth(getWidth());
        fArrow->setAbsolutePos(getAbsolutePos());
        fArrow->setSize(getWidth(), getHeight() / 2);

        LabWidget::updateSize(updateChildren);
    }
};

template<FaustParameterIndex kId>
using DelayMenuNumberBox = DelayMenuNumberBoxWidget<DelayNumberBoxWidget, kId>;

template<FaustParameterIndex kId>
using DelayLinkedMenuNumberBox = DelayMenuNumberBoxWidget<DelayLinkedNumberBoxWidget, kId>;

template<FaustParameterIndex kId>
using DelayMenuBox = DelayCell<DelayMenuNumberBox, kId>;

template<FaustParameterIndex kId>
using DelayLinkedMenuBox = DelayCell<DelayLinkedMenuNumberBox, kId>;

// A number box with a gain reduction meter, as in the reverb: an orange bar along its top edge growing leftward from
// the right as the reduction reported by the kMeterId output parameter grows.

template<FaustParameterIndex kId, FaustParameterIndex kMeterId>
class DelayMeterNumberBoxWidget final : public DelayNumberBoxWidget<kId>
{
    using BaseWidget = DelayNumberBoxWidget<kId>;
    using R = Reference::Widgets::NumberBox<>;

    float fMeterValue = 0.f;

public:
    explicit DelayMeterNumberBoxWidget(LabWidget* const parent, const FaustParameterIndex id)
        : BaseWidget(parent, id) {}

private:
    void onNanoDisplay() final
    {
        BaseWidget::onNanoDisplay();

        const FaustParameter& meter = kFaustParameters[kMeterId];
        const float s = this->fScaleFactor;
        const float w = this->getWidth();
        const float h = this->getHeight();
        const float bh = R::Bar::height * s;
        const float r = R::borderRadius * s;
        const float reduction = std::clamp((fMeterValue - meter.min) / (meter.max - meter.min), 0.f, 1.f);
        const float x = (1.f - reduction) * w;

        this->save();

        this->scissor(0, 0, w, bh);
        this->beginPath();
        this->roundedRect(0, 0, w, h, r);
        this->fillColor(R::Bar::color〡deactivated);
        this->fill();

        this->scissor(x, 0, w - x, bh);
        this->beginPath();
        this->roundedRect(0, 0, w, h, r);
        this->fillColor(Reference::Colors::orange);
        this->fill();

        this->restore();
    }

    // repaint whenever the meter moves
    void idleCallback() final
    {
        BaseWidget::idleCallback();

        if (const float value = this->fInterface->getParameterValue(kParametersMainStart + kMeterId);
            d_isNotEqual(value, fMeterValue))
        {
            fMeterValue = value;
            this->repaint();
        }
    }
};

template<FaustParameterIndex kId, FaustParameterIndex kMeterId>
struct DelayMeterBox
{
    using Widget = DelayMeterNumberBoxWidget<kId, kMeterId>;
    static constexpr const FaustParameterIndex id = kId;
    static constexpr const int sync = -1;
};

template<FaustParameterIndex kId>
using DelayCheckbox = DelayCell<DelayCheckboxCellWidget, kId>;

template<FaustParameterIndex kId, int kValue>
struct DelayChoiceCheckbox
{
    using Widget = DelayChoiceCheckboxWidget<kValue>;
    static constexpr const FaustParameterIndex id = kId;
    static constexpr const int sync = -1;
};

// A row of a panel: one control per cell, the panel's padding apart, sharing its width.

// A cell shown only while Sync is off (kSync 0) or only while it is on (kSync 1). Cells that always show have a sync
// of -1.

template<class Cell, int kSync>
struct DelaySyncCell : Cell
{
    static constexpr const int sync = kSync;
};

template<class Cell>
using DelayFreeOnly = DelaySyncCell<Cell, 0>;

template<class Cell>
using DelaySyncOnly = DelaySyncCell<Cell, 1>;

template<uint kHeight, class... Cells>
class DelayPanelRowWidget final
    : public ReferenceContainerWidget<Reference::Height<kHeight, Reference::Padding<DelayGroupReference::padding>>>,
      private IdleCallback
{
    using BaseWidget = ReferenceContainerWidget<Reference::Height<kHeight, Reference::Padding<DelayGroupReference::padding>>>;

    static constexpr const std::array<int, sizeof...(Cells)> kSync { Cells::sync... };
    static constexpr const bool kFollowsSync = ((Cells::sync != -1) || ...);

    const std::array<std::shared_ptr<LabWidget>, sizeof...(Cells)> widgets {
        this->template addWidget<typename Cells::Widget, Expanding>(Cells::id)...
    };
    bool fSync = false;

public:
    static constexpr const uint height = kHeight;
    static constexpr const FaustParameterIndex firstId = std::get<0>(std::make_tuple(Cells::id...));

    explicit DelayPanelRowWidget(LabWidget* const parent)
        : BaseWidget(parent)
    {
        if constexpr (kFollowsSync)
        {
            update(true);
            this->addIdleCallback(this);
        }
    }

private:
    void idleCallback() final
    {
        update(false);
    }

    // shows the cells that belong to the current Sync state, and lays the row out again when it changes
    void update(const bool init)
    {
        const bool sync = d_isNotZero(this->fInterface->getParameterValue(kParametersMainStart + kFaustParameterSync));

        if (! init && sync == fSync)
            return;

        fSync = sync;

        for (size_t i = 0; i < widgets.size(); ++i)
            if (kSync[i] != -1)
                widgets[i]->setVisible((kSync[i] == 1) == sync);

        if (! init)
        {
            this->updateSize(true);
            this->repaint();
        }
    }
};

template<FaustParameterIndex... kIds>
using DelayBoxRowWidget = DelayPanelRowWidget<kBoxHeight, DelayBox<kIds>...>;

// --------------------------------------------------------------------------------------------------------------------
// width of n number boxes side by side in a panel row
static constexpr uint boxColumns(const uint n) noexcept
{
    return kBoxWidth * n + DelayGroupReference::padding * (n - 1);
}

// The title along the top of a panel, styled as in the reverb, in the accent colour of the panel's first parameter.
// It is aligned left, in line with the names inside the number boxes below it.

template<const char* kTitle>
class DelayPanelTitleWidget final : public LabWidget
{
    static constexpr const float kFontSize = 12.f;
    static constexpr const float kLetterSpacing = 0.4f;
    using NumberBoxReference = Reference::Widgets::NumberBox<>;

    const Color fAccent;

public:
    static constexpr const uint height = 20;

    explicit DelayPanelTitleWidget(LabWidget* const parent, const FaustParameterIndex id)
        : LabWidget(parent),
          fAccent(Reference::kAccentColors[std::clamp(kFaustParameters[id].accentcolor, 1u, 6u) - 1])
    {
        updateSize(false);
    }

private:
    void onNanoDisplay() final
    {
        fontFace("regular");
        fontSize(kFontSize * fScaleFactor);
        textLetterSpacing(kLetterSpacing * fScaleFactor);
        textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
        fillColor(fAccent);
        text((NumberBoxReference::border * 2 + NumberBoxReference::margin) * fScaleFactor, getHeight() * 0.5f, kTitle, nullptr);
        textLetterSpacing(0.f);
    }

    void updateSize(const bool updateChildren) final
    {
        setSize(std::max(1u, getWidth()), d_roundToUnsignedInt(height * fScaleFactor));
        LabWidget::updateSize(updateChildren);
    }
};

static constexpr const char kTitleOptions[] = "OPTIONS";
static constexpr const char kTitleRepeats[] = "REPEATS";
static constexpr const char kTitleDynamics[] = "DYNAMICS";
static constexpr const char kTitleModulation[] = "MODULATION";
static constexpr const char kTitleOutput[] = "OUTPUT";
static constexpr const char kTitleTimeLeft[] = "LEFT";
static constexpr const char kTitleTimeRight[] = "RIGHT";
static constexpr const char kTitleFilter[] = "FILTER";

// A floating panel: a title, then rows of controls, kContentWidth wide inside its rim and as tall as the title and
// rows need, or kFixedHeight tall when that is not 0. With a kContentWidth of 0 it takes the width its parent gives it.

template<uint kContentWidth, uint kFixedHeight, const char* kTitle, class... Rows>
class DelaySizedPanelWidget final : public ReferenceContainerWidget<DelayGroupReference, kVertical>
{
    using R = DelayGroupReference;
    using BaseWidget = ReferenceContainerWidget<R, kVertical>;

    static constexpr const uint kWidth = (R::border + R::margin) * 2 + kContentWidth;
    static constexpr const uint kHeight = kFixedHeight != 0
                                        ? kFixedHeight
                                        : (R::border + R::margin) * 2 + DelayPanelTitleWidget<kTitle>::height
                                          + (Rows::height + ...) + R::padding * static_cast<uint>(sizeof...(Rows));

    const std::shared_ptr<LabWidget> fTitle =
        addWidget<DelayPanelTitleWidget<kTitle>>(std::tuple_element_t<0, std::tuple<Rows...>>::firstId);
    const std::array<std::shared_ptr<LabWidget>, sizeof...(Rows)> rows {
        addWidget<Rows>()...
    };

public:
    static constexpr const uint width = kWidth;
    static constexpr const uint height = kHeight;

    explicit DelaySizedPanelWidget(LabWidget* const parent)
        : BaseWidget(parent)
    {
        updateSize(false);
    }

private:
    void updateSize(const bool updateChildren) final
    {
        if constexpr (kContentWidth != 0)
            LabWidget::setSize(d_roundToUnsignedInt(kWidth * fScaleFactor), d_roundToUnsignedInt(kHeight * fScaleFactor));
        else
            LabWidget::setHeight(d_roundToUnsignedInt(kHeight * fScaleFactor));
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

template<uint kContentWidth, const char* kTitle, class... Rows>
using DelayPanelWidget = DelaySizedPanelWidget<kContentWidth, 0, kTitle, Rows...>;

using DelayRepeatPanelWidget = DelayPanelWidget<boxColumns(1), kTitleRepeats,
    DelayBoxRowWidget<kFaustParameterFeedback>,
    DelayBoxRowWidget<kFaustParameterCross>,
    DelayBoxRowWidget<kFaustParameterDrive>>;

using DelayDynamicsPanelWidget = DelayPanelWidget<boxColumns(1), kTitleDynamics,
    DelayPanelRowWidget<kBoxHeight, DelayMeterBox<kFaustParameterDeess_amount, kFaustParameterDeess_meter>>,
    DelayPanelRowWidget<kBoxHeight, DelayMeterBox<kFaustParameterDuck, kFaustParameterDuck_meter>>>;

using DelayModulationPanelWidget = DelayPanelWidget<boxColumns(1), kTitleModulation,
    DelayBoxRowWidget<kFaustParameterMod_rate>,
    DelayBoxRowWidget<kFaustParameterMod_depth>,
    DelayBoxRowWidget<kFaustParameterMod_stereo_phase>>;

using DelayOutputPanelWidget = DelayPanelWidget<boxColumns(1), kTitleOutput,
    DelayBoxRowWidget<kFaustParameterWidth>,
    DelayBoxRowWidget<kFaustParameterDrywet>>;

// Time L and R give way to Div L and R, the musical divisions, while Sync is on.

using DelayTimeLeftPanelWidget = DelayPanelWidget<0, kTitleTimeLeft,
    DelayPanelRowWidget<kBoxHeight,
                        DelayBox<kFaustParameterOffset_l>,
                        DelayFreeOnly<DelayBox<kFaustParameterTime_l>>,
                        DelaySyncOnly<DelayMenuBox<kFaustParameterDiv_l>>>>;

using DelayTimeRightPanelWidget = DelayPanelWidget<0, kTitleTimeRight,
    DelayPanelRowWidget<kBoxHeight,
                        DelayFreeOnly<DelayLinkedBox<kFaustParameterTime_r>>,
                        DelaySyncOnly<DelayLinkedMenuBox<kFaustParameterDiv_r>>,
                        DelayBox<kFaustParameterOffset_r>>>;

using DelayFilterPanelWidget = DelayPanelWidget<boxColumns(1), kTitleFilter,
    DelayBoxRowWidget<kFaustParameterHp_freq>,
    DelayBoxRowWidget<kFaustParameterLp_freq>>;

// as tall as the Filter panel, level with it across the scope; its four checkbox rows reach 2 px into its bottom margin
using DelayOptionsPanelWidget = DelaySizedPanelWidget<boxColumns(1), DelayFilterPanelWidget::height, kTitleOptions,
    DelayPanelRowWidget<kRowHeight, DelayCheckbox<kFaustParameterSync>>,
    DelayPanelRowWidget<kRowHeight, DelayCheckbox<kFaustParameterLink>>,
    DelayPanelRowWidget<kRowHeight, DelayChoiceCheckbox<kFaustParameterPingpong, 1>>,
    DelayPanelRowWidget<kRowHeight, DelayChoiceCheckbox<kFaustParameterPingpong, 2>>>;

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
    // the panels' height, kPanelGap apart, unscaled
    static constexpr const uint height = (Panels::height + ...) + kPanelGap * static_cast<uint>(sizeof...(Panels) - 1);

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
// The Expert page, from the top down: the Mode toggle and the Time panels, each across the whole width, then the scope
// between two columns of panels: Repeats, Options and Dynamics on the left, Modulation, Filter and Output on the right.
// The body is as tall as the taller column, so the scope ends level with the bottom panels.

using DelayLeftColumnWidget =
    DelayColumnWidget<DelayRepeatPanelWidget, DelayOptionsPanelWidget, DelayDynamicsPanelWidget>;
using DelayRightColumnWidget =
    DelayColumnWidget<DelayModulationPanelWidget, DelayFilterPanelWidget, DelayOutputPanelWidget>;

class DelayBodyWidget final : public ReferenceContainerWidget<Reference::Padding<kPanelGap>>
{
    using BaseWidget = ReferenceContainerWidget<Reference::Padding<kPanelGap>>;

    const std::shared_ptr<LabWidget> fLeft = addWidget<DelayLeftColumnWidget>();
    const std::shared_ptr<DelayScopeWidget> fScope = addWidget<DelayScopeWidget, Expanding>();
    const std::shared_ptr<LabWidget> fRight = addWidget<DelayRightColumnWidget>();

public:
    explicit DelayBodyWidget(LabWidget* const parent)
        : BaseWidget(parent)
    {
        updateSize(false);
    }

private:
    void updateSize(const bool updateChildren) final
    {
        LabWidget::setHeight(d_roundToUnsignedInt(std::max(DelayLeftColumnWidget::height, DelayRightColumnWidget::height)
                                                  * fScaleFactor));
        BaseWidget::updateSize(updateChildren);
    }

public:
    [[nodiscard]] DelayScopeWidget* getScope() const noexcept
    {
        return fScope.get();
    }
};

// The Mode toggle (Digital / Tape / BBD) across the whole width, the row as tall as the toggle.

class DelayModeRowWidget final : public ReferenceContainerWidget<Reference::Zero>
{
    using BaseWidget = ReferenceContainerWidget<Reference::Zero>;

    const std::shared_ptr<LabWidget> fToggle =
        addWidget<PillToggleWidget<Expanding>, Expanding>(kFaustParameters[kFaustParameterMode],
                                                          kParametersMainStart + kFaustParameterMode);

public:
    explicit DelayModeRowWidget(LabWidget* const parent)
        : BaseWidget(parent)
    {
        updateSize(false);
    }

private:
    void updateSize(const bool updateChildren) final
    {
        LabWidget::setHeight(fToggle->getHeight());
        BaseWidget::updateSize(updateChildren);
    }
};

// The left and right Time panels sharing the whole width.

class DelayTimeBarWidget final : public ReferenceContainerWidget<Reference::Padding<kPanelGap>>
{
    using BaseWidget = ReferenceContainerWidget<Reference::Padding<kPanelGap>>;

    const std::shared_ptr<LabWidget> fLeft = addWidget<DelayTimeLeftPanelWidget, Expanding>();
    const std::shared_ptr<LabWidget> fRight = addWidget<DelayTimeRightPanelWidget, Expanding>();

public:
    explicit DelayTimeBarWidget(LabWidget* const parent)
        : BaseWidget(parent)
    {
        updateSize(false);
    }

private:
    void updateSize(const bool updateChildren) final
    {
        LabWidget::setHeight(fLeft->getHeight());
        BaseWidget::updateSize(updateChildren);
    }
};

class DelayExpertWidget final : public ReferenceContainerWidget<Reference::Padding<kPanelGap>, kVertical>
{
    using BaseWidget = ReferenceContainerWidget<Reference::Padding<kPanelGap>, kVertical>;

    const std::shared_ptr<LabWidget> fMode = addWidget<DelayModeRowWidget>();
    const std::shared_ptr<LabWidget> fTime = addWidget<DelayTimeBarWidget>();
    const std::shared_ptr<DelayBodyWidget> fBody = addWidget<DelayBodyWidget>();
    const std::shared_ptr<LabWidget> fSpacer = addSpacer();

public:
    explicit DelayExpertWidget(LabWidget* const parent)
        : BaseWidget(parent) {}

    [[nodiscard]] DelayScopeWidget* getScope() const noexcept
    {
        return fBody->getScope();
    }
};

// --------------------------------------------------------------------------------------------------------------------
// The menu of a parameter's options, styled as main's menu: a dark rounded panel of rows, the current option filled
// with the accent colour, the hovered one tinted with it. It opens above the box it belongs to, or below it when
// there is no room above, and is at least as wide as the box. While open it is an overlay covering the whole stage,
// so it draws over everything and takes every click: one on an option picks it, any other closes the menu.

class DelayMenuWidget final : public LabWidget
{
    struct R {
        static constexpr const Color backgroundColor { 0x1e, 0x1f, 0x24, 0.96f };
        static constexpr const Color borderColor { 0.f, 0.f, 0.f, 0.45f };
        static constexpr const float border = 1.f;
        static constexpr const float borderRadius = 5.f;
        static constexpr const float margin = 4.f;
        static constexpr const float padding = 4.f;
        struct Item {
            static constexpr const Color color = Reference::Colors::ink2;
            static constexpr const Color colorChecked = Reference::Colors::track;
            static constexpr const float fontSize = 15.f;
            static constexpr const float borderRadius = 5.f;
            static constexpr const float height = 18.f;
            static constexpr const float margin = 4.f;
        };
    };

    FaustParameterIndex fId = kFaustParameterCount;
    Color fAccent;
    Rectangle<float> fBox;
    int fHoverRow = -1;

public:
    explicit DelayMenuWidget(LabWidget* const parent)
        : LabWidget(parent)
    {
        hide();
    }

    // anchor is the box the menu belongs to, in absolute coordinates
    void open(const FaustParameterIndex id, const Rectangle<float>& anchor)
    {
        const FaustParameter& param = kFaustParameters[id];
        const float s = fScaleFactor;
        const float inset = (R::border + R::margin) * s;

        fId = id;
        fAccent = Reference::kAccentColors[std::clamp(param.accentcolor, 1u, 6u) - 1];
        fHoverRow = -1;

        fontFace("regular");
        fontSize(R::Item::fontSize * s);
        textAlign(0);

        float itemWidth = 0.f;
        for (uint i = 0; i < param.scalePointCount; ++i)
        {
            Rectangle<float> bounds;
            textBounds(0, 0, param.scalePoints[i].label, nullptr, bounds);
            itemWidth = std::max(itemWidth, bounds.getWidth() + R::Item::margin * 2 * s);
        }

        const uint count = param.scalePointCount;
        const float width = std::max(anchor.getWidth(), itemWidth + inset * 2);
        const float height = inset * 2 + (count * R::Item::height + (count - 1) * R::padding) * s;
        const float ax = anchor.getX() - getAbsoluteX();
        const float ay = anchor.getY() - getAbsoluteY();
        const float x = std::clamp(ax - (width - anchor.getWidth()) * 0.5f, 0.f, std::max(0.f, getWidth() - width));
        const float y = ay > height ? ay - height : std::min(ay + anchor.getHeight(), std::max(0.f, getHeight() - height));

        fBox = Rectangle<float>(x, y, width, height);

        show();
        toFront();
        repaint();
    }

private:
    [[nodiscard]] float rowY(const uint row) const noexcept
    {
        return fBox.getY() + (R::border + R::margin + row * (R::Item::height + R::padding)) * fScaleFactor;
    }

    [[nodiscard]] int rowAt(const Point<double>& pos) const noexcept
    {
        const float s = fScaleFactor;
        const float inset = (R::border + R::margin) * s;
        const float x = pos.getX();
        const float y = pos.getY();

        if (x < fBox.getX() + inset || x > fBox.getX() + fBox.getWidth() - inset)
            return -1;

        for (uint i = 0; i < kFaustParameters[fId].scalePointCount; ++i)
            if (y >= rowY(i) && y < rowY(i) + R::Item::height * s)
                return static_cast<int>(i);

        return -1;
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
        const float inset = (R::border + R::margin) * s;
        const int current = d_roundToInt(fInterface->getParameterValue(kParametersMainStart + fId));

        beginPath();
        roundedRect(fBox.getX() + 0.5f, fBox.getY() + 0.5f, fBox.getWidth() - 1.f, fBox.getHeight() - 1.f,
                    R::borderRadius * s);
        fillColor(R::backgroundColor);
        fill();
        strokeColor(R::borderColor);
        strokeWidth(R::border * s);
        stroke();

        fontFace("regular");
        fontSize(R::Item::fontSize * s);
        textAlign(ALIGN_CENTER | ALIGN_MIDDLE);

        const float ix = fBox.getX() + inset;
        const float iw = fBox.getWidth() - inset * 2;
        const float ih = R::Item::height * s;

        for (uint i = 0; i < param.scalePointCount; ++i)
        {
            const float iy = rowY(i);
            const bool checked = param.scalePoints[i].value == current;

            if (checked || static_cast<int>(i) == fHoverRow)
            {
                beginPath();
                roundedRect(ix, iy, iw, ih, R::Item::borderRadius * s);
                fillColor(checked ? fAccent : Color(fAccent, 0.15f));
                fill();
            }

            fillColor(checked ? R::Item::colorChecked : R::Item::color);
            text(ix + iw * 0.5f, iy + ih * 0.5f, param.scalePoints[i].label, nullptr);
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

    // created last, so it draws over everything and gets events first
    const std::unique_ptr<DelayMenuWidget> fMenu { new DelayMenuWidget(this) };

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

    void openMenu(const FaustParameterIndex id, const Rectangle<float>& anchor)
    {
        fMenu->open(id, anchor);
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
        fMenu->hide();
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

    void updateSize(const bool updateChildren) final
    {
        BaseWidget::updateSize(updateChildren);

        fMenu->setAbsolutePos(getAbsolutePos());
        fMenu->setSize(getSize());
    }
};

// opens the stage's menu for this arrow's parameter, anchored to the number box the arrow sits in
void DelayMenuArrowWidget::openMenu()
{
    SubWidget* const box = dynamic_cast<SubWidget*>(getParentWidget());
    DISTRHO_SAFE_ASSERT_RETURN(box != nullptr,);

    const Rectangle<float> anchor(box->getAbsoluteX(), box->getAbsoluteY(), box->getWidth(), box->getHeight());

    for (Widget* widget = box->getParentWidget(); widget != nullptr;)
    {
        if (DelayStageWidget* const stage = dynamic_cast<DelayStageWidget*>(widget))
            return stage->openMenu(fId, anchor);

        SubWidget* const sub = dynamic_cast<SubWidget*>(widget);
        widget = sub != nullptr ? sub->getParentWidget() : nullptr;
    }
}

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
