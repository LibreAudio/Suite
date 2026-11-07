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

// --------------------------------------------------------------------------------------------------------------------

namespace LibreAudio {

// height of the number boxes, taller than the reference one, as in voiceIsolation
static constexpr const uint kBoxHeight = 45;

// two checkboxes stack in the height of a number box, this far apart
static constexpr const uint kRowGap = 5;
static constexpr const uint kRowHeight = (kBoxHeight - kRowGap) / 2;

// width of the number boxes in the groups that do not share the stage's width, see setFloatingGroupWidth
static constexpr const uint kTopBoxWidth = 110;

// --------------------------------------------------------------------------------------------------------------------
// Control groups float in rows over the stage. Each group is a panel styled like the mbComp5 band groups, as in the
// limiter.

struct DelayGroupReference : Reference::Zero {
    static constexpr const Color backgroundColor { 0x30, 0x32, 0x3c, 0.7f };
    static constexpr const Color borderColor = backgroundColor;
    static constexpr const Color borderHighlightColor { 0x40, 0x42, 0x4c, 0.8f };
    static constexpr const Color borderShadowColor { 0, 0, 0, 0.12f };
    static constexpr const uint border = 1;
    static constexpr const uint borderRadius = 8;
    static constexpr const uint margin = 8;
    static constexpr const uint padding = 6;
    static constexpr const uint height = kBoxHeight + margin * 2;

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
// Two checkboxes stacked in the height of a number box, kRowGap apart, the stack as wide as the wider one.

template<FaustParameterIndex kTop, FaustParameterIndex kBottom>
class DelayCheckboxStackWidget final : public ReferenceContainerWidget<Reference::Padding<kRowGap>, kVertical>
{
    using BaseWidget = ReferenceContainerWidget<Reference::Padding<kRowGap>, kVertical>;

    const std::array<std::shared_ptr<LabWidget>, 2> widgets {
        addWidget<DelayCheckboxWidget>(kTop),
        addWidget<DelayCheckboxWidget>(kBottom),
    };

public:
    explicit DelayCheckboxStackWidget(LabWidget* const parent)
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

// --------------------------------------------------------------------------------------------------------------------
// A floating panel holding one group of controls: number boxes, which share whatever width the row gives the
// group, and controls of their own width.

class DelayGroupWidget : public ReferenceContainerWidget<DelayGroupReference>
{
    using R = DelayGroupReference;
    using BaseWidget = ReferenceContainerWidget<R>;

    const uint fBoxCount;

public:
    DelayGroupWidget(LabWidget* const parent, const uint boxCount)
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
class DelayBoxGroupWidget final : public DelayGroupWidget
{
    const std::array<std::shared_ptr<LabWidget>, sizeof...(kIds)> widgets {
        addWidget<DelayNumberBoxWidget<kIds>, Expanding>(kIds)...
    };

public:
    explicit DelayBoxGroupWidget(LabWidget* const parent)
        : DelayGroupWidget(parent, sizeof...(kIds)) {}
};

// The Time group: the Time and Offset number boxes, with Sync and Link stacked on the right.

class DelayTimeGroupWidget final : public DelayGroupWidget
{
    const std::array<std::shared_ptr<LabWidget>, 5> widgets {
        addWidget<DelayNumberBoxWidget<kFaustParameterTime_l>, Expanding>(kFaustParameterTime_l),
        addWidget<DelayNumberBoxWidget<kFaustParameterTime_r>, Expanding>(kFaustParameterTime_r),
        addWidget<DelayNumberBoxWidget<kFaustParameterOffset_l>, Expanding>(kFaustParameterOffset_l),
        addWidget<DelayNumberBoxWidget<kFaustParameterOffset_r>, Expanding>(kFaustParameterOffset_r),
        addWidget<DelayCheckboxStackWidget<kFaustParameterSync, kFaustParameterLink>>(),
    };

public:
    explicit DelayTimeGroupWidget(LabWidget* const parent)
        : DelayGroupWidget(parent, 4) {}
};

// --------------------------------------------------------------------------------------------------------------------
// The row of groups along the bottom of the stage, a fixed gap apart. Every number box gets the same width, from
// what the row has left once the groups' fixed parts and the gaps are taken out.

struct DelayGroupRowReference : Reference::Zero {
    static constexpr const uint padding = 8;
    static constexpr const uint height = DelayGroupReference::height;
};

class DelayGroupRowWidget final : public ReferenceContainerWidget<DelayGroupRowReference>
{
    using R = DelayGroupRowReference;
    using BaseWidget = ReferenceContainerWidget<R>;

