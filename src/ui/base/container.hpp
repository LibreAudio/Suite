// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "empty.hpp"
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

class EmptyClass {};

template<class R, LibreAudioOrientation orientation = kHorizontal, class BaseWidget = LibreAudioWidget>
class LibreAudioContainer : public BaseWidget,
                            public std::conditional_t<orientation == kHorizontal, HorizontalLayout, VerticalLayout>,
                            public std::conditional_t<std::is_same_v<BaseWidget, LibreAudioTopLevelWidget>, LibreAudioUIWidgetInterface, EmptyClass>
{
public:
    using Layout = std::conditional_t<orientation == kHorizontal, HorizontalLayout, VerticalLayout>;
    using PositionChangedEvent = typename BaseWidget::PositionChangedEvent;
    using ResizeEvent = typename BaseWidget::ResizeEvent;

    explicit LibreAudioContainer(LibreAudioWidget* const parent)
        : BaseWidget(parent)
    {
        _initSize();
    }

    explicit LibreAudioContainer(LibreAudioTopLevelWidget* const parent)
        : BaseWidget(parent)
    {
        _initSize();
    }

    explicit LibreAudioContainer(Window& windowToMapTo)
        : BaseWidget(windowToMapTo, this)
    {
        // initial size set by DPF UI class
    }

protected:
    std::unique_ptr<LibreAudioWidget> createSpacer()
    {
        std::unique_ptr<LibreAudioWidget> widget { new LibreAudioEmptyWidget(this) };
        Layout::widgets.push_back({ widget.get(), Expanding });
        return widget;
    }

    template<class W,
             SizeHint sizeHint = Fixed,
             typename = std::enable_if_t<std::is_base_of_v<LibreAudioWidget, W>>>
    std::unique_ptr<W> createWidget()
    {
        std::unique_ptr<W> widget { new W(this) };
        Layout::widgets.push_back({ widget.get(), sizeHint });
        if (sizeHint == Fixed && widget->getSize().isNull())
            d_stderr2("Error: createWidget called with Fixed sizeHint but widget does not have a known size");
        return widget;
    }

    void onNanoDisplay() override
    {
        const float w = BaseWidget::getWidth();
        const float h = BaseWidget::getHeight();

        BaseWidget::beginPath();

        if constexpr (std::is_same_v<BaseWidget, LibreAudioTopLevelWidget>)
        {
            BaseWidget::rect(0, 0, w, h);
            BaseWidget::fillPaint(
                BaseWidget::linearGradient(0, 0, 0, h, R::backgroundGradientStart, R::backgroundGradientStop));
            BaseWidget::fill();
        }
        else
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

    // template<typename = std::enable_if_t<std::is_same_v<BaseWidget, LibreAudioWidget>>>
    void onPositionChanged(const PositionChangedEvent& ev)
    // final
    {
        BaseWidget::onPositionChanged(ev);

        const float fScaleFactor = this->fScaleFactor;
        const uint border = d_roundToUnsignedInt(R::border * fScaleFactor);
        const uint margin = d_roundToUnsignedInt(R::margin * fScaleFactor);
        const uint padding = d_roundToUnsignedInt(R::padding * fScaleFactor);

        Layout::setAbsolutePos(ev.pos.getX(), ev.pos.getY(), padding, border + margin);
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

END_NAMESPACE_DISTRHO
