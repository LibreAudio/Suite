// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../pages/easy.hpp"
#include "../pages/expert.hpp"
#include "../widgets/fft-waveform.hpp"

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

template<class EasyPageWidget = EasyPageWidget, class ExpertPageWidget = ExpertPageWidget>
class CentralAreaWidget : public LabReferenceWidget<Reference::CentralArea>,
                          protected IdleCallback
{
public:
    using R = Reference::CentralArea;

private:
    using BaseWidget = LabReferenceWidget<R>;

    const std::shared_ptr<EasyPageWidget> fEasy { new EasyPageWidget(this) };
    const std::shared_ptr<ExpertPageWidget> fExpert { new ExpertPageWidget(this) };
    std::shared_ptr<WaveformWidget> fWaveform;

    std::list<std::shared_ptr<LabWidget>> fGraphWidgets;

public:
    CentralAreaWidget(LabWidget* const parent)
        : BaseWidget(parent)
    {
        fExpert->hide();

        if constexpr (std::is_same_v<EasyPageWidget, LibreAudio::EasyPageWidget> &&
                      std::is_same_v<ExpertPageWidget, LibreAudio::ExpertPageWidget>)
            fWaveform = addGraphWidget<WaveformWidget>(FFTAnalysis::kSmallestValue, FFTAnalysis::kSmallestValue, 0.f);

        addIdleCallback(this);
    }

    [[nodiscard]] std::shared_ptr<EasyPageWidget> getEasyWidget() const noexcept
    {
        return fEasy;
    }

    [[nodiscard]] std::shared_ptr<ExpertPageWidget> getExpertWidget() const noexcept
    {
        return fExpert;
    }

    [[nodiscard]] std::shared_ptr<WaveformWidget> getWaveformWidget() const noexcept
    {
        return fWaveform;
    }

protected:
    Page fLastPage = kPageEasy;

    template<class W,
             typename... Args,
             typename = std::enable_if_t<std::is_base_of_v<LabWidget, W> && std::is_base_of_v<GraphInterface, W>>>
    std::shared_ptr<W> addGraphWidget(Args... args)
    {
        std::shared_ptr<W> widget { new W(this, args...) };
        fEasy->toFront();
        fExpert->toFront();
        fGraphWidgets.emplace_back(widget);
        if constexpr (std::is_same_v<W, WaveformWidget>)
            fWaveform = widget;
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

    void setGraphRange(const float min, const float start, const float max)
    {
        for (const std::shared_ptr<LabWidget>& w : fGraphWidgets)
            dynamic_cast<GraphInterface*>(w.get())->setRange(min, start, max);
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

        for (const std::shared_ptr<LabWidget>& w : fGraphWidgets)
            reinterpret_cast<GraphInterface*>(w.get())->setBorderRadius(getBorderRadius());

        BaseWidget::updateSize(updateChildren);
    }
};

// --------------------------------------------------------------------------------------------------------------------

template<Orientation orientation>
using CentralAreaSubWidget = LabReferenceContainerWidget<Reference::Margin<0, Reference::CentralArea>, orientation>;

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
