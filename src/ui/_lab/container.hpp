// lab: Libre Audio Base-Widgets
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: ISC

#pragma once

#include "empty.hpp"
#include "metrics.hpp"

#include "Layout.hpp"

#include <memory>
#include <type_traits>

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------
// base widget container, with a templated subclass and horizontal or vertical layout

template<class BaseWidget, Orientation orientation>
class ContainerBaseWidgetOf : public BaseWidget,
                              public std::conditional_t<orientation == Horizontal, HorizontalLayout, VerticalLayout>
{
public:
    using Layout = std::conditional_t<orientation == Horizontal, HorizontalLayout, VerticalLayout>;
    using BaseWidget::BaseWidget;

    template<class W = LabEmptyWidget,
             SizeHint sizeHint = Expanding,
             typename = std::enable_if_t<std::is_base_of_v<LabWidget, W>>>
    std::shared_ptr<W> addSpacer()
    {
        std::shared_ptr<W> widget { new W(this) };
        Layout::widgets.push_back({ widget.get(), sizeHint });
        if constexpr (sizeHint == Fixed)
        {
            if (widget->getSize().isNull())
                d_stderr2("Error: addSpacer called with Fixed sizeHint but widget %u: '%s' does not have a known size",
                          widget->getId(),
                          widget->getName());
        }
        return widget;
    }

    template<class W,
             SizeHint sizeHint = Fixed,
             typename... Args,
             typename = std::enable_if_t<std::is_base_of_v<LabWidget, W>>>
    std::shared_ptr<W> addWidget(Args... args)
    {
        std::shared_ptr<W> widget { new W(this, args...) };
        Layout::widgets.push_back({ widget.get(), sizeHint });
        if constexpr (sizeHint == Fixed)
        {
            if (widget->getSize().isNull())
                d_stderr2("Error: addWidget called with Fixed sizeHint but widget %u: '%s' does not have a known size",
                          widget->getId(),
                          widget->getName());
        }
        return widget;
    }

    uint getLastWidgetWidth() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(! Layout::widgets.empty(), 0);

        return static_cast<LabWidget*>(Layout::widgets.back().widget)->getWidth();
    }

    uint getLastWidgetHeight() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(! Layout::widgets.empty(), 0);

        return static_cast<LabWidget*>(Layout::widgets.back().widget)->getHeight();
    }
};

// --------------------------------------------------------------------------------------------------------------------
// reference base widget container

template<class W, class R, Orientation orientation>
class ReferenceContainerBaseWidgetOf : public ContainerBaseWidgetOf<W, orientation>
{
public:
    using BaseWidget = ContainerBaseWidgetOf<W, orientation>;
    using ResizeEvent = typename BaseWidget::ResizeEvent;
    using BaseWidget::BaseWidget;

protected:
    using Layout = typename BaseWidget::Layout;

    // set width or height based on first child
    void autoSize()
    {
        DISTRHO_SAFE_ASSERT_RETURN(! Layout::widgets.empty(),);

        const LabMetrics<R, uint> m { this };
        const uint space = (m.border + m.margin) * 2;

        if constexpr (orientation == Horizontal)
            LabWidget::setHeight(space + Layout::widgets.front().widget->getHeight());
        else
            LabWidget::setWidth(space + Layout::widgets.front().widget->getWidth());

        // update children
        ReferenceContainerBaseWidgetOf::updateSize(true);
    }

    void updateSize(const bool updateChildren) override
    {
        const LabMetrics<R, uint> m { this };

        if constexpr (orientation == Horizontal)
            Layout::setWidth(m.width, m.padding, m.border + m.margin);
        else
            Layout::setHeight(m.height, m.padding, m.border + m.margin);

        BaseWidget::updateSize(updateChildren);

        if constexpr (std::is_same_v<W, LabTopLevelWidget>)
        {
            Layout::align(0, 0, m.width, m.height, m.padding, m.border + m.margin);
        }
        else
        {
            Layout::align(BaseWidget::getAbsoluteX(),
                          BaseWidget::getAbsoluteY(),
                          m.width,
                          m.height,
                          m.padding,
                          m.border + m.margin);
        }

        BaseWidget::updateSize(updateChildren);
    }
};

// --------------------------------------------------------------------------------------------------------------------
// do not allow using `ContainerBaseWidgetOf` after this point

#ifdef __GNUC__
#pragma GCC poison ContainerBaseWidgetOf
#endif

// --------------------------------------------------------------------------------------------------------------------
// reference container (sub) widget class

template<class R, Orientation orientation>
using LabReferenceContainerWidget = ReferenceContainerBaseWidgetOf<LabReferenceWidget<R>, R, orientation>;

#if 0
// possibly useful?
void onPositionChanged(const PositionChangedEvent& ev) override
{
    BaseWidget::onPositionChanged(ev);

    const float fScaleFactor = this->fScaleFactor;
    const uint border = d_roundToUnsignedInt(R::border * fScaleFactor);
    const uint margin = d_roundToUnsignedInt(R::margin * fScaleFactor);
    const uint padding = d_roundToUnsignedInt(R::padding * fScaleFactor);

    Layout::setAbsolutePos(ev.pos.getX(), ev.pos.getY(), padding, border + margin);
}
#endif

// --------------------------------------------------------------------------------------------------------------------
// reference container top-level widget class

template<class R, Orientation orientation>
using LabReferenceContainerTopLevelWidget = ReferenceContainerBaseWidgetOf<LabTopLevelWidget, R, orientation>;

// --------------------------------------------------------------------------------------------------------------------
// do not allow using `ReferenceContainerBaseWidgetOf` after this point

#ifdef __GNUC__
#pragma GCC poison ReferenceContainerBaseWidgetOf
#endif

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL
