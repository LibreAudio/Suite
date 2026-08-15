// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "empty.hpp"
#include "interface.hpp"

#include "Layout.hpp"

#include <memory>
#include <type_traits>

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------
// widget container, with an horizontal or vertical layout for child widgets

enum LibreAudioOrientation {
    kHorizontal,
    kVertical,
};

template<class BaseWidget, class R, LibreAudioOrientation orientation = kHorizontal>
class LibreAudioContainerBaseWidget : public BaseWidget,
                                      public std::conditional_t<orientation == kHorizontal, HorizontalLayout, VerticalLayout>
{
public:
    using Layout = std::conditional_t<orientation == kHorizontal, HorizontalLayout, VerticalLayout>;
    using ResizeEvent = typename BaseWidget::ResizeEvent;

    explicit LibreAudioContainerBaseWidget(LibreAudioWidget* const parent)
        : BaseWidget(parent)
    {
        _initSize();
    }

    explicit LibreAudioContainerBaseWidget(LibreAudioTopLevelWidget* const parent)
        : BaseWidget(parent)
    {
        _initSize();
    }

    explicit LibreAudioContainerBaseWidget(Window& windowToMapTo, LibreAudioUIWidgetInterface* const iface)
        : BaseWidget(windowToMapTo, iface)
    {
        _initSize();
    }

protected:
    std::unique_ptr<LibreAudioWidget> addSpacer()
    {
        std::unique_ptr<LibreAudioWidget> widget { new LibreAudioEmptyWidget(this) };
        Layout::widgets.push_back({ widget.get(), Expanding });
        return widget;
    }

    template<class W,
             SizeHint sizeHint = Fixed,
             typename = std::enable_if_t<std::is_base_of_v<LibreAudioWidget, W>>>
    std::unique_ptr<W> addWidget()
    {
        std::unique_ptr<W> widget { new W(this) };
        Layout::widgets.push_back({ widget.get(), sizeHint });
        if (sizeHint == Fixed && widget->getSize().isNull())
            d_stderr2("Error: addWidget called with Fixed sizeHint but widget '%s' does not have a known size",
                      widget->getName());
        return widget;
    }

    void onNanoDisplay() override
    {
        const float w = BaseWidget::getWidth();
        const float h = BaseWidget::getHeight();

        BaseWidget::beginPath();

        if constexpr (std::is_same_v<BaseWidget, LibreAudioWidget>)
        {
            if constexpr (R::borderRadius != 0)
                BaseWidget::roundedRect(0, 0, w, h, R::borderRadius * this->fScaleFactor);
            else
                BaseWidget::rect(0, 0, w, h);

            if constexpr (d_isNotZero(R::backgroundColor.alpha))
            {
                BaseWidget::fillColor(R::backgroundColor);
                BaseWidget::fill();
            }
        }

        if constexpr (R::border != 0 && d_isNotZero(R::borderColor.alpha))
        {
            BaseWidget::strokeColor(R::borderColor);
            BaseWidget::strokeWidth(R::border * 2 * this->fScaleFactor);
            BaseWidget::stroke();
        }
    }

    void onResize(const ResizeEvent& ev) override
    {
        BaseWidget::onResize(ev);

        const float fScaleFactor = this->fScaleFactor;
        const uint border = d_roundToUnsignedInt(R::border * fScaleFactor);
        const uint margin = d_roundToUnsignedInt(R::margin * fScaleFactor);
        const uint padding = d_roundToUnsignedInt(R::padding * fScaleFactor);

        if constexpr (std::is_same_v<BaseWidget, LibreAudioTopLevelWidget>)
        {
            Layout::align(0, 0, BaseWidget::getWidth(), BaseWidget::getHeight(), padding, border + margin);
        }
        else
        {
            Layout::align(BaseWidget::getAbsoluteX(),
                          BaseWidget::getAbsoluteY(),
                          BaseWidget::getWidth(),
                          BaseWidget::getHeight(),
                          padding,
                          border + margin);
        }
    }

private:
    void _initSize()
    {
        if constexpr (R::width != 0)
            BaseWidget::setWidth(d_roundToUnsignedInt(R::width * this->fScaleFactor));

        if constexpr (R::height != 0)
            BaseWidget::setHeight(d_roundToUnsignedInt(R::height * this->fScaleFactor));
    }
};

// --------------------------------------------------------------------------------------------------------------------

template<class R, LibreAudioOrientation orientation = kHorizontal>
class LibreAudioContainerSubWidget : public LibreAudioContainerBaseWidget<LibreAudioWidget, R, orientation>
{
public:
    using BaseWidget = LibreAudioContainerBaseWidget<LibreAudioWidget, R, orientation>;
    using Layout = std::conditional_t<orientation == kHorizontal, HorizontalLayout, VerticalLayout>;
    using PositionChangedEvent = typename BaseWidget::PositionChangedEvent;

    explicit LibreAudioContainerSubWidget(LibreAudioWidget* const parent)
        : BaseWidget(parent) {}

    explicit LibreAudioContainerSubWidget(LibreAudioTopLevelWidget* const parent)
        : BaseWidget(parent) {}

protected:
    void onPositionChanged(const PositionChangedEvent& ev) override
    {
        BaseWidget::onPositionChanged(ev);

        const float fScaleFactor = this->fScaleFactor;
        const uint border = d_roundToUnsignedInt(R::border * fScaleFactor);
        const uint margin = d_roundToUnsignedInt(R::margin * fScaleFactor);
        const uint padding = d_roundToUnsignedInt(R::padding * fScaleFactor);

        Layout::setAbsolutePos(ev.pos.getX(), ev.pos.getY(), padding, border + margin);
    }
};

// --------------------------------------------------------------------------------------------------------------------

template<class R, LibreAudioOrientation orientation = kHorizontal>
class LibreAudioContainerRootWidget : public LibreAudioContainerBaseWidget<LibreAudioTopLevelWidget, R, orientation>
{
public:
    using BaseWidget = LibreAudioContainerBaseWidget<LibreAudioTopLevelWidget, R, orientation>;

    explicit LibreAudioContainerRootWidget(Window& windowToMapTo, LibreAudioUIWidgetInterface* const iface)
        : BaseWidget(windowToMapTo, iface) {}
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