    const std::array<std::shared_ptr<DelayGroupWidget>, 3> fGroups {
        addWidget<DelayBoxGroupWidget<kFaustParameterDeess_amount, kFaustParameterDuck>>(),
        addWidget<DelayBoxGroupWidget<kFaustParameterHp_freq, kFaustParameterLp_freq>>(),
        addWidget<DelayBoxGroupWidget<kFaustParameterWidth, kFaustParameterDrywet>>(),
    };

public:
    explicit DelayGroupRowWidget(LabWidget* const parent)
        : BaseWidget(parent) {}

private:
    void sizeGroups()
    {
        const int gap = d_roundToIntPositive(R::padding * fScaleFactor);
        int available = static_cast<int>(getWidth()) - gap * static_cast<int>(fGroups.size() - 1);
        uint boxes = 0;

        for (const std::shared_ptr<DelayGroupWidget>& group : fGroups)
        {
            available -= static_cast<int>(group->getFixedWidth());
            boxes += group->getBoxCount();
        }

        const uint boxWidth = boxes != 0 ? static_cast<uint>(std::max(0, available) / static_cast<int>(boxes)) : 0;

        for (const std::shared_ptr<DelayGroupWidget>& group : fGroups)
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
// Groups that float at the top of the stage or just above the bottom row, rather than sharing the stage's width:
// their number boxes are kTopBoxWidth wide.

static void setFloatingGroupWidth(DelayGroupWidget& group, const float scaleFactor)
{
    group.setGroupWidth(group.getFixedWidth() + group.getBoxCount() * d_roundToUnsignedInt(kTopBoxWidth * scaleFactor));
}

// The Time group, at the top left of the stage.

class DelayTopRowWidget final : public ReferenceContainerWidget<DelayGroupRowReference>
{
    using BaseWidget = ReferenceContainerWidget<DelayGroupRowReference>;

    const std::shared_ptr<DelayGroupWidget> fTime = addWidget<DelayTimeGroupWidget>();
    const std::shared_ptr<LabWidget> fSpacer = addSpacer();

public:
    explicit DelayTopRowWidget(LabWidget* const parent)
        : BaseWidget(parent) {}

private:
    // the checkboxes know their size only once updated, so the group is sized again after that
    void updateSize(const bool updateChildren) final
    {
        setFloatingGroupWidth(*fTime, fScaleFactor);
        BaseWidget::updateSize(updateChildren);
        setFloatingGroupWidth(*fTime, fScaleFactor);
        BaseWidget::updateSize(updateChildren);
    }
};

// The groups just above the bottom row, Feedback at the left and Modulation at the right.

class DelayMiddleRowWidget final : public ReferenceContainerWidget<DelayGroupRowReference>
{
    using BaseWidget = ReferenceContainerWidget<DelayGroupRowReference>;

    const std::shared_ptr<DelayGroupWidget> fFeedback =
        addWidget<DelayBoxGroupWidget<kFaustParameterFeedback, kFaustParameterCross, kFaustParameterDrive>>();
    const std::shared_ptr<LabWidget> fSpacer = addSpacer();
    const std::shared_ptr<DelayGroupWidget> fModulation =
        addWidget<DelayBoxGroupWidget<kFaustParameterMod_rate, kFaustParameterMod_depth,
                                      kFaustParameterMod_stereo_phase>>();

public:
    explicit DelayMiddleRowWidget(LabWidget* const parent)
        : BaseWidget(parent) {}

private:
    void updateSize(const bool updateChildren) final
    {
        setFloatingGroupWidth(*fFeedback, fScaleFactor);
        setFloatingGroupWidth(*fModulation, fScaleFactor);
        BaseWidget::updateSize(updateChildren);
    }
};

// --------------------------------------------------------------------------------------------------------------------
// The stage is the display, with the starfield behind it. In Expert mode it shows the Time group at its top left, and
// two rows of control groups along its bottom; in Easy mode it shows four preset cards along its bottom. The
// cards are the suite's Easy page cards and do not load anything yet.

class DelayStageWidget final : public ReferenceContainerWidget<Reference::Stage, kVertical>,
                               private IdleCallback
{
    using R = Reference::Stage;
    using BaseWidget = ReferenceContainerWidget<R, kVertical>;

    const std::shared_ptr<DelayTopRowWidget> fTopGroups = addWidget<DelayTopRowWidget>();
    const std::shared_ptr<LabWidget> fSpacer = addSpacer();
    const std::shared_ptr<DelayMiddleRowWidget> fMiddleGroups = addWidget<DelayMiddleRowWidget>();
    const std::shared_ptr<DelayGroupRowWidget> fGroups = addWidget<DelayGroupRowWidget>();
    const std::shared_ptr<CardGroupWidget> fCards = addCards();

    Page fPage = kPageEasy;

public:
    explicit DelayStageWidget(LabWidget* const parent)
        : BaseWidget(parent)
    {
        showPage(getCurrentPage(fInterface));
        addIdleCallback(this);
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

    // the groups in Expert mode, the cards in Easy mode, neither on the other pages
    void showPage(const Page page)
    {
        fPage = page;
        fTopGroups->setVisible(page == kPageExpert);
        fMiddleGroups->setVisible(page == kPageExpert);
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
};

using DelayRootWidget = RootWidget<TopBar, MainAreaContainerWidget<DelayStageWidget>>;

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

public:
    DelayUI()
        : LibreAudioBaseUI()
    {
        createRootWidget<LibreAudio::DelayRootWidget>();

        fRootWidget->enableShaders({ fShaderBackground.get() });
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
