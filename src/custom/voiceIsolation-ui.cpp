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

// --------------------------------------------------------------------------------------------------------------------

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------
// The stage is the display, with the starfield behind it, and a row of control groups floats at its bottom.
// Each group is a panel styled like the mbComp5 band groups.

struct VoiceIsolationGroupReference : Reference::Zero {
    static constexpr const Color backgroundColor { 0xff, 0xff, 0xff, 0.05f };
    static constexpr const uint borderRadius = 8;
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

// An on / off parameter shown as its name only, as tall as a number box: grey when off, accent colour when on,
// 70% accent when hovered.

struct VoiceIsolationToggleReference : Reference::Zero {
    static constexpr const Color color = Reference::Colors::ink3;
    static constexpr const float fontSize = 13.f;
    static constexpr const float letterSpacing = fontSize * 0.06f;
    static constexpr const uint margin = 6;
    static constexpr const uint height = Reference::Widgets::NumberBox<>::height;
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

    void onNanoDisplay() final
    {
        constexpr const Color& accent = accentColor<id>();

        setupFont();
        fillColor(isChecked() ? accent : isHovered() ? accent.withAlpha(0.7f) : R::color);
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

class VoiceIsolationDenoiseGroupWidget final : public VoiceIsolationGroupWidget
{
    const std::array<std::shared_ptr<LabWidget>, 3> widgets {
        addWidget<VoiceIsolationNumberBoxWidget<kFaustParameterMax_reduction>, Expanding>(kFaustParameterMax_reduction),
        addWidget<VoiceIsolationNumberBoxWidget<kFaustParameterRelease>, Expanding>(kFaustParameterRelease),
        addWidget<VoiceIsolationToggleWidget<kFaustParameterPitch_enhancement>>(),
    };

public:
    explicit VoiceIsolationDenoiseGroupWidget(LabWidget* const parent)
        : VoiceIsolationGroupWidget(parent, 2) {}
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

// --------------------------------------------------------------------------------------------------------------------
// The row of groups along the bottom of the display, a fixed gap apart. Every number box gets the same width, from
// what the row has left once the groups' fixed parts and the gaps are taken out.

struct VoiceIsolationGroupRowReference : Reference::Zero {
    static constexpr const uint padding = 8;
    static constexpr const uint height = VoiceIsolationGroupReference::height;
};

class VoiceIsolationGroupRowWidget final : public ReferenceContainerWidget<VoiceIsolationGroupRowReference>
{
    using R = VoiceIsolationGroupRowReference;
    using BaseWidget = ReferenceContainerWidget<R>;

    const std::array<std::shared_ptr<VoiceIsolationGroupWidget>, 2> fGroups {
        addWidget<VoiceIsolationDenoiseGroupWidget>(),
        addWidget<VoiceIsolationGateGroupWidget>(),
    };

public:
    explicit VoiceIsolationGroupRowWidget(LabWidget* const parent)
        : BaseWidget(parent) {}

private:
    void sizeGroups()
    {
        const int gap = d_roundToIntPositive(R::padding * fScaleFactor);
        int available = static_cast<int>(getWidth()) - gap * static_cast<int>(fGroups.size() - 1);
        uint boxes = 0;

        for (const std::shared_ptr<VoiceIsolationGroupWidget>& group : fGroups)
        {
            available -= static_cast<int>(group->getFixedWidth());
            boxes += group->getBoxCount();
        }

        const uint boxWidth = boxes != 0 ? static_cast<uint>(std::max(0, available) / static_cast<int>(boxes)) : 0;

        for (const std::shared_ptr<VoiceIsolationGroupWidget>& group : fGroups)
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
// The stage is the display: the starfield fills it. Along the bottom it shows the row of control groups in Expert
// mode and four preset cards in Easy mode; the cards are the suite's Easy page cards and do not load anything yet.

class VoiceIsolationStageWidget final : public ReferenceContainerWidget<Reference::Stage, kVertical>,
                                        private IdleCallback
{
    using R = Reference::Stage;
    using BaseWidget = ReferenceContainerWidget<R, kVertical>;

    const std::shared_ptr<LabWidget> fSpacer = addSpacer();
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
};

using VoiceIsolationRootWidget = RootWidget<TopBar, MainAreaContainerWidget<VoiceIsolationStageWidget>>;

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

class VoiceIsolationUI final : public LibreAudioBaseUI
{
    const std::unique_ptr<LibreAudio::ShaderBaseWidget> fShaderBackground {
        new LibreAudio::BotShaderWidget<SHADERS_SHADERTOY_CLOUDSTARFIELD_FRAG_DATA,
                                        SHADERS_SHADERTOY_CLOUDSTARFIELD_FRAG_LEN>(this, this)
    };

public:
    VoiceIsolationUI()
        : LibreAudioBaseUI()
    {
        createRootWidget<LibreAudio::VoiceIsolationRootWidget>();

        fRootWidget->enableShaders({ fShaderBackground.get() });
    }

private:
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoiceIsolationUI)
};

UI* createUI()
{
    return new VoiceIsolationUI();
}

END_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------
