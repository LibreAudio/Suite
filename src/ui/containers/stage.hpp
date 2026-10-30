// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../base.hpp"
#include "../widgets/card.hpp"
#include "../widgets/pill-toggle.hpp"
#include "knob-group.hpp"

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

class EasyStageWidget final : public ReferenceContainerWidget<Reference::Stage, Vertical>
{
    using R = Reference::Stage;
    using BaseWidget = ReferenceContainerWidget<R, Vertical>;

    const std::vector<std::shared_ptr<LabWidget>> _ = {
        addSpacer(),
        addWidget<EasyKnobsGroupWidget>(),
        addSpacer(),
        addCards(),
    };

public:
    explicit EasyStageWidget(LabWidget* const parent)
        : BaseWidget(parent) {}

private:
    std::shared_ptr<LabWidget> addCards()
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

#if 0
    void onNanoDisplay() final
    {
        // drawReferenceBackground<R>();

        const float w = getWidth();
        const float h = getHeight();

        // TODO move to layer 2

        beginPath();

        if constexpr (R::borderRadius != 0)
            roundedRect(0, 0, w, h, R::borderRadius * fScaleFactor);
        else
            rect(0, 0, w, h);

        fillPaint(linearGradient(0,
                                 h - h * 0.4f,
                                 0,
                                 h - h * 0.2f,
                                 Reference::Colors::transparent,
                                 Color(0.f, 0.f, 0.f, 0.2f)));
        fill();

        drawReferenceBorder<R>();
    }
#endif
};

// --------------------------------------------------------------------------------------------------------------------

class ExpertStageWidget final : public ReferenceContainerWidget<Reference::Stage, Vertical>
{
    using R = Reference::Stage;
    using BaseWidget = ReferenceContainerWidget<R, Vertical>;

    const std::vector<std::shared_ptr<LabWidget>> _ = {
        addWidget<PillAreaWidget<>>(),
        addSpacer(),
        addWidget<ExpertKnobsGroupWidget>(),
    };

public:
    explicit ExpertStageWidget(LabWidget* const parent)
        : BaseWidget(parent) {}

#if 0
private:
    void onNanoDisplay() final
    {
        const float w = getWidth();
        const float h = getHeight();

        // ------------------------------------------------------------------------------------------------------------
        // draw background and border

        beginPath();

        if constexpr (R::borderRadius != 0)
            roundedRect(0, 0, w, h, R::borderRadius * fScaleFactor);
        else
            rect(0, 0, w, h);

        // if constexpr (d_isNotZero(R::backgroundColor.alpha))
        // {
        //     fillColor(R::backgroundColor);
        //     fill();
        // }

        fillPaint(linearGradient(0,
                                 h - h * 0.4f,
                                 0,
                                 h - h * 0.2f,
                                 Reference::Colors::transparent,
                                 Color(0.f, 0.f, 0.f, 0.2f)));
        fill();

        drawReferenceBorder<R>();
    }
#endif
};

// --------------------------------------------------------------------------------------------------------------------

template<class EasyPageWidget = EasyStageWidget, class ExpertPageWidget = ExpertStageWidget>
class StageWidget : public LabWidget,
                    protected IdleCallback
{
    using BaseWidget = LabWidget;

    std::shared_ptr<EasyPageWidget> fEasy { new EasyPageWidget(this) };
    std::shared_ptr<ExpertPageWidget> fExpert { new ExpertPageWidget(this) };

    std::list<std::shared_ptr<LabWidget>> fGraphWidgets;

public:
    StageWidget(LabWidget* const parent)
        : BaseWidget(parent)
    {
        fExpert->hide();

        addIdleCallback(this);
    }

    [[nodiscard]] const EasyPageWidget* getEasyWidget() const noexcept
    {
        return fEasy.get();
    }

    [[nodiscard]] const ExpertPageWidget* getExpertWidget() const noexcept
    {
        return fExpert.get();
    }

    [[nodiscard]] float getBorderRadius() const noexcept
    {
        return R::borderRadius * fScaleFactor;
    }

protected:
    using R = Reference::Stage;

    Page fLastPage = kPageEasy;

    template<class W,
             typename... Args,
             typename = std::enable_if_t<std::is_base_of_v<LabWidget, W>>>
    std::shared_ptr<W> addGraphWidget(Args... args)
    {
        std::shared_ptr<W> widget { new W(this, args...) };
        widget->toBottom();
        fGraphWidgets.emplace_back(widget);
        return widget;
    }

    void idleCallback() override
    {
        const Page page = getCurrentPage(fInterface);
        if (fLastPage == page)
            return;
        fLastPage = page;

        switch (page)
        {
        case kPageAbout:
        case kPageSettings:
            fEasy->hide();
            fExpert->hide();
            for (const std::shared_ptr<LabWidget>& w : fGraphWidgets)
                w->hide();
            break;
        case kPageEasy:
            fEasy->show();
            fExpert->hide();
            updateGraphSize();
            for (const std::shared_ptr<LabWidget>& w : fGraphWidgets)
                w->show();
            break;
        case kPageExpert:
            fEasy->hide();
            fExpert->show();
            updateGraphSize();
            for (const std::shared_ptr<LabWidget>& w : fGraphWidgets)
                w->show();
            break;
        }
    }

    virtual Size<uint> getGraphSize() const noexcept
    {
        return getSize();
    }

    void updateGraphSize()
    {
        const Size<uint> graphsize = getGraphSize();

        for (const std::shared_ptr<LabWidget>& w : fGraphWidgets)
            w->setSize(graphsize);
    }

    void onNanoDisplay() override
    {
        drawReferenceBackground<R>();
        drawReferenceBorder<R>();
    }

    void onPositionChanged(const PositionChangedEvent& ev) override
    {
        BaseWidget::onPositionChanged(ev);

        static_cast<LabWidget*>(fEasy.get())->setAbsolutePos(ev.pos);
        static_cast<LabWidget*>(fExpert.get())->setAbsolutePos(ev.pos);

        const LabMetrics<R> m { this };
        const  uint space = m.border;
        const Point<int> graphpos(ev.pos.getX() + space, ev.pos.getY() + space);

        for (const std::shared_ptr<LabWidget>& w : fGraphWidgets)
            w->setAbsolutePos(graphpos);
    }

    // void onResize(const ResizeEvent& ev) override
    // {
    //     Widget::onResize(ev);
    //
    //     fEasy->setSize(ev.size);
    //     fExpert->setSize(ev.size);
    // }

    void updateSize(const bool updateChildren) override
    {
        static_cast<LabWidget*>(fEasy.get())->setSize(getSize());
        static_cast<LabWidget*>(fExpert.get())->setSize(getSize());
        updateGraphSize();

        BaseWidget::updateSize(updateChildren);
    }
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
